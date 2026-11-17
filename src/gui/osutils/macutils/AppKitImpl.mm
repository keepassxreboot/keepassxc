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
#import <QWindow>
#import <QMenu>
#import <QMenuBar>
#import <Cocoa/Cocoa.h>
#if __clang_major__ >= 13 && MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_3
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#endif

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
        // Skip while updating OLED window chrome to avoid applyTheme re-entrancy
        if (self.oledNativeChromeBusy) {
            return;
        }
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

// Dark (OLED) theme: pure-black titled windows; reversible when leaving OLED.
//
// Cross-macOS safety:
// - Never set NSApp.appearance (KVO → applyTheme recursion / crash on several releases)
// - Use @available / respondsToSelector; soft-fail so the Qt OLED theme still works
// - Only style ordinary titled windows (not HUD/panel/tooltips)
// - Apply on main queue after windows exist (avoids races on older Qt/macOS)
//
- (BOOL) oledChromeSupported
{
    // Transparent title bar needs 10.10+; DarkAqua appearance 10.14+.
    // KeePassXC targets modern macOS; if unavailable, leave system chrome alone.
    if (@available(macOS 10.14, *)) {
        return YES;
    }
    return NO;
}

- (BOOL) shouldStyleWindowForOled:(NSWindow*) window
{
    if (!window) {
        return NO;
    }
    // Titled document-style windows / dialogs only
    if ((window.styleMask & NSWindowStyleMaskTitled) == 0) {
        return NO;
    }
    // Skip sheets and floating utility panels — chrome APIs differ by release
    if (window.isSheet) {
        return NO;
    }
    if ([window isKindOfClass:[NSPanel class]]) {
        NSPanel* panel = static_cast<NSPanel*>(window);
        if (panel.floatingPanel || panel.becomesKeyOnlyIfNeeded) {
            return NO;
        }
    }
    return YES;
}

- (void) setOledChromeEnabled:(bool) enabled
{
    // Always record desired state so a in-flight async pass applies the latest value
    const BOOL wantEnabled = enabled ? YES : NO;
    self.oledNativeChromeDesired = wantEnabled;

    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
    [nc removeObserver:self name:NSWindowDidBecomeKeyNotification object:nil];
    [nc removeObserver:self name:NSWindowDidBecomeMainNotification object:nil];

    // Soft no-op for native chrome on unsupported OS; Qt OLED theme still applies
    if (![self oledChromeSupported]) {
        return;
    }

    if (wantEnabled) {
        [nc addObserver:self
               selector:@selector(oledWindowDidBecomeKey:)
                   name:NSWindowDidBecomeKeyNotification
                 object:nil];
        [nc addObserver:self
               selector:@selector(oledWindowDidBecomeKey:)
                   name:NSWindowDidBecomeMainNotification
                 object:nil];
    }

    // Already scheduled a pass — it will read the latest oledNativeChromeDesired
    if (self.oledNativeChromeBusy) {
        return;
    }
    self.oledNativeChromeBusy = YES;

    // Defer to next main-queue turn so NSWindows exist on all macOS + Qt combos.
    // Strong self is fine under MRC: AppKitImpl lives for the process.
    AppKitImpl* strongSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        @try {
            for (NSWindow* window in NSApp.windows) {
                if (strongSelf.oledNativeChromeDesired) {
                    [strongSelf applyOledWindowChrome:window];
                } else {
                    [strongSelf clearOledWindowChrome:window];
                }
            }
        } @catch (NSException* ex) {
            NSLog(@"KeePassXC: OLED window chrome update failed: %@", ex);
        }
        strongSelf.oledNativeChromeBusy = NO;
    });
}

- (void) oledWindowDidBecomeKey:(NSNotification*) notification
{
    if (!self.oledNativeChromeDesired || self.oledNativeChromeBusy) {
        return;
    }
    id obj = notification.object;
    if ([obj isKindOfClass:[NSWindow class]]) {
        [self applyOledWindowChrome:obj];
    }
}

- (void) applyOledWindowChrome:(NSWindow*) window
{
    if (!self.oledNativeChromeDesired || ![self oledChromeSupported]) {
        return;
    }
    if (![self shouldStyleWindowForOled:window]) {
        return;
    }

    @try {
        // Per-window dark chrome only (never NSApp.appearance)
        if (@available(macOS 10.14, *)) {
            NSAppearance* dark = [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
            if (dark != nil) {
                window.appearance = dark;
            }
        }

        // Transparent title bar over pure black content (10.10+)
        if (@available(macOS 10.10, *)) {
            if ([window respondsToSelector:@selector(setTitlebarAppearsTransparent:)]) {
                window.titlebarAppearsTransparent = YES;
            }
            if ([window respondsToSelector:@selector(setTitleVisibility:)]) {
                // Keep title text; hiding it is unnecessary and varies by release
                window.titleVisibility = NSWindowTitleVisible;
            }
        }

        if ([window respondsToSelector:@selector(setBackgroundColor:)]) {
            window.backgroundColor = [NSColor blackColor];
        }
        // Do not set FullSizeContentView — breaks Qt toolbar layout on multiple releases
    } @catch (NSException* ex) {
        NSLog(@"KeePassXC: applyOledWindowChrome failed: %@", ex);
    }
}

- (void) clearOledWindowChrome:(NSWindow*) window
{
    if (![self shouldStyleWindowForOled:window]) {
        return;
    }

    @try {
        if (@available(macOS 10.14, *)) {
            window.appearance = nil; // follow system again
        }
        if (@available(macOS 10.10, *)) {
            if ([window respondsToSelector:@selector(setTitlebarAppearsTransparent:)]) {
                window.titlebarAppearsTransparent = NO;
            }
            if ([window respondsToSelector:@selector(setTitleVisibility:)]) {
                window.titleVisibility = NSWindowTitleVisible;
            }
        }
        if ([window respondsToSelector:@selector(setBackgroundColor:)]) {
            // System document background — correct light/dark adaptive color on 10.14+
            if (@available(macOS 10.14, *)) {
                window.backgroundColor = [NSColor windowBackgroundColor];
            } else {
                window.backgroundColor = [NSColor windowBackgroundColor];
            }
        }
    } @catch (NSException* ex) {
        NSLog(@"KeePassXC: clearOledWindowChrome failed: %@", ex);
    }
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
    [[NSNotificationCenter defaultCenter] removeObserver:static_cast<id>(self)];
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

void AppKit::setOledChromeEnabled(bool enabled)
{
    [static_cast<id>(self) setOledChromeEnabled:enabled];
}

void AppKit::applyOledWindowChrome(QWindow* window)
{
    if (!window) {
        return;
    }
    auto view = reinterpret_cast<NSView*>(window->winId());
    if (view && view.window) {
        [static_cast<id>(self) applyOledWindowChrome:view.window];
    }
}
