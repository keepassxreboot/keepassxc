/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_WELCOMEWIDGET_H
#define KEEPASSX_WELCOMEWIDGET_H

#include <QListWidgetItem>

namespace Ui
{
    class WelcomeWidget;
}

class WelcomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget* parent = nullptr);
    ~WelcomeWidget() override;
    void refreshLastDatabases();

signals:
    void newDatabase();
    void openDatabase();
    void openDatabaseFile(QString);
    void importFile();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void openDatabaseFromFile(QListWidgetItem* item);

private:
    const QScopedPointer<Ui::WelcomeWidget> m_ui;
    void removeFromLastDatabases(QListWidgetItem* item);
};

#endif // KEEPASSX_WELCOMEWIDGET_H
