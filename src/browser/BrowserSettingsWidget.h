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

#ifndef BROWSERSETTINGSWIDGET_H
#define BROWSERSETTINGSWIDGET_H

#include <QPointer>
#include <QWidget>

namespace Ui
{
    class BrowserSettingsWidget;
}

class BrowserSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BrowserSettingsWidget(QWidget* parent = nullptr);
    ~BrowserSettingsWidget() override;

public slots:
    void loadSettings();
    void saveSettings();

private slots:
    void showProxyLocationFileDialog();
    void validateProxyLocation();
    void showCustomBrowserLocationFileDialog();

private:
    QString resolveCustomProxyLocation();

    QScopedPointer<Ui::BrowserSettingsWidget> m_ui;
};

#endif // BROWSERSETTINGSWIDGET_H
