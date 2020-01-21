/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_EDITMARKUPWIDGET_H
#define KEEPASSX_EDITMARKUPWIDGET_H

#include "ui_EditMarkupWidget.h"

#include <QScopedPointer>
#include <QWidget>

namespace Ui
{
    class EditMarkupWidget;
}

class EditMarkupWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditMarkupWidget(QWidget* parent = nullptr);
    void setReadOnly(bool readOnly);
    void setMarkup(const QString& markup);
    QString getMarkup();
    void clear();

private slots:
    void updatePreview();

private:
    const QScopedPointer<Ui::EditMarkupWidget> m_ui;
};

#endif // KEEPASSX_EDITMARKUPWIDGET_H
