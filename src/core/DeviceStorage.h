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

#ifndef KEEPASSXC_DEVICESTORAGE_H
#define KEEPASSXC_DEVICESTORAGE_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <functional>
#include <memory>

/**
 * Databases stored on a device instead of a file.
 *
 * A device database is named by a location, `yubikey:<serial>/<vault>.kdbx`
 * (`yubikey:/<vault>.kdbx` for the only YubiKey connected), used wherever
 * KeePassXC uses a file path as an identity.  Its bytes go between the
 * device and memory: no copy on disk.  The YubiKey is reached through
 * libyubiblob (https://github.com/douzebis/yb), which keeps the vault in the
 * YubiKey's PIV application, encrypted to a key that never leaves it, and
 * replaces it safely (an interrupted write keeps the previous vault).
 *
 * Reads are served from a cache of the encrypted bytes, checked against the
 * device without the PIN; writes go to the device at once (write-through).
 */
class DeviceStorage
{
public:
    /// Outcomes, as libyubiblob's status codes (yb spec 0039).
    enum Status
    {
        Ok = 0,
        NotFound = 1,
        StoreFull = 2,
        CardFull = 3,
        WrongPin = 4,
        PinBlocked = 5,
        NoDevice = 6,
        SeveralDevices = 7,
        NoStore = 8,
        CardRemoved = 9,
        Corrupted = 10,
        DefaultCredentials = 11,
        InvalidArgument = 12,
        Internal = 13,
        /// KeePassXC built without libyubiblob.
        NotAvailable = -1,
    };

    /// A blob on the device, as listed without the PIN.
    struct BlobInfo
    {
        QString name;
        quint32 size = 0;
        quint32 mtime = 0;
        /// Changes with every write of the blob.
        quint32 generation = 0;
        bool encrypted = false;
    };

    /// The device side: libyubiblob, or a stand-in in tests.  `serial` is
    /// empty for the only YubiKey connected.  On failure, `error` says why
    /// (for people) and `pinTries`, after a wrong PIN, the tries left.
    class Backend
    {
    public:
        virtual ~Backend() = default;
        virtual Status list(const QString& serial, QList<BlobInfo>* blobs, QString* error) = 0;
        virtual Status fetch(const QString& serial,
                             const QString& name,
                             const QByteArray& pin,
                             QByteArray* data,
                             QString* error,
                             int* pinTries) = 0;
        virtual Status store(const QString& serial,
                             const QString& name,
                             const QByteArray& data,
                             const QByteArray& pin,
                             QString* error,
                             int* pinTries) = 0;
    };

    /// Asks the user for the PIN of the YubiKey `serial` (empty: the only
    /// one connected); a null string cancels.
    using PinPrompt = std::function<QString(const QString& serial)>;

    static DeviceStorage* instance();

    static bool isDeviceLocation(const QString& location);
    static QString location(const QString& serial, const QString& blob);
    /// Splits a location into the YubiKey's serial and the blob name.
    static bool parse(const QString& location, QString* serial, QString* blob);

    /// The vault's bytes: from the cache if the device still holds them,
    /// otherwise fetched (with the PIN).  With `mayAskPin` false (KeePassXC
    /// reading only the public headers, e.g. restoring tabs at start-up),
    /// nothing is fetched unless the PIN is already known.
    bool read(const QString& location, QByteArray* data, QString* error, bool mayAskPin = true);
    /// Store `data` on the device (with the PIN); updates the cache.
    bool write(const QString& location, const QByteArray& data, QString* error);

    /// The vaults (`*.kdbx` blobs) on a YubiKey; no PIN needed.  On
    /// failure, `error` is set, and `status` says why.
    QStringList listVaults(const QString& serial, QString* error, Status* status = nullptr);
    /// The vault's stored master password (`<vault>.passwd`), if any.
    bool readPassword(const QString& location, QString* password, QString* error);
    /// Whether the YubiKey holds the vault's master password; no PIN needed.
    bool hasStoredPassword(const QString& location);

    void setPin(const QString& serial, const QString& pin);
    /// Forget the PIN of a YubiKey, overwriting it in memory.
    void forgetPin(const QString& serial);

    /// Replace the device side (tests); nullptr restores libyubiblob.
    void setBackend(std::unique_ptr<Backend> backend);
    void setPinPrompt(PinPrompt prompt);
    /// Drop cached bytes (tests).
    void clearCache();

private:
    DeviceStorage();

    struct CacheEntry
    {
        QByteArray data;
        /// The vault's generation and size: detects a change on the
        /// device without the PIN.
        QByteArray fingerprint;
    };

    bool pinFor(const QString& serial, QByteArray* pin, QString* error);
    bool find(const QString& serial, const QString& blob, BlobInfo* info, Status* status, QString* error);
    QByteArray fingerprint(const QString& serial, const QString& blob, QString* error);
    bool fetch(const QString& serial, const QString& blob, QByteArray* data, QString* error);
    QString describe(Status status, const QString& detail, const QString& blob, int pinTries) const;

    std::unique_ptr<Backend> m_backend;
    PinPrompt m_pinPrompt;
    QHash<QString, QByteArray> m_pins;
    QHash<QString, CacheEntry> m_cache;
};

#endif // KEEPASSXC_DEVICESTORAGE_H
