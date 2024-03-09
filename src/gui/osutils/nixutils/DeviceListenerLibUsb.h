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

#ifndef DEVICELISTENER_LIBUSB_H
#define DEVICELISTENER_LIBUSB_H

#define DEVICELISTENER_IMPL DeviceListenerLibUsb

#include <QAtomicInt>
#include <QFuture>
#include <QSet>
#include <QWidget>

class QUuid;

class DeviceListenerLibUsb : public QObject
{
    Q_OBJECT

public:
    explicit DeviceListenerLibUsb(QWidget* parent);
    DeviceListenerLibUsb(const DeviceListenerLibUsb&) = delete;
    ~DeviceListenerLibUsb() override;

    int registerHotplugCallback(bool arrived, bool left, int vendorId = -1, int productId = -1, const QUuid* = nullptr);
    void deregisterHotplugCallback(int handle);
    void deregisterAllHotplugCallbacks();

signals:
    void devicePlugged(bool state, void* ctx, void* device);

private:
    void* m_ctx;
    QSet<int> m_callbackHandles;
    QFuture<void> m_usbEvents;
    QAtomicInt m_completed;
};

#endif // DEVICELISTENER_LIBUSB_H
