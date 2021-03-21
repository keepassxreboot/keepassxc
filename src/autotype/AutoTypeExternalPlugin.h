/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_AUTOTYPEEXTERNALPLUGIN_H
#define KEEPASSX_AUTOTYPEEXTERNALPLUGIN_H

#include <QWidget>
#include "autotype/AutoTypeAction.h"

class TargetMap
{
public:
    const QString& operator[](const QString& identifier) const
    {
        for (const auto& pair : m_targets) {
            if (pair.first == identifier) {
                return pair.second;
            }
        }
        qWarning("Did not find item with key %s in TargetMap", qPrintable(identifier));
        return m_nullString;
    }

    QString& operator[](const QString& identifier)
    {
        for (auto& pair : m_targets) {
            if (pair.first == identifier) {
                return pair.second;
            }
        }
        auto pair = QPair<QString, QString>(identifier, "");
        m_targets.append(pair);
        return m_targets.last().second;
    }

    QList<QString> keys() const
    {
        QList<QString> keys;
        for (auto& pair : m_targets) {
            keys.append(pair.first);
        }
        return keys;
    }

private:
    QString m_nullString;
    QList<QPair<QString, QString>> m_targets;
};

class AutoTypeExternalInterface
{
public:
    virtual ~AutoTypeExternalInterface()
    {
    }
    virtual bool isAvailable() = 0;
    virtual TargetMap availableTargets() = 0;
    virtual void unload()
    {
    }

    virtual TargetedAutoTypeExecutor* createExecutor() = 0;
};

Q_DECLARE_INTERFACE(AutoTypeExternalInterface, "org.keepassx.AutoTypeExternalInterface/1")

#endif // KEEPASSX_AUTOTYPEEXTERNALPLUGIN_H
