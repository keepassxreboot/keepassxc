/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_KPTOOLBAR_H
#define KEEPASSXC_KPTOOLBAR_H

#include <QPointer>
#include <QTimer>
#include <QToolBar>

class QAbstractButton;

class KPToolBar : public QToolBar
{
    Q_OBJECT

public:
    explicit KPToolBar(const QString& title, QWidget* parent = nullptr);
    explicit KPToolBar(QWidget* parent = nullptr);
    ~KPToolBar() override = default;

    bool isExpanded();
    bool canExpand();

public slots:
    void setExpanded(bool state);

protected:
    bool event(QEvent* event) override;

private:
    void init();

    QTimer m_expandTimer;
    QPointer<QAbstractButton> m_expandButton;
};

#endif // KEEPASSXC_KPTOOLBAR_H
