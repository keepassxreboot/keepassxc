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
#include "ui_EditGroupWidgetMain.h"

#include "core/Metadata.h"
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetProperties.h"

EditGroupWidget::EditGroupWidget(QWidget* parent)
    : EditWidget(parent)
    , m_mainUi(new Ui::EditGroupWidgetMain())
    , m_editGroupWidgetMain(new QWidget())
    , m_editGroupWidgetIcons(new EditWidgetIcons())
    , m_editWidgetProperties(new EditWidgetProperties())
    , m_group(Q_NULLPTR)
    , m_database(Q_NULLPTR)
{
    m_mainUi->setupUi(m_editGroupWidgetMain);

    add(tr("Group"), m_editGroupWidgetMain);
    add(tr("Icon"), m_editGroupWidgetIcons);
    add(tr("Properties"), m_editWidgetProperties);

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

    if (m_group->parentGroup()) {
        addTriStateItems(m_mainUi->searchComboBox, m_group->parentGroup()->resolveSearchingEnabled());
        addTriStateItems(m_mainUi->autotypeComboBox, m_group->parentGroup()->resolveAutoTypeEnabled());
    }
    else {
        addTriStateItems(m_mainUi->searchComboBox, true);
        addTriStateItems(m_mainUi->autotypeComboBox, true);
    }

    m_mainUi->editName->setText(m_group->name());
    m_mainUi->editNotes->setPlainText(m_group->notes());
    m_mainUi->expireCheck->setChecked(group->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(group->timeInfo().expiryTime().toLocalTime());
    m_mainUi->searchComboBox->setCurrentIndex(indexFromTriState(group->searchingEnabled()));
    m_mainUi->autotypeComboBox->setCurrentIndex(indexFromTriState(group->autoTypeEnabled()));

    IconStruct iconStruct;
    iconStruct.uuid = group->iconUuid();
    iconStruct.number = group->iconNumber();
    m_editGroupWidgetIcons->load(group->uuid(), database, iconStruct);

    m_editWidgetProperties->setFields(group->timeInfo(), group->uuid());

    setCurrentRow(0);

    m_mainUi->editName->setFocus();
}

void EditGroupWidget::save()
{
    m_group->setName(m_mainUi->editName->text());
    m_group->setNotes(m_mainUi->editNotes->toPlainText());
    m_group->setExpires(m_mainUi->expireCheck->isChecked());
    m_group->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

    m_group->setSearchingEnabled(triStateFromIndex(m_mainUi->searchComboBox->currentIndex()));
    m_group->setAutoTypeEnabled(triStateFromIndex(m_mainUi->autotypeComboBox->currentIndex()));

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

void EditGroupWidget::addTriStateItems(QComboBox* comboBox, bool inheritDefault)
{
    QString inheritDefaultString;
    if (inheritDefault) {
        inheritDefaultString = tr("Enable");
    }
    else {
        inheritDefaultString = tr("Disable");
    }

    comboBox->clear();
    comboBox->addItem(tr("Inherit from parent group (%1)").arg(inheritDefaultString));
    comboBox->addItem(tr("Enable"));
    comboBox->addItem(tr("Disable"));
}

int EditGroupWidget::indexFromTriState(Group::TriState triState)
{
    switch (triState) {
    case Group::Inherit:
        return 0;
    case Group::Enable:
        return 1;
    case Group::Disable:
        return 2;
    default:
        Q_ASSERT(false);
        return 0;
    }
}

Group::TriState EditGroupWidget::triStateFromIndex(int index)
{
    switch (index) {
    case 0:
        return Group::Inherit;
    case 1:
        return Group::Enable;
    case 2:
        return Group::Disable;
    default:
        Q_ASSERT(false);
        return Group::Inherit;
    }
}
