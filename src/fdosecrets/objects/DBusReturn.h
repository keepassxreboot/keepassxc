/*
 *  Copyright (C) 2019 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETS_DBUSRETURN_H
#define KEEPASSXC_FDOSECRETS_DBUSRETURN_H

#include <QDBusError>
#include <QDebug>
#include <QString>

#include <type_traits>

namespace FdoSecrets
{

    namespace details
    {
        class DBusReturnImpl
        {
        public:
            /**
             * Check if this object contains an error
             * @return true if it contains an error, false otherwise.
             */
            bool isError() const
            {
                return !m_errorName.isEmpty();
            }

            /**
             * Get the error name
             * @return
             */
            QString errorName() const
            {
                return m_errorName;
            }

            void okOrDie() const
            {
                Q_ASSERT(!isError());
            }

        protected:
            struct WithErrorTag
            {
            };

            /**
             * Construct from an error
             * @param errorName
             * @param value
             */
            DBusReturnImpl(QString errorName, WithErrorTag)
                : m_errorName(std::move(errorName))
            {
            }

            DBusReturnImpl() = default;

        protected:
            QString m_errorName;
        };
    } // namespace details

    /**
     * Either a return value or a DBus error
     * @tparam T
     */
    template <typename T = void> class DBusReturn : public details::DBusReturnImpl
    {
    protected:
        using DBusReturnImpl::DBusReturnImpl;

    public:
        using value_type = T;

        DBusReturn() = default;

        /**
         * Implicitly construct from a value
         * @param value
         */
        DBusReturn(T&& value) // NOLINT(google-explicit-constructor)
            : m_value(std::move(value))
        {
        }

        DBusReturn(const T& value) // NOLINT(google-explicit-constructor)
            : m_value(std::move(value))
        {
        }

        /**
         * Implicitly convert from another error of different value type.
         *
         * @tparam U must not be the same as T
         * @param other
         */
        template <typename U, typename = typename std::enable_if<!std::is_same<T, U>::value>::type>
        DBusReturn(const DBusReturn<U>& other) // NOLINT(google-explicit-constructor)
            : DBusReturn(other.errorName(), DBusReturnImpl::WithErrorTag{})
        {
            Q_ASSERT(other.isError());
        }

        /**
         * Construct from error
         * @param errorType
         * @return a DBusReturn object containing the error
         */
        static DBusReturn Error(QDBusError::ErrorType errorType)
        {
            return DBusReturn{QDBusError::errorString(errorType), DBusReturnImpl::WithErrorTag{}};
        }

        /**
         * Overloaded version
         * @param errorName
         * @return a DBusReturnImpl object containing the error
         */
        static DBusReturn Error(QString errorName)
        {
            return DBusReturn{std::move(errorName), DBusReturnImpl::WithErrorTag{}};
        }

        /**
         * Get a reference to the enclosed value
         * @return
         */
        const T& value() const&
        {
            okOrDie();
            return m_value;
        }

        /**
         * Get a rvalue reference to the enclosed value if this object is rvalue
         * @return a rvalue reference to the enclosed value
         */
        T value() &&
        {
            okOrDie();
            return std::move(m_value);
        }

        template <typename P> T valueOrHandle(P* p) const&
        {
            if (isError()) {
                if (p->calledFromDBus()) {
                    p->sendErrorReply(errorName());
                }
                return {};
            }
            return m_value;
        }

        template <typename P> T&& valueOrHandle(P* p) &&
        {
            if (isError()) {
                if (p->calledFromDBus()) {
                    p->sendErrorReply(errorName());
                }
            }
            return std::move(m_value);
        }

    private:
        T m_value{};
    };

    template <> class DBusReturn<void> : public details::DBusReturnImpl
    {
    protected:
        using DBusReturnImpl::DBusReturnImpl;

    public:
        using value_type = void;

        DBusReturn() = default;

        /**
         * Implicitly convert from another error of different value type.
         *
         * @tparam U must not be the same as T
         * @param other
         */
        template <typename U, typename = typename std::enable_if<!std::is_same<void, U>::value>::type>
        DBusReturn(const DBusReturn<U>& other) // NOLINT(google-explicit-constructor)
            : DBusReturn(other.errorName(), DBusReturnImpl::WithErrorTag{})
        {
            Q_ASSERT(other.isError());
        }

        /**
         * Construct from error
         * @param errorType
         * @return a DBusReturn object containing the error
         */
        static DBusReturn Error(QDBusError::ErrorType errorType)
        {
            return DBusReturn{QDBusError::errorString(errorType), DBusReturnImpl::WithErrorTag{}};
        }

        /**
         * Overloaded version
         * @param errorName
         * @return a DBusReturnImpl object containing the error
         */
        static DBusReturn Error(QString errorName)
        {
            return DBusReturn{std::move(errorName), DBusReturnImpl::WithErrorTag{}};
        }

        /**
         * If this is return contains an error, handle it if we were called from DBus
         * @tparam P
         * @param p
         */
        template <typename P> void handle(P* p) const
        {
            if (isError()) {
                if (p->calledFromDBus()) {
                    p->sendErrorReply(errorName());
                }
            }
        }
    };

} // namespace FdoSecrets

#endif // KEEPASSXC_FDOSECRETS_DBUSRETURN_H
