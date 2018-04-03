/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DATABASESETTINGSWIDGET_H
#define KEEPASSX_DATABASESETTINGSWIDGET_H

#include <QLayout>
#include <QScopedPointer>
#include <QSpinBox>
#include <QWidget>

#include "crypto/kdf/Kdf.h"
#include "gui/DialogyWidget.h"

class Database;

namespace Ui
{
    class DatabaseSettingsWidget;
    class DatabaseSettingsWidgetGeneral;
    class DatabaseSettingsWidgetEncryption;
}

class DatabaseSettingsWidget : public DialogyWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingsWidget(QWidget* parent = nullptr);
    ~DatabaseSettingsWidget();
    Q_DISABLE_COPY(DatabaseSettingsWidget)

    void load(Database* db);

signals:
    void editFinished(bool accepted);

private slots:
    void save();
    void reject();
    void transformRoundsBenchmark();
    void kdfChanged(int index);
    void memoryChanged(int value);
    void parallelismChanged(int value);

private:
    void truncateHistories();

    const QScopedPointer<Ui::DatabaseSettingsWidget> m_ui;
    const QScopedPointer<Ui::DatabaseSettingsWidgetGeneral> m_uiGeneral;
    const QScopedPointer<Ui::DatabaseSettingsWidgetEncryption> m_uiEncryption;
    QWidget* m_uiGeneralPage;
    QWidget* m_uiEncryptionPage;
    Database* m_db;
};

#endif // KEEPASSX_DATABASESETTINGSWIDGET_H
