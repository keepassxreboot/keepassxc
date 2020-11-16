/*
 *  Copyright (C) 2018 Aetf <aetf@unlimitedcodeworks.xyz>
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

#ifndef KEEPASSXC_FDOSECRETSPLUGIN_H
#define KEEPASSXC_FDOSECRETSPLUGIN_H

#include "gui/ApplicationSettingsWidget.h"
#include "gui/Icons.h"

#include <QPointer>

#include <memory>

class DatabaseTabWidget;

namespace FdoSecrets
{
    class Service;
} // namespace FdoSecrets

class FdoSecretsPlugin : public QObject, public ISettingsPage
{
    Q_OBJECT
public:
    explicit FdoSecretsPlugin(DatabaseTabWidget* tabWidget);
    ~FdoSecretsPlugin() override = default;

    QString name() override
    {
        return QObject::tr("Secret Service Integration");
    }

    QIcon icon() override
    {
        return icons()->icon("freedesktop");
    }

    QWidget* createWidget() override;
    void loadSettings(QWidget* widget) override;
    void saveSettings(QWidget* widget) override;

    void updateServiceState();

    /**
     * @return The service instance, can be nullptr if the service is disabled.
     */
    FdoSecrets::Service* serviceInstance() const;

    /**
     * @return The db tabs widget, containing opened databases. Can be nullptr.
     */
    DatabaseTabWidget* dbTabs() const;

    /**
     * Check the running secret service and returns info about it
     * @return html string suitable to be shown in the UI
     */
    QString reportExistingService() const;

    // TODO: Only used for testing. Need to split service functions away from settings page.
    static FdoSecretsPlugin* getPlugin();

public slots:
    void emitRequestSwitchToDatabases();
    void emitRequestShowNotification(const QString& msg, const QString& title = {});

    /**
     * @brief Show error in the GUI
     * @param msg
     */
    void emitError(const QString& msg);

signals:
    void error(const QString& msg);
    void requestSwitchToDatabases();
    void requestShowNotification(const QString& msg, const QString& title, int msTimeoutHint);
    void secretServiceStarted();
    void secretServiceStopped();

private:
    QPointer<DatabaseTabWidget> m_dbTabs;
    QSharedPointer<FdoSecrets::Service> m_secretService;
};

#endif // KEEPASSXC_FDOSECRETSPLUGIN_H
