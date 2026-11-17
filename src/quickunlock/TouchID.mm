/*
*  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
*
*  This program is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 2 or (at your option)
*  version 3 of the License.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "quickunlock/TouchID.h"

#include "core/AsyncTask.h"
#include "crypto/Random.h"
#include "crypto/SymmetricCipher.h"
#include "crypto/CryptoHash.h"
#include "config-keepassx.h"

#include <botan/mem_ops.h>

#include <Foundation/Foundation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <LocalAuthentication/LocalAuthentication.h>
#include <Security/Security.h>

#include <QCoreApplication>
#include <QString>
#include <QDebug>
#include <QDeadlineTimer>
#include <QScopeGuard>
#include <QSharedPointer>
#include <QTimer>
#include <QPromise>

inline std::string StatusToErrorMessage(OSStatus status)
{
   CFStringRef text = SecCopyErrorMessageString(status, nullptr);
   if (!text) {
      return std::to_string(status);
   }

   auto msg = CFStringGetCStringPtr(text, kCFStringEncodingUTF8);
   std::string result;
   if (msg) {
       result = msg;
   }
   CFRelease(text);
   return result;
}

inline CFMutableDictionaryRef makeDictionary() {
   return CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

//! Try to delete an existing keychain entry
void TouchID::deleteKeyEntry(const QString& accountName)
{
    NSString* nsAccountName = accountName.toNSString(); // The NSString is released by Qt

    // try to delete an existing entry
    CFMutableDictionaryRef query = makeDictionary();
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrAccount, (__bridge CFStringRef) nsAccountName);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanFalse);
    CFDictionarySetValue(query, kSecUseDataProtectionKeychain, kCFBooleanTrue);

    // get data from the KeyChain
    SecItemDelete(query);
    CFRelease(query);
}

QString TouchID::databaseKeyName(const QUuid& dbUuid)
{
   static const QString keyPrefix = "KeepassXC_TouchID_Keys_";
   return keyPrefix + dbUuid.toString();
}

QString TouchID::errorString() const
{
    return m_error;
}

void TouchID::reset()
{
    m_encryptedMasterKeys.clear();
}

QuickUnlockInterface::KeyStatus TouchID::setKey(const QUuid& dbUuid, const QByteArray& passwordKey, const bool ignoreTouchID)
{
    m_error.clear();

    if (passwordKey.isEmpty()) {
        m_error = QObject::tr("Illegal arguments");
        return ERR_NO_KEY;
    }

    if (m_encryptedMasterKeys.contains(dbUuid)) {
        m_error = QObject::tr("Already stored key for this database");
        return OK;
    }

    // generate random AES 256bit key and IV
    QByteArray randomKey = randomGen()->randomArray(SymmetricCipher::keySize(SymmetricCipher::Aes256_GCM));
    QByteArray randomIV = randomGen()->randomArray(SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM));

    SymmetricCipher aes256Encrypt;
    if (!aes256Encrypt.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Encrypt, randomKey, randomIV)) {
        m_error = QObject::tr("AES initialization failed");
        return ERR_CRYPTO_FAILURE;
    }

    // encrypt and keep result in memory
    QByteArray encryptedMasterKey = passwordKey;
    if (!aes256Encrypt.finish(encryptedMasterKey)) {
        m_error = QObject::tr("AES encrypt failed: %1").arg(aes256Encrypt.errorString().toUtf8().constData());
        return ERR_CRYPTO_FAILURE;
    }

    const QString keyName = databaseKeyName(dbUuid);

    deleteKeyEntry(keyName); // Try to delete the existing key entry

    // prepare adding secure entry to the macOS KeyChain
    CFErrorRef error = nullptr;

    // We need both runtime and compile time checks here to solve the following problems:
    // - Not all flags are available in all OS versions, so we have to check it at compile time
    // - Requesting Biometry/TouchID/DevicePassword when to fingerprint sensor is available will result in runtime error
    SecAccessControlCreateFlags accessControlFlags = 0;
#if XC_COMPILER_SUPPORT(APPLE_BIOMETRY)
    // Needs a special check to work with SecItemAdd, when TouchID is not enrolled and the flag
    // is set, the method call fails with an error. But we want to still set this flag if TouchID is
    // enrolled but temporarily unavailable due to closed lid
    //
    // At least on a Hackintosh the enrolled-check does not work, there LAErrorBiometryNotAvailable gets returned instead of
    // LAErrorBiometryNotEnrolled.
    //
    // That's kinda unfortunate, because now you cannot know for sure if TouchID hardware is either temporarily unavailable or not present
    // at all, because LAErrorBiometryNotAvailable is used for both cases.
    //
    // So to make quick unlock fallbacks possible on these machines you have to try to save the key a second time without this flag, if the
    // first try fails with an error.
    if (!ignoreTouchID) {
        // Prefer the non-deprecated flag when available
        accessControlFlags = kSecAccessControlBiometryCurrentSet;
    }
#elif XC_COMPILER_SUPPORT(TOUCH_ID)
    if (!ignoreTouchID) {
        accessControlFlags = kSecAccessControlTouchIDCurrentSet;
    }
#endif

#if XC_COMPILER_SUPPORT(WATCH_UNLOCK)
      accessControlFlags = accessControlFlags | kSecAccessControlOr | kSecAccessControlWatch;
#endif

#if XC_COMPILER_SUPPORT(TOUCH_ID)
   if (isPasswordFallbackPossible()) {
       accessControlFlags = accessControlFlags | kSecAccessControlOr | kSecAccessControlDevicePasscode;
   }
#endif

   SecAccessControlRef sacObject = SecAccessControlCreateWithFlags(
       kCFAllocatorDefault, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, accessControlFlags, &error);

    if (sacObject == nullptr || error != nullptr) {
        NSError* e = (__bridge NSError*) error;
        m_error = QObject::tr("Error creating security flags: %1").arg(e.localizedDescription.UTF8String);
        return ERR_OTHER;
    }

    NSString *accountName = keyName.toNSString(); // The NSString is released by Qt

    // prepare data (key) to be stored
    QByteArray keychainKeyValue = (randomKey + randomIV).toHex();
    CFDataRef keychainValueData =
        CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<UInt8 *>(keychainKeyValue.data()),
                                    keychainKeyValue.length(), kCFAllocatorDefault);

    CFMutableDictionaryRef attributes = makeDictionary();
    CFDictionarySetValue(attributes, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(attributes, kSecAttrAccount, (__bridge CFStringRef) accountName);
    CFDictionarySetValue(attributes, kSecValueData, (__bridge CFDataRef) keychainValueData);
    CFDictionarySetValue(attributes, kSecAttrSynchronizable, kCFBooleanFalse);
    CFDictionarySetValue(attributes, kSecUseAuthenticationUI, kSecUseAuthenticationUIAllow);
    CFDictionarySetValue(attributes, kSecAttrAccessControl, sacObject);
    CFDictionarySetValue(attributes, kSecUseDataProtectionKeychain, kCFBooleanTrue);

    // add to KeyChain
    OSStatus status = SecItemAdd(attributes, nullptr);

    CFRelease(sacObject);
    CFRelease(attributes);
    
    // Cleanse the key information from the memory
    Botan::secure_scrub_memory(randomKey.data(), randomKey.size());
    Botan::secure_scrub_memory(randomIV.data(), randomIV.size());

    if (status != errSecSuccess) {
        m_error = QString::fromStdString(StatusToErrorMessage(status));
        return ERR_OTHER;
    }

    // memorize which database the stored key is for
    m_encryptedMasterKeys.insert(dbUuid, encryptedMasterKey);
    return OK;
}

/**
 * Generates a random AES 256bit key and uses it to encrypt the PasswordKey that
 * protects the database. The encrypted PasswordKey is kept in memory while the
 * AES key is stored in the macOS KeyChain protected by either TouchID or Apple Watch.
 */
QuickUnlockInterface::KeyStatus TouchID::setKey(const QUuid& dbUuid, const QByteArray& passwordKey)
{
    if (setKey(dbUuid,passwordKey, false) != OK) {
        return setKey(dbUuid, passwordKey, true);
    } else {
        return OK;
    }
}

/**
 * Checks if an encrypted PasswordKey is available for the given database, tries to
 * decrypt it using the KeyChain and if successful, returns it.
 */
QuickUnlockInterface::KeyStatus TouchID::getKey(const QUuid& dbUuid, QByteArray& passwordKey)
{
    passwordKey.clear();
    m_error.clear();

    if (!hasKey(dbUuid)) {
        m_error = QObject::tr("No stored key found");
        return ERR_NO_KEY;
    }

    // query the KeyChain for the AES key
    CFMutableDictionaryRef query = makeDictionary();

    const QString keyName = databaseKeyName(dbUuid);
    NSString* accountName = keyName.toNSString(); // The NSString is released by Qt
    NSString* touchPromptMessage =
        QCoreApplication::translate("DatabaseOpenWidget", "authenticate to access the database")
            .toNSString();  // The NSString is released by Qt

    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrAccount, (__bridge CFStringRef) accountName);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecUseOperationPrompt, (__bridge CFStringRef) touchPromptMessage);
    CFDictionarySetValue(query, kSecUseDataProtectionKeychain, kCFBooleanTrue);

    LAContext* context = [[LAContext alloc] init];
    CFDictionarySetValue(query, kSecUseAuthenticationContext, (__bridge CFTypeRef) context);

    // The macOS user password prompt allows an infinite number of failed authentications, and there
    // doesn't seem to be any way to change that. We therefore start a 30 second timer after triggering the
    // password prompt. If authentication hasn't succeeded when the timer runs out, Quick Unlock is cancelled.
    QDeadlineTimer timeout(30000, Qt::PreciseTimer);
    QTimer timer;
    timer.setSingleShot(true);
    timer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&timer, &QTimer::timeout, &timer, [context] {
        // Cancel context after 30 seconds
        [context invalidate];
    });
    timer.start(30000);

    auto promise = QSharedPointer<QPromise<QPair<NSInteger, QString>>>::create();
    promise->start();
    [context evaluatePolicy:LAPolicyDeviceOwnerAuthentication
            localizedReason:touchPromptMessage reply:^(BOOL success, NSError* error) {
        const auto code = success ? 0 : ([error.domain isEqualToString:LAErrorDomain] ? error.code : LAErrorInvalidContext);
        promise->addResult(qMakePair(code, error ? QString::fromNSString(error.localizedDescription) : QString()));
        promise->finish();
    }];
    const auto authentication = AsyncTask::waitForFuture(promise->future());
    timer.stop();

    // Authentication successful
    CFTypeRef dataTypeRef = nullptr;
    auto releaseData = qScopeGuard([&] {
        if (dataTypeRef) {
            CFRelease(dataTypeRef);
        }
    });
    OSStatus status = errSecAuthFailed;
    if (!timeout.hasExpired() && authentication.first == 0) {
        context.interactionNotAllowed = YES;
        status = SecItemCopyMatching(query, &dataTypeRef);
    }
    [context invalidate];
    [context release];
    CFRelease(query);

    // Context deadline has expired (30 seconds passed)
    if (timeout.hasExpired()) {
        reset(dbUuid);
        m_error = QObject::tr("Quick Unlock authentication timed out");
        return ERR_AUTH_FAILURE;
    }
    if (authentication.first != 0) {
        m_error = authentication.second;
        switch (authentication.first) {
        case LAErrorUserCancel:
        case LAErrorSystemCancel:
        case LAErrorAppCancel:
            return ERR_USER_CANCELED;
        case LAErrorAuthenticationFailed:
        case LAErrorBiometryLockout:
            return ERR_AUTH_FAILURE;
        default:
            return ERR_OTHER;
        }
    }

    if (status == errSecUserCanceled) {
        m_error = QObject::tr("User canceled authentication");
        return ERR_USER_CANCELED;
    } else if (status == errSecAuthFailed) {
        m_error = QObject::tr("Authentication failure");
        return ERR_AUTH_FAILURE;
    } else if (status != errSecSuccess || dataTypeRef == nullptr) {
        m_error = QObject::tr("Unknown error");
        return ERR_OTHER;
    }

    CFDataRef valueData = static_cast<CFDataRef>(dataTypeRef);
    QByteArray dataBytes = QByteArray::fromHex(QByteArray(reinterpret_cast<const char*>(CFDataGetBytePtr(valueData)),
                                                          CFDataGetLength(valueData)));

    // extract AES key and IV from data bytes
    QByteArray key = dataBytes.left(SymmetricCipher::keySize(SymmetricCipher::Aes256_GCM));
    QByteArray iv = dataBytes.right(SymmetricCipher::defaultIvSize(SymmetricCipher::Aes256_GCM));

    SymmetricCipher aes256Decrypt;
    if (!aes256Decrypt.init(SymmetricCipher::Aes256_GCM, SymmetricCipher::Decrypt, key, iv)) {
        m_error = QObject::tr("AES initialization failed");
        return ERR_CRYPTO_FAILURE;
    }

    // decrypt PasswordKey from memory using AES
    passwordKey = m_encryptedMasterKeys[dbUuid];
    if (!aes256Decrypt.finish(passwordKey)) {
        passwordKey.clear();
        m_error = QObject::tr("AES decrypt failed: %1").arg(aes256Decrypt.errorString().toUtf8().constData());
        return ERR_CRYPTO_FAILURE;
    }

    // Cleanse the key information from the memory
    Botan::secure_scrub_memory(key.data(), key.size());
    Botan::secure_scrub_memory(iv.data(), iv.size());

    return OK;
}

bool TouchID::hasKey(const QUuid& dbUuid) const
{
    return m_encryptedMasterKeys.contains(dbUuid);
}

// TODO: Both functions below should probably handle the returned errors to
// provide more information on availability. E.g.: the closed laptop lid results
// in an error (because touch id is not unavailable). That error could be
// displayed to the user when we first check for availability instead of just
// hiding the checkbox.

//! @return true if Apple Watch is available for authentication.
bool TouchID::isWatchAvailable()
{
#if XC_COMPILER_SUPPORT(WATCH_UNLOCK)
   @try {
      LAContext *context = [[LAContext alloc] init];

      LAPolicy policyCode = LAPolicyDeviceOwnerAuthenticationWithWatch;
      NSError *error = nil;
      bool canAuthenticate = [context canEvaluatePolicy:policyCode error:&error];
      [context release];
      if (!canAuthenticate && error) {
         qWarning() << "Quick Unlock:" << error.description.UTF8String;
      }
      return canAuthenticate;
   } @catch (NSException *) {
      return false;
   }
#else
   return false;
#endif
}

//! @return true if Touch ID is available for authentication.
bool TouchID::isTouchIdAvailable()
{
#if XC_COMPILER_SUPPORT(TOUCH_ID)
    @try {
        LAContext *context = [[LAContext alloc] init];

        LAPolicy policyCode = LAPolicyDeviceOwnerAuthenticationWithBiometrics;
        NSError *error = nil;

        bool canAuthenticate = [context canEvaluatePolicy:policyCode error:&error];
        [context release];
        if (!canAuthenticate && error) {
            qWarning() << "Quick Unlock:" << error.description.UTF8String;
        }
        return canAuthenticate;
    } @catch (NSException *) {
        return false;
    }
#else
   return false;
#endif
}

bool TouchID::isPasswordFallbackPossible()
{
#if XC_COMPILER_SUPPORT(TOUCH_ID)
    @try {
        LAContext *context = [[LAContext alloc] init];

        LAPolicy policyCode = LAPolicyDeviceOwnerAuthentication;
        NSError *error = nil;

        bool canAuthenticate = [context canEvaluatePolicy:policyCode error:&error];
        [context release];
        if (!canAuthenticate && error) {
            qWarning() << "Quick Unlock:" << error.description.UTF8String;
        }
        return canAuthenticate;
    } @catch (NSException *) {
        return false;
    }
#else
    return false;
#endif
}

//! @return true if either TouchID or Apple Watch is available at the moment.
bool TouchID::isAvailable() const
{
   // note: we cannot cache the check results because the configuration
   // is dynamic in its nature. User can close the laptop lid or take off
   // the watch, thus making one (or both) of the authentication types unavailable.
   return  isWatchAvailable() || isTouchIdAvailable() || isPasswordFallbackPossible();
}

/**
 * Resets the inner state either for all or for the given database
 */
void TouchID::reset(const QUuid& dbUuid)
{
    if (!dbUuid.isNull()) {
        m_encryptedMasterKeys.remove(dbUuid);
    }
}
