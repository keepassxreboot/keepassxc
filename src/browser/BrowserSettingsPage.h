/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_BROWSERSETTINGSPAGE_H
#define KEEPASSXC_BROWSERSETTINGSPAGE_H

#include "gui/ApplicationSettingsWidget.h"

class BrowserSettingsPage : public ISettingsPage
{
public:
    explicit BrowserSettingsPage() = default;
    ~BrowserSettingsPage() override = default;

    QString name() override;
    QIcon icon() override;
    QWidget* createWidget() override;
    void loadSettings(QWidget* widget) override;
    void saveSettings(QWidget* widget) override;
};

#endif // KEEPASSXC_BROWSERSETTINGSPAGE_H
