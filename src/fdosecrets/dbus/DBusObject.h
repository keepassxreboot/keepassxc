/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETS_DBUSOBJECT_H
#define KEEPASSXC_FDOSECRETS_DBUSOBJECT_H

#include "DBusConstants.h"
#include "DBusMgr.h"
#include "DBusTypes.h"

#include <QDBusError>

#ifndef Q_MOC_RUN
// define the tag text as empty, so the compiler doesn't see it
#define DBUS_PROPERTY
#endif // #ifndef Q_MOC_RUN

namespace FdoSecrets
{
    class Service;

    /**
     * @brief A common base class for all dbus-exposed objects.
     */
    class DBusObject : public QObject
    {
        Q_OBJECT
    public:
        ~DBusObject() override;

        const QDBusObjectPath& objectPath() const
        {
            return m_objectPath;
        }

        const QSharedPointer<DBusMgr>& dbus() const
        {
            return m_dbus;
        }

    signals:
        /**
         * @brief Necessary because by the time QObject::destroyed is emitted,
         * we already lost any info in DBusObject
         */
        void destroyed(DBusObject* self);

    protected:
        explicit DBusObject(DBusObject* parent);
        explicit DBusObject(QSharedPointer<DBusMgr> dbus);

    private:
        friend class DBusMgr;
        void setObjectPath(const QString& path);

        QDBusObjectPath m_objectPath;
        QSharedPointer<DBusMgr> m_dbus;
    };

    /**
     * @brief A dbus error or not
     */
    class DBusResult : public QString
    {
    public:
        DBusResult() = default;
        explicit DBusResult(QString error)
            : QString(std::move(error))
        {
        }

        // Implicitly convert from QDBusError
        DBusResult(QDBusError::ErrorType error) // NOLINT(google-explicit-constructor)
            : QString(error == QDBusError::ErrorType::NoError ? "" : QDBusError::errorString(error))
        {
        }

        bool ok() const
        {
            return isEmpty();
        }
        bool err() const
        {
            return !isEmpty();
        }
        void okOrDie() const
        {
            Q_ASSERT(ok());
        }
    };

    /**
     * Encode the string value to a DBus object path safe representation,
     * using a schema similar to URI encoding, but with percentage(%) replaced with
     * underscore(_). All characters except [A-Za-z0-9] are encoded. For non-ascii
     * characters, UTF-8 encoding is first applied and each of the resulting byte
     * value is encoded.
     * @param value
     * @return encoded string
     */
    QString encodePath(const QString& value);

} // namespace FdoSecrets

Q_DECLARE_METATYPE(FdoSecrets::DBusResult);

#endif // KEEPASSXC_FDOSECRETS_DBUSOBJECT_H
