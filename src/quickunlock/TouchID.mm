#include "quickunlock/TouchID.h"

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

#define TOUCH_ID_ENABLE_DEBUG_LOGS() 0
#if TOUCH_ID_ENABLE_DEBUG_LOGS()
#define debug(...) qWarning(__VA_ARGS__)
#else
inline void debug(const char *message, ...)
{
   Q_UNUSED(message);
}
#endif

inline std::string StatusToErrorMessage(OSStatus status)
{
   CFStringRef text = SecCopyErrorMessageString(status, NULL);
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

inline void LogStatusError(const char *message, OSStatus status)
{
   if (!status) {
      return;
   }

   std::string msg = StatusToErrorMessage(status);
   debug("%s: %s", message, msg.c_str());
}

inline CFMutableDictionaryRef makeDictionary() {
   return CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
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

   // get data from the KeyChain
   OSStatus status = SecItemDelete(query);
   LogStatusError("TouchID::deleteKeyEntry - Error deleting existing entry", status);
}

QString TouchID::databaseKeyName(const QUuid& dbUuid)
{
   static const QString keyPrefix = "KeepassXC_TouchID_Keys_";
   return keyPrefix + dbUuid.toString();
}

QString TouchID::errorString() const
{
    // TODO
    return "";
}

void TouchID::reset()
{
    m_encryptedMasterKeys.clear();
}

/**
 * Store the serialized database key into the macOS key store. The OS handles encrypt/decrypt operations.
 * https://developer.apple.com/documentation/security/keychain_services/keychain_items
 */
bool TouchID::setKey(const QUuid& dbUuid, const QByteArray& key)
{
    if (key.isEmpty()) {
        debug("TouchID::setKey - illegal arguments");
        return false;
    }

    const auto keyName = databaseKeyName(dbUuid);
    // Try to delete the existing key entry
    deleteKeyEntry(keyName);

    // prepare adding secure entry to the macOS KeyChain
    CFErrorRef error = NULL;

    // We need both runtime and compile time checks here to solve the following problems:
    // - Not all flags are available in all OS versions, so we have to check it at compile time
    // - Requesting Biometry/TouchID when to fingerprint sensor is available will result in runtime error
    SecAccessControlCreateFlags accessControlFlags = 0;
    if (isTouchIdAvailable()) {
#if XC_COMPILER_SUPPORT(APPLE_BIOMETRY)
       // Prefer the non-deprecated flag when available
       accessControlFlags = kSecAccessControlBiometryCurrentSet;
#elif XC_COMPILER_SUPPORT(TOUCH_ID)
       accessControlFlags = kSecAccessControlTouchIDCurrentSet;
#endif
    }

   if (isWatchAvailable()) {
#if XC_COMPILER_SUPPORT(WATCH_UNLOCK)
      accessControlFlags = accessControlFlags | kSecAccessControlOr | kSecAccessControlWatch;
#endif
   }

   SecAccessControlRef sacObject = SecAccessControlCreateWithFlags(
       kCFAllocatorDefault, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, accessControlFlags, &error);

    if (sacObject == NULL || error != NULL) {
        auto e = (__bridge NSError*) error;
        debug("TouchID::setKey - Error creating security flags: %s", e.localizedDescription.UTF8String);
        return false;
    }

    auto accountName = keyName.toNSString();
    auto keyBase64 = key.toBase64();

    // prepare data (key) to be stored
    auto keyValueData = CFDataCreateWithBytesNoCopy(
        kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(keyBase64.data()),
        keyBase64.length(), kCFAllocatorDefault);

    auto attributes = makeDictionary();
    CFDictionarySetValue(attributes, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(attributes, kSecAttrAccount, (__bridge CFStringRef) accountName);
    CFDictionarySetValue(attributes, kSecValueData, (__bridge CFDataRef) keyValueData);
    CFDictionarySetValue(attributes, kSecAttrSynchronizable, kCFBooleanFalse);
    CFDictionarySetValue(attributes, kSecUseAuthenticationUI, kSecUseAuthenticationUIAllow);
#ifndef QT_DEBUG
    // Only use TouchID when in release build, also requires application entitlements and signing
    CFDictionarySetValue(attributes, kSecAttrAccessControl, sacObject);
#endif

    // add to KeyChain
    OSStatus status = SecItemAdd(attributes, NULL);
    LogStatusError("TouchID::setKey - Error adding new keychain item", status);

    CFRelease(sacObject);
    CFRelease(attributes);

    return status == errSecSuccess;
}

/**
 * Retrieve serialized key data from the macOS Keychain after successful authentication
 * with TouchID or Watch interface.
 */
bool TouchID::getKey(const QUuid& dbUuid, QByteArray& key)
{
    key.clear();

    if (!hasKey(dbUuid)) {
        debug("TouchID::getKey - No stored key found");
        return false;
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

    // get data from the KeyChain
    CFTypeRef dataTypeRef = NULL;
    OSStatus status = SecItemCopyMatching(query, &dataTypeRef);
    CFRelease(query);

    if (status == errSecUserCanceled) {
        // user canceled the authentication, return true with empty key
        debug("TouchID::getKey - User canceled authentication");
        return true;
    } else if (status != errSecSuccess || dataTypeRef == NULL) {
        LogStatusError("TouchID::getKey - key query error", status);
        return false;
    }

    // Convert value returned to serialized key
    CFDataRef valueData = static_cast<CFDataRef>(dataTypeRef);
    key = QByteArray::fromBase64(QByteArray(reinterpret_cast<const char*>(CFDataGetBytePtr(valueData)),
                                 CFDataGetLength(valueData)));
    CFRelease(dataTypeRef);

    return true;
}

bool TouchID::hasKey(const QUuid& dbUuid) const
{
    const QString keyName = databaseKeyName(dbUuid);
    NSString* accountName = keyName.toNSString(); // The NSString is released by Qt

    CFMutableDictionaryRef query = makeDictionary();
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrAccount, (__bridge CFStringRef) accountName);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanFalse);

    CFTypeRef item = NULL;
    OSStatus status = SecItemCopyMatching(query, &item);
    CFRelease(query);

    return status == errSecSuccess;
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
      NSError *error;

      bool canAuthenticate = [context canEvaluatePolicy:policyCode error:&error];
      [context release];
      if (error) {
         debug("Apple Watch is not available: %s", error.localizedDescription.UTF8String);
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
      NSError *error;

      bool canAuthenticate = [context canEvaluatePolicy:policyCode error:&error];
      [context release];
      if (error) {
         debug("Touch ID is not available: %s", error.localizedDescription.UTF8String);
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
   return  isWatchAvailable() || isTouchIdAvailable();
}

bool TouchID::canRemember() const
{
    return true;
}

/**
 * Resets the inner state either for all or for the given database
 */
void TouchID::reset(const QUuid& dbUuid)
{
   deleteKeyEntry(databaseKeyName(dbUuid));
}
