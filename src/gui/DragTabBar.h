/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DRAGTABBAR_H
#define KEEPASSX_DRAGTABBAR_H

#include <QTabBar>

#include "core/Global.h"

class DragTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DragTabBar(QWidget* parent = Q_NULLPTR);

protected:
    void dragEnterEvent(QDragEnterEvent* event) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent* event) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent* event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent* event) Q_DECL_OVERRIDE;
    void tabLayoutChange() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void dragSwitchTab();

private:
    QTimer* const m_tabSwitchTimer;
    int m_tabSwitchIndex;
};

#endif // KEEPASSX_DRAGTABBAR_H
