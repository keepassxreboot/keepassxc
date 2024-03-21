/*
 * Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 or (at your option)
 * version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DeviceListenerMac.h"

#include <QPointer>
#include <IOKit/IOKitLib.h>

DeviceListenerMac::DeviceListenerMac(QObject* parent)
    : QObject(parent)
    , m_mgr(nullptr)
{
}

DeviceListenerMac::~DeviceListenerMac()
{
    if (m_mgr) {
        IOHIDManagerUnscheduleFromRunLoop(m_mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerClose(m_mgr, kIOHIDOptionsTypeNone);
        CFRelease(m_mgr);
    }
}

void DeviceListenerMac::registerHotplugCallback(bool arrived, bool left, int vendorId, int productId, const QUuid*)
{
    if (!m_mgr) {
        m_mgr = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone);
        if (!m_mgr) {
            qWarning("Failed to create IOHIDManager.");
            return;
        }
        IOHIDManagerScheduleWithRunLoop(m_mgr, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    }

    if (vendorId > 0 || productId > 0) {
        CFMutableDictionaryRef matchingDict = IOServiceMatching(kIOHIDDeviceKey);
        if (vendorId > 0) {
            auto vid = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vendorId);
            CFDictionaryAddValue(matchingDict, CFSTR(kIOHIDVendorIDKey), vid);
            CFRelease(vid);
        }
        if (productId > 0) {
            auto pid = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &vendorId);
            CFDictionaryAddValue(matchingDict, CFSTR(kIOHIDProductIDKey), pid);
            CFRelease(pid);
        }
        IOHIDManagerSetDeviceMatching(m_mgr, matchingDict);
        CFRelease(matchingDict);
    } else {
        IOHIDManagerSetDeviceMatching(m_mgr, nullptr);
    }

    QPointer that = this;
    if (arrived) {
        IOHIDManagerRegisterDeviceMatchingCallback(m_mgr, [](void* ctx, IOReturn, void*, IOHIDDeviceRef device) {
            static_cast<DeviceListenerMac*>(ctx)->onDeviceStateChanged(true, device);
        }, that);
    }
    if (left) {
        IOHIDManagerRegisterDeviceRemovalCallback(m_mgr, [](void* ctx, IOReturn, void*, IOHIDDeviceRef device) {
            static_cast<DeviceListenerMac*>(ctx)->onDeviceStateChanged(true, device);
        }, that);
    }

    if (IOHIDManagerOpen(m_mgr, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
        qWarning("Could not open enumerated devices.");
    }
}

void DeviceListenerMac::deregisterHotplugCallback()
{
    if (m_mgr) {
        IOHIDManagerRegisterDeviceMatchingCallback(m_mgr, nullptr, this);
        IOHIDManagerRegisterDeviceRemovalCallback(m_mgr, nullptr, this);
    }
}

void DeviceListenerMac::onDeviceStateChanged(bool state, void* device)
{
    emit devicePlugged(state, m_mgr, device);
}
