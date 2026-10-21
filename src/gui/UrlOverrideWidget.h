/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_URLOVERRIDEWIDGET_H
#define KEEPASSXC_URLOVERRIDEWIDGET_H

#include <QWidget>

#include "gui/UrlOverride.h"

class QPushButton;
class QTableWidget;

// Editor for the URL scheme override rules, shown as a tab of the general application settings
class UrlOverrideWidget : public QWidget
{
    Q_OBJECT

public:
    explicit UrlOverrideWidget(QWidget* parent = nullptr);

    void loadSettings();
    void saveSettings();

private:
    QList<UrlOverride::Rule> tableRules() const;
    void setTableRules(const QList<UrlOverride::Rule>& rules);
    void addRow(bool enabled, const QString& scheme, const QString& command);
    void removeSelectedRow();
    void moveSelectedRow(int direction);

    QTableWidget* const m_table;
    QPushButton* const m_addButton;
    QPushButton* const m_removeButton;
    QPushButton* const m_moveUpButton;
    QPushButton* const m_moveDownButton;
    QList<UrlOverride::Rule> m_originalRules;
};

#endif // KEEPASSXC_URLOVERRIDEWIDGET_H
