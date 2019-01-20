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

#ifndef KEEPASSXC_POPUPHELPWIDGET_H
#define KEEPASSXC_POPUPHELPWIDGET_H

#include <QFrame>
#include <QPointer>

class PopupHelpWidget : public QFrame
{
    Q_OBJECT
public:
    explicit PopupHelpWidget(QWidget* parent);
    ~PopupHelpWidget() override;

    void setOffset(const QPoint& offset);
    void setPosition(Qt::Corner corner);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void alignWithParent();
    QPointer<QWidget> m_parentWindow;
    QPointer<QWidget> m_appWindow;

    QPoint m_offset;
    Qt::Corner m_corner;
};

#endif // KEEPASSXC_POPUPHELPWIDGET_H
