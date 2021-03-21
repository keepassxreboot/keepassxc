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

#ifndef KEEPASSX_AUTOTYPELIBVIRT_H
#define KEEPASSX_AUTOTYPELIBVIRT_H

#include <QApplication>
#include <QSet>
#include <QWidget>
#include <QtPlugin>
#include <libvirt.h>
#include "autotype/AutoTypeExternalPlugin.h"
#include "core/Tools.h"

class AutoTypeExtLibvirt : public QObject, public AutoTypeExternalInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypeExtLibvirt")
    Q_INTERFACES(AutoTypeExternalInterface)

public:
    AutoTypeExtLibvirt();
    void unload() override;

    bool isAvailable() override;
    TargetMap availableTargets() override;

    TargetedAutoTypeExecutor* createExecutor() override;

    QList<uint> charToKeyCodeGroup(const QChar& ch);
    uint keyToKeyCode(Qt::Key key);

    void sendKeyCodesToDomain(const QString& domainUuid, QList<uint> keyCodes);

private:
    virConnectPtr m_libvirtConnection;
};

class AutoTypeExecutorLibvirt : public TargetedAutoTypeExecutor
{
public:
    explicit AutoTypeExecutorLibvirt(AutoTypeExtLibvirt* plugin);

    AutoTypeAction::Result execBegin(const QString& targetIdentifier, const AutoTypeBegin* action) override;
    AutoTypeAction::Result execType(const QString& targetIdentifier, AutoTypeKey* action) override;
    AutoTypeAction::Result execClearField(const QString& targetIdentifier, AutoTypeClearField* action) override;

private:
    AutoTypeExtLibvirt* const m_plugin;
};

#endif // KEEPASSX_AUTOTYPELIBVIRT_H
