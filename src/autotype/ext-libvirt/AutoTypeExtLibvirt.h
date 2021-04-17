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

enum class OperatingSystem
{
    Unknown,
    Linux,
    Windows,
    MacOSX,
};

class AutoTypeTargetLibvirt : public AutoTypeTarget
{
public:
    AutoTypeTargetLibvirt(QString identifier,
                          QString presentableName,
                          virDomainPtr domain);
    ~AutoTypeTargetLibvirt();

    virDomainPtr getDomain();
    OperatingSystem getOperatingSystem();

private:
    OperatingSystem detectOperatingSystem();

    virDomainPtr m_domain;
    OperatingSystem m_operatingSystem;

    static const QRegExp m_patternLinuxOperatingSystemId;
    static const QRegExp m_patternLinuxOperatingSystemUrl;
};

class AutoTypeExtLibvirt : public QObject, public AutoTypeExternalInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypeExtLibvirt")
    Q_INTERFACES(AutoTypeExternalInterface)

public:
    AutoTypeExtLibvirt();
    void unload() override;

    bool isAvailable() override;
    bool isTargetSelectionRequired() override;
    AutoTypeTargetMap availableTargets() override;

    TargetedAutoTypeExecutor* createExecutor() override;

    QVector<uint> charToKeyCodes(const QChar& character, OperatingSystem targetOperatingSystem);
    QVector<uint> keyToKeyCodes(Qt::Key key);
    QVector<uint> modifiersToKeyCodes(Qt::KeyboardModifiers modifiers);

    void sendKeyCodesToTarget(QSharedPointer<AutoTypeTargetLibvirt> target, QVector<uint> keyCodes);

private:
    bool shouldHandleDeadKeys(OperatingSystem operatingSystem);

    virConnectPtr m_libvirtConnection;

    static const QHash<uint, uint> m_lowerSymbolKeyMapping;
    static const QHash<uint, uint> m_upperSymbolKeyMapping;
    static const QHash<Qt::Key, uint> m_keyToKeyCodeMapping;
    static const QHash<Qt::KeyboardModifier, uint> m_modifierToKeyCodeMapping;
    static const QVector<uint> m_deadKeys;
};

class AutoTypeExecutorLibvirt : public TargetedAutoTypeExecutor
{
public:
    explicit AutoTypeExecutorLibvirt(AutoTypeExtLibvirt* plugin);

    AutoTypeAction::Result execBegin(const AutoTypeBegin* action, QSharedPointer<AutoTypeTarget> target) override;
    AutoTypeAction::Result execType(AutoTypeKey* action, QSharedPointer<AutoTypeTarget> target) override;
    AutoTypeAction::Result execClearField(AutoTypeClearField* action, QSharedPointer<AutoTypeTarget> target) override;

private:
    AutoTypeExtLibvirt* const m_plugin;
};

#endif // KEEPASSX_AUTOTYPELIBVIRT_H
