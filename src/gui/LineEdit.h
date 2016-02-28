/*
 *  Copyright (C) 2007 Trolltech ASA <info@trolltech.com>
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2012 Florian Geyer <blueice@fobos.de>
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

#ifndef KEEPASSX_LINEEDIT_H
#define KEEPASSX_LINEEDIT_H

#include <QLineEdit>

class QToolButton;

class LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit LineEdit(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private Q_SLOTS:
    void updateCloseButton(const QString& text);

private:
    QToolButton* const m_clearButton;
};

#endif // KEEPASSX_LINEEDIT_H
