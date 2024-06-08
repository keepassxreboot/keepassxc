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

#include "DeviceListenerLibUsb.h"
#include "core/Tools.h"

#include <QPointer>
#include <QtConcurrent>
#include <QtGlobal>
#include <libusb.h>

DeviceListenerLibUsb::DeviceListenerLibUsb(QWidget* parent)
    : QObject(parent)
    , m_ctx(nullptr)
    , m_completed(false)
{
}

DeviceListenerLibUsb::~DeviceListenerLibUsb()
{
    if (m_ctx) {
        deregisterAllHotplugCallbacks();
        libusb_exit(static_cast<libusb_context*>(m_ctx));
        m_ctx = nullptr;
    }
}

namespace
{
    void handleUsbEvents(libusb_context* ctx, QAtomicInt* completed)
    {
        while (!*completed) {
            libusb_handle_events_completed(ctx, reinterpret_cast<int*>(completed));
            Tools::sleep(100);
        }
    }
} // namespace

DeviceListenerLibUsb::Handle
DeviceListenerLibUsb::registerHotplugCallback(bool arrived, bool left, int vendorId, int productId, const QUuid*)
{
    if (!m_ctx) {
        if (libusb_init(reinterpret_cast<libusb_context**>(&m_ctx)) != LIBUSB_SUCCESS) {
            qWarning("Unable to initialize libusb. USB devices may not be detected properly.");
            return 0;
        }
    }

    int events = 0;
    if (arrived) {
        events |= LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED;
    }
    if (left) {
        events |= LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT;
    }

    Handle handle = 0;
    auto* handleNative = reinterpret_cast<libusb_hotplug_callback_handle*>(&handle);
    const QPointer that = this;
    const int ret = libusb_hotplug_register_callback(
        static_cast<libusb_context*>(m_ctx),
        static_cast<libusb_hotplug_event>(events),
        static_cast<libusb_hotplug_flag>(0),
        vendorId,
        productId,
        LIBUSB_HOTPLUG_MATCH_ANY,
        [](libusb_context* ctx, libusb_device* device, libusb_hotplug_event event, void* userData) -> int {
            if (!ctx) {
                return true;
            }
            emit static_cast<DeviceListenerLibUsb*>(userData)->devicePlugged(
                event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, ctx, device);
            return false;
        },
        that,
        handleNative);
    if (ret != LIBUSB_SUCCESS) {
        qWarning("Failed to register USB listener callback.");
        handle = 0;
    }

    if (m_completed && m_usbEvents.isRunning()) {
        // Avoid race conditions
        m_usbEvents.waitForFinished();
    }
    if (handle > 0) {
        m_callbackHandles.insert(handle);
        if (!m_usbEvents.isRunning()) {
            m_completed = false;
            m_usbEvents = QtConcurrent::run(handleUsbEvents, static_cast<libusb_context*>(m_ctx), &m_completed);
        }
    }
    return handle;
}

void DeviceListenerLibUsb::deregisterHotplugCallback(Handle handle)
{
    if (!m_ctx || !m_callbackHandles.contains(handle)) {
        return;
    }
#ifdef Q_OS_FREEBSD
    auto* handleNative = reinterpret_cast<libusb_hotplug_callback_handle>(handle);
#else
    auto handleNative = static_cast<libusb_hotplug_callback_handle>(handle);
#endif
    libusb_hotplug_deregister_callback(static_cast<libusb_context*>(m_ctx), handleNative);
    m_callbackHandles.remove(handle);

    if (m_callbackHandles.isEmpty() && m_usbEvents.isRunning()) {
        m_completed = true;
        m_usbEvents.waitForFinished();
    }
}

void DeviceListenerLibUsb::deregisterAllHotplugCallbacks()
{
    while (!m_callbackHandles.isEmpty()) {
        deregisterHotplugCallback(*m_callbackHandles.constBegin());
    }
}
