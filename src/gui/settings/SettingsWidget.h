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

#ifndef KEEPASSXC_SETTINGSWIDGET_H
#define KEEPASSXC_SETTINGSWIDGET_H

#include <QWidget>

class Database;

/**
 * Pure-virtual base class for KeePassXC settings widgets.
 */
class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget* parent = nullptr);
    Q_DISABLE_COPY(SettingsWidget);
    ~SettingsWidget() override;

public slots:
    /**
     * Initialize settings widget.
     */
    virtual void initialize() = 0;

    /**
     * Perform needed clean-up operations before widget is destroyed or re-initialized.
     */
    virtual void uninitialize() = 0;

    /**
     * Save settings.
     *
     * @return true on success, false on failure
     */
    virtual bool save() = 0;

    /**
     * Discard settings.
     */
    virtual void discard();

signals:
    void editFinished(bool saved);

private:
    bool m_advancedMode = false;
};

#endif // KEEPASSXC_SETTINGSWIDGET_H
