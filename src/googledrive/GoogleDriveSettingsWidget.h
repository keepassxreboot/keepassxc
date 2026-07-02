/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_GOOGLEDRIVESETTINGSWIDGET_H
#define KEEPASSXC_GOOGLEDRIVESETTINGSWIDGET_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;

class GoogleDriveService;

class GoogleDriveSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GoogleDriveSettingsWidget(QWidget* parent = nullptr);
    ~GoogleDriveSettingsWidget() override;

    void loadSettings();
    void saveSettings();

private slots:
    void connectClicked();
    void disconnectClicked();
    void onAuthStatusChanged(bool authenticated);
    void onAuthFailed(const QString& error);

private:
    GoogleDriveService* m_service;

    QLabel* m_statusLabel;
    QLineEdit* m_clientIdEdit;
    QLineEdit* m_clientSecretEdit;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
};

#endif // KEEPASSXC_GOOGLEDRIVESETTINGSWIDGET_H
