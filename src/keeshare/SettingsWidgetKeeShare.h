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

#ifndef KEEPASSXC_SETTINGSWIDGETKEESHARE_H
#define KEEPASSXC_SETTINGSWIDGETKEESHARE_H

#include <QPointer>
#include <QWidget>

#include "gui/MessageWidget.h"
#include "keeshare/KeeShareSettings.h"

class Database;

class QStandardItemModel;

namespace Ui
{
    class SettingsWidgetKeeShare;
}

class SettingsWidgetKeeShare : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidgetKeeShare(QWidget* parent = nullptr);
    ~SettingsWidgetKeeShare();

    void loadSettings();
    void saveSettings();

signals:
    void settingsMessage(const QString&, MessageWidget::MessageType type);

private slots:
    void setVerificationExporter(const QString& signer);

    void generateCertificate();
    void importCertificate();
    void exportCertificate();

    void trustSelectedCertificates();
    void askSelectedCertificates();
    void untrustSelectedCertificates();
    void removeSelectedCertificates();

private:
    void updateOwnCertificate();
    void updateForeignCertificates();

    QScopedPointer<Ui::SettingsWidgetKeeShare> m_ui;

    KeeShareSettings::Own m_own;
    KeeShareSettings::Foreign m_foreign;
    QScopedPointer<QStandardItemModel> m_importedCertificateModel;
};

#endif // KEEPASSXC_SETTINGSWIDGETKEESHARE_H
