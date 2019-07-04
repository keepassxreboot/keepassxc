/*
 *  Copyright (C) 2016 Lennart Glauer <mail@lennart-glauer.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#import <AppKit/NSWorkspace.h>
#import <Availability.h>

#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101200
static const NSEventMask NSEventMaskKeyDown = NSKeyDownMask;
#endif

@implementation AppKitImpl

- (id) initWithObject:(AppKit*)appkit
{
    self = [super init];
    if (self) {
        m_appkit = appkit;
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:static_cast<id>(self)
                                                           selector:@selector(didDeactivateApplicationObserver:)
                                                               name:NSWorkspaceDidDeactivateApplicationNotification
                                                             object:nil];
    
        [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:static_cast<id>(self)
                                                            selector:@selector(userSwitchHandler:)
                                                                name:NSWorkspaceSessionDidResignActiveNotification
                                                                object:nil];
    }
    return self;
}

//
// Update last active application property
//
- (void) didDeactivateApplicationObserver:(NSNotification*) notification
{
    NSDictionary* userInfo = notification.userInfo;
    NSRunningApplication* app = userInfo[NSWorkspaceApplicationKey];

    if (app.processIdentifier != [self ownProcessId]) {
        self.lastActiveApplication = app;
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
    NSDictionary* dict = [[NSUserDefaults standardUserDefaults] persistentDomainForName:NSGlobalDomain];
    id style = [dict objectForKey:@"AppleInterfaceStyle"];
    return ( style && [style isKindOfClass:[NSString class]]
             && NSOrderedSame == [style caseInsensitiveCompare:@"dark"] );
}

//
// Notification for user switch
//
- (void) userSwitchHandler:(NSNotification*) notification
{
    if ([[notification name] isEqualToString:NSWorkspaceSessionDidResignActiveNotification] && m_appkit)
    {
        emit m_appkit->lockDatabases();
    }
}

//
// Add global event monitor
//
- (id) addGlobalMonitor:(NSEventMask) mask handler:(void (^)(NSEvent*)) handler
{
    return [NSEvent addGlobalMonitorForEventsMatchingMask:mask handler:handler];
}

//
// Remove global event monitor
//
- (void) removeGlobalMonitor:(id) monitor
{
    [NSEvent removeMonitor:monitor];
}

//
// Check if accessibility is enabled, may show an popup asking for permissions
//
- (bool) enableAccessibility
{
    NSDictionary* opts = @{static_cast<id>(kAXTrustedCheckOptionPrompt): @YES};
    return AXIsProcessTrustedWithOptions(static_cast<CFDictionaryRef>(opts));
}

@end

//
// ------------------------- C++ Trampolines -------------------------
//

AppKit::AppKit(QObject* parent) : QObject(parent)
{
    self = [[AppKitImpl alloc] initWithObject:this];
}

AppKit::~AppKit()
{
    [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:static_cast<id>(self)];
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

void* AppKit::addGlobalMonitor(CGKeyCode keycode, CGEventFlags modifier, void* userData, void (*handler)(void*))
{
    return [static_cast<id>(self) addGlobalMonitor:NSEventMaskKeyDown handler:^(NSEvent* event) {
        if (event.keyCode == keycode && (event.modifierFlags & modifier) == modifier) {
            handler(userData);
        }
    }];
}

void AppKit::removeGlobalMonitor(void* monitor)
{
    [static_cast<id>(self) removeGlobalMonitor:static_cast<id>(monitor)];
}

bool AppKit::enableAccessibility()
{
    return [static_cast<id>(self) enableAccessibility];
}
