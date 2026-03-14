/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_URLEDIT_H
#define KEEPASSX_URLEDIT_H

#include <QAction>
#include <QLineEdit>
#include <QPointer>

#include "core/Entry.h"

class URLEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit URLEdit(QWidget* parent = nullptr);
    void enableVerifyMode();
    void setEntry(Entry* entry);

private slots:
    void updateStylesheet(const QString& url = {});

private:
    QPointer<QAction> m_errorAction;
    QPointer<Entry> m_entry;
};

#endif // KEEPASSX_URLEDIT_H
