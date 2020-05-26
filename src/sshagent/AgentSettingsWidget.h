/*
 *  Copyright (C) 2017 Toni Spets <toni.spets@iki.fi>
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

#ifndef AGENTSETTINGSWIDGET_H
#define AGENTSETTINGSWIDGET_H

#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class AgentSettingsWidget;
}

class AgentSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AgentSettingsWidget(QWidget* parent = nullptr);
    ~AgentSettingsWidget();

public slots:
    void loadSettings();
    void saveSettings();
    void toggleSettingsEnabled();

private:
    QScopedPointer<Ui::AgentSettingsWidget> m_ui;
};

#endif // AGENTSETTINGSWIDGET_H
