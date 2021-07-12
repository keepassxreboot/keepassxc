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

#ifndef KEEPASSXC_REPORTSWIDGET_H
#define KEEPASSXC_REPORTSWIDGET_H

#include "gui/settings/SettingsWidget.h"

class Database;

/**
 * Pure-virtual base class for KeePassXC database settings widgets.
 */
class ReportsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ReportsWidget(QWidget* parent = nullptr);
    Q_DISABLE_COPY(ReportsWidget);
    ~ReportsWidget() override;

    virtual void load(QSharedPointer<Database> db);

    const QSharedPointer<Database> getDatabase() const;

signals:
    /**
     * Can be emitted to indicate size changes and allow parents widgets to adjust properly.
     */
    void sizeChanged();

protected:
    QSharedPointer<Database> m_db;
};

#endif // KEEPASSXC_REPORTSWIDGET_H
