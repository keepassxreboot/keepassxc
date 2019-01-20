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

#include "MessageBox.h"
#include "core/CustomData.h"
#include "core/TimeInfo.h"

#include <QUuid>

EditWidgetProperties::EditWidgetProperties(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EditWidgetProperties())
    , m_customDataModel(new QStandardItemModel(this))
{
    m_ui->setupUi(this);
    m_ui->removeCustomDataButton->setEnabled(false);
    m_ui->customDataTable->setModel(m_customDataModel);

    // clang-format off
    connect(m_ui->customDataTable->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(toggleRemoveButton(QItemSelection)));
    // clang-format on
    connect(m_ui->removeCustomDataButton, SIGNAL(clicked()), SLOT(removeSelectedPluginData()));
}

EditWidgetProperties::~EditWidgetProperties()
{
}

void EditWidgetProperties::setFields(const TimeInfo& timeInfo, const QUuid& uuid)
{
    static const QString timeFormat("d MMM yyyy HH:mm:ss");
    m_ui->modifiedEdit->setText(timeInfo.lastModificationTime().toLocalTime().toString(timeFormat));
    m_ui->createdEdit->setText(timeInfo.creationTime().toLocalTime().toString(timeFormat));
    m_ui->accessedEdit->setText(timeInfo.lastAccessTime().toLocalTime().toString(timeFormat));
    m_ui->uuidEdit->setText(uuid.toRfc4122().toHex());
}

void EditWidgetProperties::setCustomData(CustomData* customData)
{
    if (m_customData) {
        m_customData->disconnect(this);
    }

    m_customData = customData;

    if (m_customData) {
        connect(m_customData, SIGNAL(customDataModified()), SLOT(update()));
    }

    update();
}

void EditWidgetProperties::removeSelectedPluginData()
{
    auto result = MessageBox::question(this,
                                       tr("Delete plugin data?"),
                                       tr("Do you really want to delete the selected plugin data?\n"
                                          "This may cause the affected plugins to malfunction."),
                                       MessageBox::Delete | MessageBox::Cancel,
                                       MessageBox::Cancel);

    if (result == MessageBox::Cancel) {
        return;
    }

    const QItemSelectionModel* itemSelectionModel = m_ui->customDataTable->selectionModel();
    if (itemSelectionModel) {
        for (const QModelIndex& index : itemSelectionModel->selectedRows(0)) {
            const QString key = index.data().toString();
            m_customData->remove(key);
        }
        update();
    }
}

void EditWidgetProperties::toggleRemoveButton(const QItemSelection& selected)
{
    m_ui->removeCustomDataButton->setEnabled(!selected.isEmpty());
}

void EditWidgetProperties::update()
{
    m_customDataModel->clear();
    m_customDataModel->setHorizontalHeaderLabels({tr("Key"), tr("Value")});
    if (!m_customData) {
        m_ui->removeCustomDataButton->setEnabled(false);
    } else {
        for (const QString& key : m_customData->keys()) {
            m_customDataModel->appendRow(QList<QStandardItem*>()
                                         << new QStandardItem(key) << new QStandardItem(m_customData->value(key)));
        }
        m_ui->removeCustomDataButton->setEnabled(!m_customData->isEmpty());
    }
}
