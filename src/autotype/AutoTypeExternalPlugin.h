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

class AutoTypeTarget
{
public:
    AutoTypeTarget(QString identifier, QString presentableName)
        : m_identifier(std::move(identifier))
        , m_presentableName(std::move(presentableName))
    {
    }

    virtual ~AutoTypeTarget() = default;

    virtual const QString& getIdentifier() const
    {
        return m_identifier;
    };

    virtual const QString& getPresentableName() const
    {
        return m_presentableName;
    };

private:
    const QString m_identifier;
    const QString m_presentableName;
};

class AutoTypeTargetMap
{
public:
    AutoTypeTargetMap()
        : m_targets(QList<QSharedPointer<AutoTypeTarget>>())
    {
    }

    ~AutoTypeTargetMap() = default;

    QSharedPointer<AutoTypeTarget> get(const QString& identifier) const
    {
        for (const auto& target : m_targets) {
            if (target->getIdentifier() == identifier) {
                return target;
            }
        }
        qWarning("Did not find item with key %s in AutoTypeTargetMap", qPrintable(identifier));
        return QSharedPointer<AutoTypeTarget>(nullptr);
    }

    void append(QSharedPointer<AutoTypeTarget> target)
    {
        m_targets.append(target);
    }

    const QList<QSharedPointer<AutoTypeTarget>>& values() const
    {
        return m_targets;
    }

private:
    QList<QSharedPointer<AutoTypeTarget>> m_targets;
};

class AutoTypeExternalInterface
{
public:
    virtual ~AutoTypeExternalInterface() = default;

    virtual bool isAvailable() = 0;
    virtual bool isTargetSelectionRequired() = 0;
    virtual AutoTypeTargetMap availableTargets() = 0;

    virtual void unload()
    {
    }

    virtual TargetedAutoTypeExecutor* createExecutor() = 0;
};

Q_DECLARE_INTERFACE(AutoTypeExternalInterface, "org.keepassx.AutoTypeExternalInterface/1")

#endif // KEEPASSX_AUTOTYPEEXTERNALPLUGIN_H
