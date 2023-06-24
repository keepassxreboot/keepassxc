/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_NEWDATABASEWIZARDPAGE_H
#define KEEPASSXC_NEWDATABASEWIZARDPAGE_H

#include <QPointer>
#include <QWizardPage>

class Database;
class DatabaseSettingsWidget;
namespace Ui
{
    class NewDatabaseWizardPage;
}

/**
 * Pure-virtual base class for "New Database" setup wizard pages
 */
class NewDatabaseWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit NewDatabaseWizardPage(QWidget* parent = nullptr);
    Q_DISABLE_COPY(NewDatabaseWizardPage)
    ~NewDatabaseWizardPage() override;

    void setPageWidget(DatabaseSettingsWidget* page);
    DatabaseSettingsWidget* pageWidget();
    void setDatabase(QSharedPointer<Database> db);

    void initializePage() override;
    bool validatePage() override;

protected:
    QPointer<DatabaseSettingsWidget> m_pageWidget;
    QSharedPointer<Database> m_db;

    const QScopedPointer<Ui::NewDatabaseWizardPage> m_ui;
};

#endif // KEEPASSXC_NEWDATABASEWIZARDPAGE_H
