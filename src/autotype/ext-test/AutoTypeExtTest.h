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

#ifndef KEEPASSX_AUTOTYPEEXTTEST
#define KEEPASSX_AUTOTYPEEXTTEST

#include "AutoTypeExtTestInterface.h"

class AutoTypeExtTest : public QObject, public AutoTypeExternalInterface, public AutoTypeExtTestInterface
{
Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.keepassx.AutoTypeExternalInterface")
    Q_INTERFACES(AutoTypeExternalInterface AutoTypeExtTestInterface)

public:
    QString keyToString(Qt::Key key) override;

    bool isAvailable() override;
    TargetedAutoTypeExecutor* createExecutor() override;

    void setTargetMap(const AutoTypeTargetMap& targetMap) override;
    AutoTypeTargetMap availableTargets() override;

    QString actionCharsForTarget(const QString& targetIdentifier) override;
    int actionCountForTarget(const QString& targetIdentifier) override;

    void clearActions() override;

    void addAction(const QSharedPointer<AutoTypeTarget>& target, const AutoTypeKey* action);

private:
    QHash<QString, int> m_actionCountPerTarget;
    QHash<QString, QString> m_actionCharsPerTarget;
    AutoTypeTargetMap m_targetMap;
};

class AutoTypeExtExecutorTest : public TargetedAutoTypeExecutor
{
public:
    explicit AutoTypeExtExecutorTest(AutoTypeExtTest* platform);

    void execType(const QSharedPointer<AutoTypeTarget>& target, AutoTypeKey* action) override;
    void execClearField(const QSharedPointer<AutoTypeTarget>& target, AutoTypeClearField* action) override;

private:
    AutoTypeExtTest* const m_platform;
};


#endif // KEEPASSX_AUTOTYPEEXTTEST_H
