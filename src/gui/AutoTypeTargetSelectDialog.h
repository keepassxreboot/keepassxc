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

#ifndef KEEPASSX_AUTO_TYPE_TARGET_SELECT_DIALOG_H
#define KEEPASSX_AUTO_TYPE_TARGET_SELECT_DIALOG_H

#include <QDialog>
#include <QPointer>
#include <QSharedPointer>
#include <QStandardItemModel>
#include "autotype/AutoTypeExternalPlugin.h"
#include "core/Entry.h"

namespace Ui
{
    class AutoTypeTargetSelectDialog;
}

class AutoTypeTargetSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutoTypeTargetSelectDialog(QString pluginName,
                                        const AutoTypeTargetMap& targetList,
                                        QWidget* parent = nullptr,
                                        Entry* entry = nullptr);
    ~AutoTypeTargetSelectDialog() override;

private slots:
    void performAutoType();

private:
    QScopedPointer<Ui::AutoTypeTargetSelectDialog> m_ui;
    Entry* m_entry;
    QString m_pluginName;
    AutoTypeTargetMap m_targetList;

    Q_DISABLE_COPY(AutoTypeTargetSelectDialog)
};

#endif // KEEPASSX_AUTO_TYPE_TARGET_SELECT_DIALOG_H
