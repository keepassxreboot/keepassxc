/*
 *  Copyright (C) 2026 Frederic Ruget <fred@atlant.is>
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

#include "DeviceStorage.h"

#include "config-keepassx.h"

#include <QCoreApplication>
#include <QThread>

#ifdef KPXC_FEATURE_YUBIBLOB
#include <yubiblob.h>
#endif

namespace
{
    const QString kScheme = QStringLiteral("yubikey:");

    QString tr(const char* text)
    {
        return QCoreApplication::translate("DeviceStorage", text);
    }

    /// The vault's name without its `.kdbx` extension.
    QString stem(const QString& blob)
    {
        return blob.endsWith(".kdbx") ? blob.chopped(5) : blob;
    }

#ifdef KPXC_FEATURE_YUBIBLOB
    static_assert(DeviceStorage::Ok == YUBIBLOB_OK && DeviceStorage::NotFound == YUBIBLOB_ERR_NOT_FOUND
                      && DeviceStorage::StoreFull == YUBIBLOB_ERR_STORE_FULL
                      && DeviceStorage::CardFull == YUBIBLOB_ERR_CARD_FULL
                      && DeviceStorage::WrongPin == YUBIBLOB_ERR_WRONG_PIN
                      && DeviceStorage::SeveralDevices == YUBIBLOB_ERR_SEVERAL_DEVICES
                      && DeviceStorage::Internal == YUBIBLOB_ERR_INTERNAL,
                  "DeviceStorage::Status mirrors libyubiblob's status codes");

    /// The device side on libyubiblob: one session per operation, so that a
    /// YubiKey unplugged and plugged back in between is simply found again.
    class YubiblobBackend : public DeviceStorage::Backend
    {
    public:
        YubiblobBackend()
        {
            // An incompatible library fails every call, saying so.
            m_compatible = yubiblob_abi_version() == YUBIBLOB_ABI_VERSION;
        }

        DeviceStorage::Status
        list(const QString& serial, QList<DeviceStorage::BlobInfo>* blobs, QString* error) override
        {
            Session s;
            auto status = open(serial, &s, error);
            if (status != DeviceStorage::Ok) {
                return status;
            }
            yubiblob_blob_info_t* found = nullptr;
            size_t count = 0;
            status = check(yubiblob_list(s.handle, &found, &count), error);
            if (status != DeviceStorage::Ok) {
                return status;
            }
            blobs->clear();
            for (size_t i = 0; i < count; ++i) {
                const auto& b = found[i];
                blobs->append({QString::fromUtf8(b.name), b.size, b.mtime, b.generation, b.encrypted});
            }
            yubiblob_blobs_free(found, count);
            return DeviceStorage::Ok;
        }

        DeviceStorage::Status fetch(const QString& serial,
                                    const QString& name,
                                    const QByteArray& pin,
                                    QByteArray* data,
                                    QString* error,
                                    int* pinTries) override
        {
            Session s;
            auto status = open(serial, &s, error);
            if (status != DeviceStorage::Ok) {
                return status;
            }
            uint8_t* content = nullptr;
            size_t len = 0;
            const auto blob = name.toUtf8();
            status = check(yubiblob_fetch(s.handle,
                                          blob.constData(),
                                          reinterpret_cast<const uint8_t*>(pin.constData()),
                                          static_cast<size_t>(pin.size()),
                                          &content,
                                          &len),
                           error,
                           pinTries);
            if (status == DeviceStorage::Ok) {
                *data = QByteArray(reinterpret_cast<const char*>(content), static_cast<qsizetype>(len));
                yubiblob_buffer_free(content, len);
            }
            return status;
        }

        DeviceStorage::Status store(const QString& serial,
                                    const QString& name,
                                    const QByteArray& data,
                                    const QByteArray& pin,
                                    QString* error,
                                    int* pinTries) override
        {
            Session s;
            auto status = open(serial, &s, error);
            if (status != DeviceStorage::Ok) {
                return status;
            }
            const auto blob = name.toUtf8();
            // A vault is already compressed and encrypted by KeePassXC;
            // yb encrypts it again, to the YubiKey's key.
            return check(yubiblob_store(s.handle,
                                        blob.constData(),
                                        reinterpret_cast<const uint8_t*>(data.constData()),
                                        static_cast<size_t>(data.size()),
                                        YUBIBLOB_STORE_ENCRYPT,
                                        reinterpret_cast<const uint8_t*>(pin.constData()),
                                        static_cast<size_t>(pin.size())),
                         error,
                         pinTries);
        }

    private:
        struct Session
        {
            yubiblob_session_t* handle = nullptr;
            ~Session()
            {
                yubiblob_close(handle);
            }
        };

        DeviceStorage::Status open(const QString& serial, Session* s, QString* error) const
        {
            if (!m_compatible) {
                *error = tr("This libyubiblob (%1) is not the version KeePassXC was built with.")
                             .arg(QString::fromUtf8(yubiblob_version()));
                return DeviceStorage::NotAvailable;
            }
            return check(yubiblob_open(serial.toUInt(), 0, &s->handle), error);
        }

        static DeviceStorage::Status check(yubiblob_status_t status, QString* error, int* pinTries = nullptr)
        {
            if (status != YUBIBLOB_OK) {
                *error = QString::fromUtf8(yubiblob_last_error());
                if (pinTries) {
                    *pinTries = yubiblob_last_pin_tries();
                }
            }
            return static_cast<DeviceStorage::Status>(status);
        }

        bool m_compatible = false;
    };
#else
    /// Built without libyubiblob: every operation says so.
    class UnavailableBackend : public DeviceStorage::Backend
    {
    public:
        DeviceStorage::Status list(const QString&, QList<DeviceStorage::BlobInfo>*, QString* error) override
        {
            return unavailable(error);
        }
        DeviceStorage::Status
        fetch(const QString&, const QString&, const QByteArray&, QByteArray*, QString* error, int*) override
        {
            return unavailable(error);
        }
        DeviceStorage::Status
        store(const QString&, const QString&, const QByteArray&, const QByteArray&, QString* error, int*) override
        {
            return unavailable(error);
        }

    private:
        static DeviceStorage::Status unavailable(QString* error)
        {
            *error = tr("This KeePassXC was built without YubiKey storage (libyubiblob).");
            return DeviceStorage::NotAvailable;
        }
    };
#endif

    std::unique_ptr<DeviceStorage::Backend> defaultBackend()
    {
#ifdef KPXC_FEATURE_YUBIBLOB
        return std::make_unique<YubiblobBackend>();
#else
        return std::make_unique<UnavailableBackend>();
#endif
    }
} // namespace

DeviceStorage::DeviceStorage()
    : m_backend(defaultBackend())
{
}

DeviceStorage* DeviceStorage::instance()
{
    static DeviceStorage storage;
    return &storage;
}

bool DeviceStorage::isDeviceLocation(const QString& location)
{
    return location.startsWith(kScheme);
}

QString DeviceStorage::location(const QString& serial, const QString& blob)
{
    return kScheme + serial + "/" + blob;
}

bool DeviceStorage::parse(const QString& location, QString* serial, QString* blob)
{
    if (!isDeviceLocation(location)) {
        return false;
    }
    auto rest = location.mid(kScheme.size());
    auto slash = rest.indexOf('/');
    if (slash < 0 || slash == rest.size() - 1) {
        return false;
    }
    *serial = rest.left(slash);
    *blob = rest.mid(slash + 1);
    return true;
}

QString DeviceStorage::describe(Status status, const QString& detail, const QString& blob, int pinTries) const
{
    switch (status) {
    case NotFound:
        return tr("%1 is not on the YubiKey.").arg(blob);
    case StoreFull:
        return tr("Not enough room on the YubiKey to save %1: it needs room for a second copy while saving.").arg(blob);
    case CardFull:
        return tr("The YubiKey's memory ran out while saving %1: the previous version is kept. Make room on "
                  "the YubiKey, then save again.")
            .arg(blob);
    case WrongPin:
        return pinTries >= 0 ? tr("Wrong PIN (%1 tries left).").arg(pinTries) : tr("Wrong PIN.");
    case PinBlocked:
        return tr("The YubiKey's PIN is blocked; unblock it with the PUK (e.g. ykman piv access unblock-pin).");
    case NoDevice:
        return tr("No YubiKey found.");
    case SeveralDevices:
        return tr("Several YubiKeys are connected: choose one by its serial number.");
    case NoStore:
        return tr("This YubiKey is not set up for yb (yb format).");
    case Corrupted:
        return tr("%1 on the YubiKey is corrupted or was tampered with: it was not read.").arg(blob);
    default:
        return detail;
    }
}

bool DeviceStorage::pinFor(const QString& serial, QByteArray* pin, QString* error)
{
    if (!m_pins.contains(serial)) {
        QString entered;
        if (m_pinPrompt) {
            // Saves run on a worker thread; the prompt is GUI, so it runs on
            // the GUI thread (which runs an event loop while it waits).
            auto* app = QCoreApplication::instance();
            if (app && QThread::currentThread() != app->thread()) {
                QMetaObject::invokeMethod(app, [&] { entered = m_pinPrompt(serial); }, Qt::BlockingQueuedConnection);
            } else {
                entered = m_pinPrompt(serial);
            }
        }
        if (entered.isNull()) {
            *error = tr("The YubiKey's PIN is needed.");
            return false;
        }
        setPin(serial, entered);
    }
    *pin = m_pins.value(serial);
    return true;
}

bool DeviceStorage::find(const QString& serial, const QString& blob, BlobInfo* info, Status* status, QString* error)
{
    QList<BlobInfo> blobs;
    QString detail;
    *status = m_backend->list(serial, &blobs, &detail);
    if (*status != Ok) {
        *error = describe(*status, detail, blob, -1);
        return false;
    }
    for (const auto& b : blobs) {
        if (b.name == blob) {
            *info = b;
            return true;
        }
    }
    *status = NotFound;
    *error = describe(NotFound, {}, blob, -1);
    return false;
}

QByteArray DeviceStorage::fingerprint(const QString& serial, const QString& blob, QString* error)
{
    BlobInfo info;
    Status status;
    if (!find(serial, blob, &info, &status, error)) {
        return {};
    }
    return QByteArray::number(info.generation) + ":" + QByteArray::number(info.size);
}

bool DeviceStorage::fetch(const QString& serial, const QString& blob, QByteArray* data, QString* error)
{
    QByteArray pin;
    if (!pinFor(serial, &pin, error)) {
        return false;
    }
    QString detail;
    int tries = -1;
    auto status = m_backend->fetch(serial, blob, pin, data, &detail, &tries);
    pin.fill('\0');
    if (status != Ok) {
        if (status == WrongPin || status == PinBlocked) {
            forgetPin(serial);
        }
        *error = describe(status, detail, blob, tries);
        return false;
    }
    return true;
}

bool DeviceStorage::read(const QString& location, QByteArray* data, QString* error, bool mayAskPin)
{
    QString serial, blob;
    if (!parse(location, &serial, &blob)) {
        *error = tr("Not a device location: %1").arg(location);
        return false;
    }
    auto current = fingerprint(serial, blob, error);
    if (current.isEmpty()) {
        return false;
    }
    auto cached = m_cache.constFind(location);
    if (cached != m_cache.constEnd() && cached->fingerprint == current) {
        *data = cached->data;
        return true;
    }
    if (!mayAskPin && !m_pins.contains(serial)) {
        *error = tr("%1 is read from the YubiKey when unlocking.").arg(blob);
        return false;
    }
    if (!fetch(serial, blob, data, error)) {
        return false;
    }
    m_cache.insert(location, {*data, current});
    return true;
}

bool DeviceStorage::write(const QString& location, const QByteArray& data, QString* error)
{
    QString serial, blob;
    if (!parse(location, &serial, &blob)) {
        *error = tr("Not a device location: %1").arg(location);
        return false;
    }
    QByteArray pin;
    if (!pinFor(serial, &pin, error)) {
        return false;
    }
    QString detail;
    int tries = -1;
    auto status = m_backend->store(serial, blob, data, pin, &detail, &tries);
    pin.fill('\0');
    if (status != Ok) {
        if (status == WrongPin || status == PinBlocked) {
            forgetPin(serial);
        }
        *error = describe(status, detail, blob, tries);
        return false;
    }
    QString ignored;
    m_cache.insert(location, {data, fingerprint(serial, blob, &ignored)});
    return true;
}

QStringList DeviceStorage::listVaults(const QString& serial, QString* error, Status* status)
{
    QList<BlobInfo> blobs;
    QString detail;
    auto result = m_backend->list(serial, &blobs, &detail);
    if (status) {
        *status = result;
    }
    if (result != Ok) {
        *error = describe(result, detail, {}, -1);
        return {};
    }
    QStringList vaults;
    for (const auto& b : blobs) {
        if (b.name.endsWith(".kdbx")) {
            vaults << b.name;
        }
    }
    vaults.sort();
    return vaults;
}

bool DeviceStorage::hasStoredPassword(const QString& location)
{
    QString serial, blob, error;
    BlobInfo info;
    Status status;
    return parse(location, &serial, &blob) && find(serial, stem(blob) + ".passwd", &info, &status, &error);
}

bool DeviceStorage::readPassword(const QString& location, QString* password, QString* error)
{
    QString serial, blob;
    if (!parse(location, &serial, &blob)) {
        *error = tr("Not a device location: %1").arg(location);
        return false;
    }
    const auto passwd = stem(blob) + ".passwd";
    BlobInfo info;
    Status status;
    if (!find(serial, passwd, &info, &status, error)) {
        if (status == NotFound) {
            // No stored master password: nothing to read.
            error->clear();
            password->clear();
            return true;
        }
        return false;
    }
    QByteArray bytes;
    if (!fetch(serial, passwd, &bytes, error)) {
        return false;
    }
    // A password typed into `yb store` may end with a newline.
    if (bytes.endsWith('\n')) {
        bytes.chop(1);
    }
    *password = QString::fromUtf8(bytes);
    bytes.fill('\0');
    return true;
}

void DeviceStorage::setPin(const QString& serial, const QString& pin)
{
    forgetPin(serial);
    m_pins.insert(serial, pin.toUtf8());
}

void DeviceStorage::forgetPin(const QString& serial)
{
    auto it = m_pins.find(serial);
    if (it != m_pins.end()) {
        it->fill('\0');
        m_pins.erase(it);
    }
}

void DeviceStorage::setBackend(std::unique_ptr<Backend> backend)
{
    m_backend = backend ? std::move(backend) : defaultBackend();
}

void DeviceStorage::setPinPrompt(PinPrompt prompt)
{
    m_pinPrompt = std::move(prompt);
}

void DeviceStorage::clearCache()
{
    m_cache.clear();
}
