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
#include "gui/Font.h"
#include "ui_EditGroupWidgetMain.h"

#include "core/Config.h"
#include "core/Metadata.h"
#include "core/Resources.h"
#include "core/TimeDelta.h"
#include "core/TriState.h"
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetProperties.h"
#include "gui/MessageBox.h"

#if defined(WITH_XC_KEESHARE)
#include "keeshare/group/EditGroupPageKeeShare.h"
#endif

class EditGroupWidget::ExtraPage
{
public:
    ExtraPage(IEditGroupPage* page, QWidget* widget)
        : editPage(page)
        , widget(widget)
    {
    }

    void set(Group* temporaryGroup, QSharedPointer<Database> database) const
    {
        editPage->set(widget, temporaryGroup, database);
    }

    void assign() const
    {
        editPage->assign(widget);
    }

    QWidget* getWidget()
    {
        return widget;
    }

private:
    QSharedPointer<IEditGroupPage> editPage;
    QWidget* widget;
};

EditGroupWidget::EditGroupWidget(QWidget* parent)
    : EditWidget(parent)
    , m_mainUi(new Ui::EditGroupWidgetMain())
    , m_editGroupWidgetMain(new QWidget())
    , m_editGroupWidgetIcons(new EditWidgetIcons())
    , m_editWidgetProperties(new EditWidgetProperties())
    , m_group(nullptr)
{
    m_mainUi->setupUi(m_editGroupWidgetMain);

    addPage(tr("Group"), Resources::instance()->icon("document-edit"), m_editGroupWidgetMain);
    addPage(tr("Icon"), Resources::instance()->icon("preferences-desktop-icons"), m_editGroupWidgetIcons);
#if defined(WITH_XC_KEESHARE)
    addEditPage(new EditGroupPageKeeShare(this));
#endif
    addPage(tr("Properties"), Resources::instance()->icon("document-properties"), m_editWidgetProperties);

    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));
    connect(m_mainUi->autoTypeSequenceCustomRadio,
            SIGNAL(toggled(bool)),
            m_mainUi->autoTypeSequenceCustomEdit,
            SLOT(setEnabled(bool)));

    connect(this, SIGNAL(apply()), SLOT(apply()));
    connect(this, SIGNAL(accepted()), SLOT(save()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));

    // clang-format off
    connect(m_editGroupWidgetIcons,
            SIGNAL(messageEditEntry(QString,MessageWidget::MessageType)),
            SLOT(showMessage(QString,MessageWidget::MessageType)));
    // clang-format on

    connect(m_editGroupWidgetIcons, SIGNAL(messageEditEntryDismiss()), SLOT(hideMessage()));

    setupModifiedTracking();
}

EditGroupWidget::~EditGroupWidget()
{
}

void EditGroupWidget::setupModifiedTracking()
{
    // Group tab
    connect(m_mainUi->editName, SIGNAL(textChanged(QString)), SLOT(setModified()));
    connect(m_mainUi->editNotes, SIGNAL(textChanged()), SLOT(setModified()));
    connect(m_mainUi->expireCheck, SIGNAL(stateChanged(int)), SLOT(setModified()));
    connect(m_mainUi->expireDatePicker, SIGNAL(dateTimeChanged(QDateTime)), SLOT(setModified()));
    connect(m_mainUi->defaultValidityPeriodComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModified()));
    connect(m_mainUi->defaultValidityPeriodSpinBox, SIGNAL(valueChanged(int)), SLOT(setModified()));
    connect(m_mainUi->searchComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModified()));
    connect(m_mainUi->autotypeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModified()));
    connect(m_mainUi->autoTypeSequenceInherit, SIGNAL(toggled(bool)), SLOT(setModified()));
    connect(m_mainUi->autoTypeSequenceCustomRadio, SIGNAL(toggled(bool)), SLOT(setModified()));
    connect(m_mainUi->autoTypeSequenceCustomEdit, SIGNAL(textChanged(QString)), SLOT(setModified()));

    // Icon tab
    connect(m_editGroupWidgetIcons, SIGNAL(widgetUpdated()), SLOT(setModified()));
}

void EditGroupWidget::loadGroup(Group* group, bool create, const QSharedPointer<Database>& database)
{
    m_group = group;
    m_db = database;

    m_temporaryGroup.reset(group->clone(Entry::CloneNoFlags, Group::CloneNoFlags));
    connect(m_temporaryGroup->customData(), SIGNAL(customDataModified()), SLOT(setModified()));

    if (create) {
        setHeadline(tr("Add group"));
    } else {
        setHeadline(tr("Edit group"));
    }

    if (m_group->parentGroup()) {
        addTriStateItems(m_mainUi->searchComboBox, m_group->parentGroup()->resolveSearchingEnabled());
        addTriStateItems(m_mainUi->autotypeComboBox, m_group->parentGroup()->resolveAutoTypeEnabled());
        addTriStateItems(m_mainUi->defaultValidityPeriodComboBox,
                         m_group->parentGroup()->resolveDefaultValidityPeriodEnabled());
    } else {
        addTriStateItems(m_mainUi->searchComboBox, true);
        addTriStateItems(m_mainUi->autotypeComboBox, true);
        addTriStateItems(m_mainUi->defaultValidityPeriodComboBox, false);
    }

    m_mainUi->defaultValidityPeriodPresets->setMenu(createPresetsMenu());
    connect(m_mainUi->defaultValidityPeriodPresets->menu(),
            SIGNAL(triggered(QAction*)),
            this,
            SLOT(useValidityPeriodPreset(QAction*)));
    m_mainUi->defaultValidityPeriodComboBox->setCurrentIndex(
        TriState::indexFromTriState(group->defaultValidityPeriodEnabled()));
    connect(m_mainUi->defaultValidityPeriodComboBox,
            SIGNAL(currentIndexChanged(int)),
            this,
            SLOT(handleDefaultValidityPeriodTriState(int)));
    m_mainUi->defaultValidityPeriodSpinBox->setEnabled(group->defaultValidityPeriodEnabled() == TriState::Enable);
    m_mainUi->defaultValidityPeriodSpinBox->setValue(group->resolveDefaultValidityPeriod());

    m_mainUi->editName->setText(m_group->name());
    m_mainUi->editNotes->setPlainText(m_group->notes());
    m_mainUi->expireCheck->setChecked(group->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(group->timeInfo().expiryTime().toLocalTime());
    m_mainUi->searchComboBox->setCurrentIndex(TriState::indexFromTriState(group->searchingEnabled()));
    m_mainUi->autotypeComboBox->setCurrentIndex(TriState::indexFromTriState(group->autoTypeEnabled()));
    if (group->defaultAutoTypeSequence().isEmpty()) {
        m_mainUi->autoTypeSequenceInherit->setChecked(true);
    } else {
        m_mainUi->autoTypeSequenceCustomRadio->setChecked(true);
    }
    m_mainUi->autoTypeSequenceCustomEdit->setText(group->effectiveAutoTypeSequence());

    if (config()->get(Config::GUI_MonospaceNotes).toBool()) {
        m_mainUi->editNotes->setFont(Font::fixedFont());
    } else {
        m_mainUi->editNotes->setFont(Font::defaultFont());
    }

    IconStruct iconStruct;
    iconStruct.uuid = m_temporaryGroup->iconUuid();
    iconStruct.number = m_temporaryGroup->iconNumber();
    m_editGroupWidgetIcons->load(m_temporaryGroup->uuid(), m_db, iconStruct);
    m_editWidgetProperties->setFields(m_temporaryGroup->timeInfo(), m_temporaryGroup->uuid());
    m_editWidgetProperties->setCustomData(m_temporaryGroup->customData());

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.set(m_temporaryGroup.data(), m_db);
    }

    setCurrentPage(0);

    m_mainUi->editName->setFocus();

    // Force the user to Save/Discard new groups
    showApplyButton(!create);

    setModified(false);
}

void EditGroupWidget::save()
{
    apply();
    clear();
    emit editFinished(true);
}

void EditGroupWidget::apply()
{
    m_temporaryGroup->setName(m_mainUi->editName->text());
    m_temporaryGroup->setNotes(m_mainUi->editNotes->toPlainText());
    m_temporaryGroup->setExpires(m_mainUi->expireCheck->isChecked());
    m_temporaryGroup->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

    m_temporaryGroup->setDefaultValidityPeriodEnabled(
        TriState::triStateFromIndex(m_mainUi->defaultValidityPeriodComboBox->currentIndex()));
    m_temporaryGroup->setDefaultValidityPeriod(m_mainUi->defaultValidityPeriodSpinBox->value());

    m_temporaryGroup->setSearchingEnabled(TriState::triStateFromIndex(m_mainUi->searchComboBox->currentIndex()));
    m_temporaryGroup->setAutoTypeEnabled(TriState::triStateFromIndex(m_mainUi->autotypeComboBox->currentIndex()));

    if (m_mainUi->autoTypeSequenceInherit->isChecked()) {
        m_temporaryGroup->setDefaultAutoTypeSequence(QString());
    } else {
        m_temporaryGroup->setDefaultAutoTypeSequence(m_mainUi->autoTypeSequenceCustomEdit->text());
    }

    IconStruct iconStruct = m_editGroupWidgetIcons->state();

    if (iconStruct.number < 0) {
        m_temporaryGroup->setIcon(Group::DefaultIconNumber);
    } else if (iconStruct.uuid.isNull()) {
        m_temporaryGroup->setIcon(iconStruct.number);
    } else {
        m_temporaryGroup->setIcon(iconStruct.uuid);
    }

    for (const ExtraPage& page : asConst(m_extraPages)) {
        page.assign();
    }

    // Icons add/remove are applied globally outside the transaction!
    m_group->copyDataFrom(m_temporaryGroup.data());

    // Assign the icon to children if selected
    if (iconStruct.applyTo == ApplyIconToOptions::CHILD_GROUPS
        || iconStruct.applyTo == ApplyIconToOptions::ALL_CHILDREN) {
        m_group->applyGroupIconToChildGroups();
    }

    if (iconStruct.applyTo == ApplyIconToOptions::CHILD_ENTRIES
        || iconStruct.applyTo == ApplyIconToOptions::ALL_CHILDREN) {
        m_group->applyGroupIconToChildEntries();
    }

    setModified(false);
}

void EditGroupWidget::cancel()
{
    if (!m_group->iconUuid().isNull() && !m_db->metadata()->hasCustomIcon(m_group->iconUuid())) {
        m_group->setIcon(Entry::DefaultIconNumber);
    }

    if (isModified()) {
        auto result = MessageBox::question(this,
                                           QString(),
                                           tr("Entry has unsaved changes"),
                                           MessageBox::Cancel | MessageBox::Save | MessageBox::Discard,
                                           MessageBox::Cancel);
        if (result == MessageBox::Cancel) {
            return;
        }
        if (result == MessageBox::Save) {
            apply();
            setModified(false);
        }
    }

    clear();
    emit editFinished(false);
}

void EditGroupWidget::clear()
{
    m_group = nullptr;
    m_db.reset();
    m_temporaryGroup.reset(nullptr);
    m_editGroupWidgetIcons->reset();
}

void EditGroupWidget::addEditPage(IEditGroupPage* page)
{
    QWidget* widget = page->createWidget();
    widget->setParent(this);

    m_extraPages.append(ExtraPage(page, widget));
    addPage(page->name(), page->icon(), widget);
}

void EditGroupWidget::addTriStateItems(QComboBox* comboBox, bool inheritDefault)
{
    QString inheritDefaultString;
    if (inheritDefault) {
        inheritDefaultString = tr("Enable");
    } else {
        inheritDefaultString = tr("Disable");
    }

    comboBox->clear();
    comboBox->addItem(tr("Inherit from parent group (%1)").arg(inheritDefaultString));
    comboBox->addItem(tr("Enable"));
    comboBox->addItem(tr("Disable"));
}

void EditGroupWidget::useValidityPeriodPreset(QAction* action)
{
    m_mainUi->defaultValidityPeriodComboBox->setCurrentIndex(TriState::indexFromTriState(TriState::Enable));
    TimeDelta delta = action->data().value<TimeDelta>();
    m_mainUi->defaultValidityPeriodSpinBox->setValue(delta.getTotalDays());
}

QMenu* EditGroupWidget::createPresetsMenu()
{
    auto* expirePresetsMenu = new QMenu(this);
    expirePresetsMenu->addAction(tr("Tomorrow"))->setData(QVariant::fromValue(TimeDelta::fromDays(1)));
    expirePresetsMenu->addSeparator();
    expirePresetsMenu->addAction(tr("%n week(s)", nullptr, 1))->setData(QVariant::fromValue(TimeDelta::fromDays(7)));
    expirePresetsMenu->addAction(tr("%n week(s)", nullptr, 2))->setData(QVariant::fromValue(TimeDelta::fromDays(14)));
    expirePresetsMenu->addAction(tr("%n week(s)", nullptr, 3))->setData(QVariant::fromValue(TimeDelta::fromDays(21)));
    expirePresetsMenu->addSeparator();
    expirePresetsMenu->addAction(tr("%n month(s)", nullptr, 1))->setData(QVariant::fromValue(TimeDelta::fromMonths(1)));
    expirePresetsMenu->addAction(tr("%n month(s)", nullptr, 3))->setData(QVariant::fromValue(TimeDelta::fromMonths(3)));
    expirePresetsMenu->addAction(tr("%n month(s)", nullptr, 6))->setData(QVariant::fromValue(TimeDelta::fromMonths(6)));
    expirePresetsMenu->addSeparator();
    expirePresetsMenu->addAction(tr("%n year(s)", nullptr, 1))->setData(QVariant::fromValue(TimeDelta::fromYears(1)));
    expirePresetsMenu->addAction(tr("%n year(s)", nullptr, 2))->setData(QVariant::fromValue(TimeDelta::fromYears(2)));
    expirePresetsMenu->addAction(tr("%n year(s)", nullptr, 3))->setData(QVariant::fromValue(TimeDelta::fromYears(3)));
    return expirePresetsMenu;
}

void EditGroupWidget::handleDefaultValidityPeriodTriState(int index)
{
    if (index == TriState::indexFromTriState(TriState::Enable)) {
        m_mainUi->defaultValidityPeriodSpinBox->setEnabled(true);
    } else {
        m_mainUi->defaultValidityPeriodSpinBox->setEnabled(false);
    }

    m_mainUi->defaultValidityPeriodSpinBox->setValue(m_group->resolveDefaultValidityPeriod());
}
