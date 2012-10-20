/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "EditGroupWidget.h"
#include "ui_EditGroupWidgetAdvanced.h"
#include "ui_EditGroupWidgetMain.h"
#include "ui_EditWidget.h"

#include "core/Metadata.h"
#include "gui/EditWidgetIcons.h"

EditGroupWidget::EditGroupWidget(QWidget* parent)
    : EditWidget(parent)
    , m_mainUi(new Ui::EditGroupWidgetMain())
    , m_advancedUi(new Ui::EditGroupWidgetAdvanced())
    , m_editGroupWidgetMain(new QWidget())
    , m_editGroupWidgetIcons(new EditWidgetIcons())
    , m_editGroupWidgetAdvanced(new QWidget())
    , m_group(Q_NULLPTR)
{
    m_mainUi->setupUi(m_editGroupWidgetMain);
    m_advancedUi->setupUi(m_editGroupWidgetAdvanced);

    add(tr("Group"), m_editGroupWidgetMain);
    add(tr("Icon"), m_editGroupWidgetIcons);
    add(tr("Advanced"), m_editGroupWidgetAdvanced);

    m_mainUi->searchComboBox->addItem("Inherit");
    m_mainUi->searchComboBox->addItem("Enable");
    m_mainUi->searchComboBox->addItem("Disable");
    m_mainUi->autotypeComboBox->addItem("Inherit");
    m_mainUi->autotypeComboBox->addItem("Enable");
    m_mainUi->autotypeComboBox->addItem("Disable");

    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));

    connect(this, SIGNAL(accepted()), SLOT(save()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));
}

EditGroupWidget::~EditGroupWidget()
{
}

void EditGroupWidget::loadGroup(Group* group, bool create, Database* database)
{
    m_group = group;
    m_database = database;

    if (create) {
        setHeadline(tr("Add group"));
    }
    else {
        setHeadline(tr("Edit group"));
    }

    m_mainUi->editName->setText(m_group->name());
    m_mainUi->editNotes->setPlainText(m_group->notes());
    m_mainUi->expireCheck->setChecked(group->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(group->timeInfo().expiryTime().toLocalTime());
    QString timeFormat("d MMM yyyy HH:mm:ss");
    m_advancedUi->modifiedEdit->setText(
                group->timeInfo().lastModificationTime().toLocalTime().toString(timeFormat));
    m_advancedUi->createdEdit->setText(
                group->timeInfo().creationTime().toLocalTime().toString(timeFormat));
    m_advancedUi->accessedEdit->setText(
                group->timeInfo().lastAccessTime().toLocalTime().toString(timeFormat));
    m_advancedUi->uuidEdit->setText(group->uuid().toHex());
    switch (group->searchingEnabled()) {
    case Group::Inherit:
        m_mainUi->searchComboBox->setCurrentIndex(0);
        break;
    case Group::Enable:
        m_mainUi->searchComboBox->setCurrentIndex(1);
        break;
    case Group::Disable:
        m_mainUi->searchComboBox->setCurrentIndex(2);
        break;
    default:
        Q_ASSERT(false);
    }
    switch (group->autoTypeEnabled()) {
    case Group::Inherit:
        m_mainUi->autotypeComboBox->setCurrentIndex(0);
        break;
    case Group::Enable:
        m_mainUi->autotypeComboBox->setCurrentIndex(1);
        break;
    case Group::Disable:
        m_mainUi->autotypeComboBox->setCurrentIndex(2);
        break;
    default:
        Q_ASSERT(false);
    }

    IconStruct iconStruct;
    iconStruct.uuid = group->iconUuid();
    iconStruct.number = group->iconNumber();
    m_editGroupWidgetIcons->load(group->uuid(), database, iconStruct);

    setCurrentRow(0);

    m_mainUi->editName->setFocus();
}

void EditGroupWidget::save()
{
    m_group->setName(m_mainUi->editName->text());
    m_group->setNotes(m_mainUi->editNotes->toPlainText());
    m_group->setExpires(m_mainUi->expireCheck->isChecked());
    m_group->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());
    switch (m_mainUi->searchComboBox->currentIndex()) {
    case 0:
        m_group->setSearchingEnabled(Group::Inherit);
        break;
    case 1:
        m_group->setSearchingEnabled(Group::Enable);
        break;
    case 2:
        m_group->setSearchingEnabled(Group::Disable);
        break;
    default:
        Q_ASSERT(false);
    }
    switch (m_mainUi->autotypeComboBox->currentIndex()) {
    case 0:
        m_group->setAutoTypeEnabled(Group::Inherit);
        break;
    case 1:
        m_group->setAutoTypeEnabled(Group::Enable);
        break;
    case 2:
        m_group->setAutoTypeEnabled(Group::Disable);
        break;
    default:
        Q_ASSERT(false);
    }

    IconStruct iconStruct = m_editGroupWidgetIcons->save();

    if (iconStruct.number < 0) {
        m_group->setIcon(Group::DefaultIconNumber);
    }
    else if (iconStruct.uuid.isNull()) {
        m_group->setIcon(iconStruct.number);
    }
    else {
        m_group->setIcon(iconStruct.uuid);
    }

    m_group = Q_NULLPTR;
    m_database = Q_NULLPTR;
    Q_EMIT editFinished(true);
}

void EditGroupWidget::cancel()
{
    if (!m_group->iconUuid().isNull() &&
            !m_database->metadata()->containsCustomIcon(m_group->iconUuid())) {
        m_group->setIcon(Entry::DefaultIconNumber);
    }

    m_group = Q_NULLPTR;
    m_database = Q_NULLPTR;
    Q_EMIT editFinished(false);
}
