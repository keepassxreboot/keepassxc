/*
 * Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_OSUTILSBASE_H
#define KEEPASSXC_OSUTILSBASE_H

#include <QObject>
#include <QPointer>

/**
 * Abstract base class for generic OS-specific functionality
 * which can be reasonably expected to be available on all platforms.
 */
class OSUtilsBase : public QObject
{
    Q_OBJECT

public:
    virtual bool isDarkMode() = 0;

protected:
    explicit OSUtilsBase(QObject* parent = nullptr);
    virtual ~OSUtilsBase();
};

#endif // KEEPASSXC_OSUTILSBASE_H
