/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2016 Lennart Glauer <mail@lennart-glauer.de>
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

#import "AppKitImpl.h"
#import "MacUtils.h"

#import <QWindow>
#import <QMenu>
#import <QMenuBar>
#import <Cocoa/Cocoa.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <LocalAuthentication/LocalAuthentication.h>
#import <Security/Security.h>
#if __clang_major__ >= 13 && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_3
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#endif

#include "config-keepassx.h"

@implementation AppKitImpl

- (id) initWithObject:(AppKit*)appkit
{
    self = [super init];

    if (self) {
        m_appkit = appkit;
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                           selector:@selector(didDeactivateApplicationObserver:)
                                                               name:NSWorkspaceDidDeactivateApplicationNotification
                                                             object:nil];

        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                            selector:@selector(userSwitchHandler:)
                                                                name:NSWorkspaceSessionDidResignActiveNotification
                                                                object:nil];

        [NSApp addObserver:self forKeyPath:@"effectiveAppearance" options:NSKeyValueObservingOptionNew context:nil];
    }
    return self;
}

//
// Update last active application property
//
- (void) didDeactivateApplicationObserver:(NSNotification*) notification
{
    NSDictionary* userInfo = notification.userInfo;
    NSRunningApplication* app = [userInfo objectForKey:NSWorkspaceApplicationKey];

    if (app.processIdentifier != [self ownProcessId]) {
        self.lastActiveApplication = app;
    }
}

- (void) observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context
{
    Q_UNUSED(object)
    Q_UNUSED(change)
    Q_UNUSED(context)
    if ([keyPath isEqualToString:@"effectiveAppearance"]) {
        if (m_appkit) {

            void (^emitBlock)(void) = ^{
                emit m_appkit->interfaceThemeChanged();
            };

            if(@available(macOS 11.0, *)) {
                // Not sure why exactly this call is needed, but Apple sample code uses it so it's best to use it here too
                [NSApp.effectiveAppearance performAsCurrentDrawingAppearance:emitBlock];
            }
            else {
                emitBlock();
            }
        }
    }
}


//
// Get process id of frontmost application (-> keyboard input)
//
- (pid_t) activeProcessId
{
    return [NSWorkspace sharedWorkspace].frontmostApplication.processIdentifier;
}

//
// Get process id of own process
//
- (pid_t) ownProcessId
{
    return [NSProcessInfo processInfo].processIdentifier;
}

//
// Activate application by process id
//
- (bool) activateProcess:(pid_t) pid
{
    NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    return [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
}

//
// Hide application by process id
//
- (bool) hideProcess:(pid_t) pid
{
    NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    return [app hide];
}

//
// Get application hidden state by process id
//
- (bool) isHidden:(pid_t) pid
{
    NSRunningApplication* app = [NSRunningApplication runningApplicationWithProcessIdentifier:pid];
    return [app isHidden];
}

//
// Get state of macOS Dark Mode color scheme
//
- (bool) isDarkMode
{
    return [NSApp.effectiveAppearance.name isEqualToString:NSAppearanceNameDarkAqua];
}


//
// Get global menu bar theme state
//
- (bool) isStatusBarDark
{
#if __clang_major__ >= 9 && MAC_OS_X_VERSION_MIN_REQUIRED >= 101000
    if (@available(macOS 10.17, *)) {
        // This is an ugly hack, but I couldn't find a way to access QTrayIcon's NSStatusItem.
        NSStatusItem* dummy = [[NSStatusBar systemStatusBar] statusItemWithLength:0];
        NSString* appearance = [dummy.button.effectiveAppearance.name lowercaseString];
        [[NSStatusBar systemStatusBar] removeStatusItem:dummy];
        return [appearance containsString:@"dark"];
    }
#endif

    return [self isDarkMode];
}

//
// Notification for user switch
//
- (void) userSwitchHandler:(NSNotification*) notification
{
    if ([[notification name] isEqualToString:NSWorkspaceSessionDidResignActiveNotification] && m_appkit)
    {
        emit m_appkit->userSwitched();
    }
}

//
// Check if accessibility is enabled, may show an popup asking for permissions
//
- (bool) enableAccessibility
{
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    // Request accessibility permissions for Auto-Type type on behalf of the user
    NSDictionary* opts = @{static_cast<id>(kAXTrustedCheckOptionPrompt): @YES};
    return AXIsProcessTrustedWithOptions(static_cast<CFDictionaryRef>(opts));
#else
    return YES;
#endif
}

//
// Check if screen recording is enabled, may show an popup asking for permissions
//
- (bool) enableScreenRecording
{
#if __clang_major__ >= 13 && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_3
    if (@available(macOS 12.3, *)) {
        __block BOOL hasPermission = NO;
        dispatch_semaphore_t sema = dispatch_semaphore_create(0);

        // Attempt to use SCShareableContent to check for screen recording permission
        [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent * _Nullable content,
                                                                        NSError * _Nullable error) {
            Q_UNUSED(error);
            if (content) {
                // Successfully obtained content, indicating permission is granted
                hasPermission = YES;
            } else {
                // No permission or other error occurred
                hasPermission = NO;
            }
            // Notify the semaphore that the asynchronous task is complete
            dispatch_semaphore_signal(sema);
        }];

        // Wait for the asynchronous callback to complete
        dispatch_time_t timeout = dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC);
        dispatch_semaphore_wait(sema, timeout);

        // Return the final result
        return hasPermission;
    }
#endif
    return YES; // Return YES for macOS versions that do not support ScreenCaptureKit
}

- (void) toggleForegroundApp:(bool) foreground
{
    if (foreground) {
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    } else {
        [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    }
}

- (void) setWindowSecurity:(NSWindow*) window state:(bool) state
{
    [window setSharingType: state ? NSWindowSharingNone : NSWindowSharingReadOnly];
}

- (void) configureWindowAndHelpMenus:(QMainWindow*) mainWindow helpMenu:(QMenu*) helpMenu
{
    QMenu *qtWindowMenu = new QMenu(AppKit::tr("Window"));
    NSMenu *nsWindowMenu = qtWindowMenu->toNSMenu();

    QString minimizeStr = AppKit::tr("Minimize");
    [nsWindowMenu addItemWithTitle:minimizeStr.toNSString() action:@selector(performMiniaturize:) keyEquivalent:@""];
    QString zoomStr = AppKit::tr("Zoom");
    [nsWindowMenu addItemWithTitle:zoomStr.toNSString() action:@selector(performZoom:) keyEquivalent:@""];
    [nsWindowMenu addItem:[NSMenuItem separatorItem]];
    QString bringAllToFrontStr = AppKit::tr("Bring All to Front");
    [nsWindowMenu addItemWithTitle:bringAllToFrontStr.toNSString() action:@selector(arrangeInFront:) keyEquivalent:@""];

    NSApp.windowsMenu = nsWindowMenu;

    mainWindow->menuBar()->insertMenu(helpMenu->menuAction(), qtWindowMenu);

    NSApp.helpMenu = helpMenu->toNSMenu();
}

@end


//
// ------------------------- C++ Trampolines -------------------------
//

AppKit::AppKit(QObject* parent)
    : QObject(parent)
{
    self = [[AppKitImpl alloc] initWithObject:this];
}

AppKit::~AppKit()
{
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:static_cast<id>(self)];
    [[NSDistributedNotificationCenter defaultCenter] removeObserver:static_cast<id>(self)];
    [NSApp removeObserver:static_cast<id>(self) forKeyPath:@"effectiveAppearance"];
    [static_cast<id>(self) dealloc];
}

pid_t AppKit::lastActiveProcessId()
{
    return [static_cast<id>(self) lastActiveApplication].processIdentifier;
}

pid_t AppKit::activeProcessId()
{
    return [static_cast<id>(self) activeProcessId];
}

pid_t AppKit::ownProcessId()
{
    return [static_cast<id>(self) ownProcessId];
}

bool AppKit::activateProcess(pid_t pid)
{
    return [static_cast<id>(self) activateProcess:pid];
}

bool AppKit::hideProcess(pid_t pid)
{
    return [static_cast<id>(self) hideProcess:pid];
}

bool AppKit::isHidden(pid_t pid)
{
    return [static_cast<id>(self) isHidden:pid];
}

bool AppKit::isDarkMode()
{
    return [static_cast<id>(self) isDarkMode];
}

bool AppKit::isStatusBarDark()
{
    return [static_cast<id>(self) isStatusBarDark];
}


bool AppKit::enableAccessibility()
{
    return [static_cast<id>(self) enableAccessibility];
}

bool AppKit::enableScreenRecording()
{
    return [static_cast<id>(self) enableScreenRecording];
}

void AppKit::toggleForegroundApp(bool foreground)
{
    [static_cast<id>(self) toggleForegroundApp:foreground];
}

void AppKit::setWindowSecurity(QWindow* window, bool state)
{
    auto view = reinterpret_cast<NSView*>(window->winId());
    [static_cast<id>(self) setWindowSecurity:view.window state:state];
}

void AppKit::configureWindowAndHelpMenus(QMainWindow* window, QMenu* helpMenu)
{
    [static_cast<id>(self) configureWindowAndHelpMenus:window helpMenu:helpMenu];
}

// Common prefix for saved secrets
static const auto s_touchIdKeyPrefix = QStringLiteral("KeepassXC_Keys_");

// Convert macOS error codes to strings
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

// Report status errors if not successful
inline void LogStatusError(const char *message, OSStatus status)
{
   if (status) {
      std::string msg = StatusToErrorMessage(status);
      qWarning("%s: %s", message, msg.c_str());
   }
}

// Create an access control object to govern use of the saved secret
SecAccessControlRef createAccessControl(bool useTouchId)
{
    // We need both runtime and compile time checks here to solve the following problems:
    // - Not all flags are available in all OS versions, so we have to check it at compile time
    // - Requesting Biometry/TouchID/DevicePassword when no fingerprint sensor is available will result in runtime error
    SecAccessControlCreateFlags accessControlFlags = 0;

    // When TouchID is not enrolled and the flag is set, the method call fails with an error. 
    // We still want to set this flag if TouchID is enrolled but temporarily unavailable due to closed lid
    //
    // Sometimes, the enrolled-check does not work, LAErrorBiometryNotAvailable is returned instead of LAErrorBiometryNotEnrolled.
    // To fallback gracefully, we have to try to save the key a second time without this flag.

    if (useTouchId) {
#if XC_COMPILER_SUPPORT(APPLE_BIOMETRY)
        // This is the non-deprecated and preferred flag
        accessControlFlags = kSecAccessControlBiometryCurrentSet;
#elif XC_COMPILER_SUPPORT(TOUCH_ID)
        accessControlFlags = kSecAccessControlTouchIDCurrentSet;
#endif
    }

    // Add support for watch authentication if available
#if XC_COMPILER_SUPPORT(WATCH_UNLOCK)
    accessControlFlags = accessControlFlags | kSecAccessControlOr | kSecAccessControlWatch;
#endif

    // Check if password fallback is possible and add that as an option
#if XC_COMPILER_SUPPORT(TOUCH_ID)
   if (macUtils()->isAuthPolicyAvailable(MacUtils::AuthPolicy::PasswordFallback)) {
       accessControlFlags = accessControlFlags | kSecAccessControlOr | kSecAccessControlDevicePasscode;
   }
#endif

   CFErrorRef error = nullptr;
   auto sacObject = SecAccessControlCreateWithFlags(
       kCFAllocatorDefault, kSecAttrAccessibleWhenUnlockedThisDeviceOnly, accessControlFlags, &error);

    if (!sacObject || error) {
        auto e = static_cast<NSError*>(error);
        qWarning("MacUtils::saveSecret - Error creating security flags: %s", e.localizedDescription.UTF8String);
        return nullptr;
    }
    return sacObject;
}

bool MacUtils::saveSecret(const QString& key, const QByteArray& secretData) const
{
    const auto keyName = s_touchIdKeyPrefix + key;

    // Delete any existing entry since macOS does not allow overwrite
    if (!removeSecret(key)) {
        qWarning("MacUtils::saveSecret - Failed to remove existing secret for key '%s'", qPrintable(key));
    }

    // Add new entry
    auto keyBase64 = secretData.toBase64();
    auto keyValueData = CFDataCreateWithBytesNoCopy(
        kCFAllocatorDefault, reinterpret_cast<const UInt8*>(keyBase64.data()),
        keyBase64.length(), kCFAllocatorDefault);
    
    auto attributes = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(attributes, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(attributes, kSecAttrAccount, static_cast<CFStringRef>(keyName.toNSString()));
    CFDictionarySetValue(attributes, kSecValueData, keyValueData);
    CFDictionarySetValue(attributes, kSecAttrSynchronizable, kCFBooleanFalse);
    CFDictionarySetValue(attributes, kSecUseAuthenticationUI, kSecUseAuthenticationUIAllow);
    // First, attempt with TouchID enabled
    CFDictionarySetValue(attributes, kSecAttrAccessControl, createAccessControl(true));

    auto status = SecItemAdd(attributes, nullptr);
    if (status != errSecSuccess) {
        qDebug("MacUtils::saveSecret - Failed to save secret with TouchID enabled");
        // Try again without TouchID enabled
        CFDictionarySetValue(attributes, kSecAttrAccessControl, createAccessControl(false));
        status = SecItemAdd(attributes, nullptr);
        if (status != errSecSuccess) {
            qWarning("MacUtils::saveSecret - Failed to save secret to keystore");
        }
    }
    
    CFRelease(keyValueData);
    CFRelease(attributes);

    return status == errSecSuccess;
}

bool MacUtils::getSecret(const QString& key, QByteArray& secretData) const
{
    const auto keyName = s_touchIdKeyPrefix + key;
    secretData.clear();

    auto query = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrAccount, static_cast<CFStringRef>(keyName.toNSString()));
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);

    CFTypeRef dataTypeRef = nullptr;
    auto status = SecItemCopyMatching(query, &dataTypeRef);
    CFRelease(query);

    if (status == errSecUserCanceled) {
        // user canceled the authentication, return true with empty key
        return true;
    } else if (status != errSecSuccess || !dataTypeRef) {
        // TODO: Log failure
        return false;
    }

    auto valueData = static_cast<CFDataRef>(dataTypeRef);
    secretData = QByteArray::fromBase64(QByteArray(reinterpret_cast<const char*>(CFDataGetBytePtr(valueData)), 
                                        CFDataGetLength(valueData)));
    CFRelease(dataTypeRef);
    
    return !secretData.isEmpty();
}

bool MacUtils::removeSecret(const QString& key) const
{
    const auto keyName = s_touchIdKeyPrefix + key;
    auto query = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrAccount, static_cast<CFStringRef>(keyName.toNSString()));
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanFalse);
    // TODO: Log failure to delete?
    SecItemDelete(query);
    CFRelease(query);
    return true;
}

bool MacUtils::removeAllSecrets() const
{
    auto query = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecReturnAttributes, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitAll);

    CFTypeRef result = nullptr;
    auto status = SecItemCopyMatching(query, &result);
    if (status == errSecSuccess && result) {
        for (NSDictionary* item in static_cast<NSArray*>(result)) {
            NSString* account = item[static_cast<id>(kSecAttrAccount)];
            if (account && [account hasPrefix:s_touchIdKeyPrefix.toNSString()]) {
                auto delQuery = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
                CFDictionarySetValue(delQuery, kSecClass, kSecClassGenericPassword);
                CFDictionarySetValue(delQuery, kSecAttrAccount, static_cast<CFStringRef>(account));
                // TODO: Log failure to delete?
                SecItemDelete(delQuery);
                CFRelease(delQuery);
            }
        }
        CFRelease(result);
    }
    CFRelease(query);
    return true;
}

bool MacUtils::isAuthPolicyAvailable(AuthPolicy policy) const
{
    LAPolicy policyCode;
    switch (policy) {
        case AuthPolicy::TouchId:
            policyCode = LAPolicyDeviceOwnerAuthenticationWithBiometrics;
            break;
        case AuthPolicy::Watch:
            policyCode = LAPolicyDeviceOwnerAuthenticationWithWatch;
            break;
        case AuthPolicy::PasswordFallback:
            policyCode = LAPolicyDeviceOwnerAuthentication;
            break;
        default:
            return false;
    }

    @try {
        LAContext *context = [[LAContext alloc] init];
        NSError *error = nil;
        bool available = [context canEvaluatePolicy:policyCode error:&error];
        [context release];
        if (error) {
            qDebug("MacUtils::isPolicyAvailable - Policy not available: %s", error.localizedDescription.UTF8String);
        }
        return available;
    } @catch (NSException *exception) {
        qWarning("MacUtils::isPolicyAvailable - Exception occurred: %s", exception.reason.UTF8String);
        return false;
    }
}
