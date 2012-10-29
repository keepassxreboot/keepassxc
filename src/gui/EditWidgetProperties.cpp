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

#include "EditWidgetProperties.h"
#include "ui_EditWidgetProperties.h"

EditWidgetProperties::EditWidgetProperties(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EditWidgetProperties())
{
    m_ui->setupUi(this);
}

EditWidgetProperties::~EditWidgetProperties()
{
}

void EditWidgetProperties::setFields(TimeInfo timeInfo, Uuid uuid)
{
    QString timeFormat("d MMM yyyy HH:mm:ss");
    m_ui->modifiedEdit->setText(
                timeInfo.lastModificationTime().toLocalTime().toString(timeFormat));
    m_ui->createdEdit->setText(
                timeInfo.creationTime().toLocalTime().toString(timeFormat));
    m_ui->accessedEdit->setText(
                timeInfo.lastAccessTime().toLocalTime().toString(timeFormat));
    m_ui->uuidEdit->setText(uuid.toHex());
}
