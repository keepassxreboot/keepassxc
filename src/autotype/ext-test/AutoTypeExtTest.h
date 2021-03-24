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
    bool isTargetSelectionRequired() override;
    TargetedAutoTypeExecutor* createExecutor() override;

    AutoTypeTargetMap availableTargets() override;

    QString getActionChars() override;
    QString getActionChars(const QString& targetIdentifier) override;
    int getActionCount() override;
    int getActionCount(const QString& targetIdentifier) override;

    void setTargetSelectionRequired(bool value) override;
    void setTargetMap(const AutoTypeTargetMap& targetMap) override;
    void clearActions() override;

    void addAction(const QSharedPointer<AutoTypeTarget>& target, const AutoTypeKey* action);

private:
    QHash<QString, int> m_actionCountPerTarget;
    QHash<QString, QString> m_actionCharsPerTarget;

    int m_actionCountTargetless = 0;
    QString m_actionCharsTargetLess;

    AutoTypeTargetMap m_targetMap;
    bool m_isTargetSelectionRequired;
};

class AutoTypeExtExecutorTest : public TargetedAutoTypeExecutor
{
public:
    explicit AutoTypeExtExecutorTest(AutoTypeExtTest* platform);

    AutoTypeAction::Result execBegin(const AutoTypeBegin* action, const QSharedPointer<AutoTypeTarget>& target) override;
    AutoTypeAction::Result execType(AutoTypeKey* action, const QSharedPointer<AutoTypeTarget>& target) override;
    AutoTypeAction::Result execClearField(AutoTypeClearField* action, const QSharedPointer<AutoTypeTarget>& target) override;

private:
    AutoTypeExtTest* const m_platform;
};


#endif // KEEPASSX_AUTOTYPEEXTTEST_H
