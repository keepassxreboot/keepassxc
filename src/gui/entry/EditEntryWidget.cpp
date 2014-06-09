/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#include "EditEntryWidget.h"
#include "ui_EditEntryWidgetAdvanced.h"
#include "ui_EditEntryWidgetAutoType.h"
#include "ui_EditEntryWidgetHistory.h"
#include "ui_EditEntryWidgetMain.h"

#include <QDesktopServices>
#include <QStackedLayout>
#include <QMenu>
#include <QSortFilterProxyModel>

#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/FilePath.h"
#include "core/Metadata.h"
#include "core/TimeDelta.h"
#include "core/Tools.h"
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetProperties.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/entry/AutoTypeAssociationsModel.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryHistoryModel.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : EditWidget(parent)
    , m_entry(Q_NULLPTR)
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_advancedUi(new Ui::EditEntryWidgetAdvanced())
    , m_autoTypeUi(new Ui::EditEntryWidgetAutoType())
    , m_historyUi(new Ui::EditEntryWidgetHistory())
    , m_mainWidget(new QWidget())
    , m_advancedWidget(new QWidget())
    , m_iconsWidget(new EditWidgetIcons())
    , m_autoTypeWidget(new QWidget())
    , m_editWidgetProperties(new EditWidgetProperties())
    , m_historyWidget(new QWidget())
    , m_entryAttachments(new EntryAttachments(this))
    , m_attachmentsModel(new EntryAttachmentsModel(m_advancedWidget))
    , m_entryAttributes(new EntryAttributes(this))
    , m_attributesModel(new EntryAttributesModel(m_advancedWidget))
    , m_historyModel(new EntryHistoryModel(this))
    , m_sortModel(new QSortFilterProxyModel(this))
    , m_autoTypeAssoc(new AutoTypeAssociations(this))
    , m_autoTypeAssocModel(new AutoTypeAssociationsModel(this))
    , m_autoTypeDefaultSequenceGroup(new QButtonGroup(this))
    , m_autoTypeWindowSequenceGroup(new QButtonGroup(this))
{
    setupMain();
    setupAdvanced();
    setupIcon();
    setupAutoType();
    setupProperties();
    setupHistory();

    connect(this, SIGNAL(accepted()), SLOT(saveEntry()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));
}

EditEntryWidget::~EditEntryWidget()
{
}

void EditEntryWidget::setupMain()
{
    m_mainUi->setupUi(m_mainWidget);
    add(tr("Entry"), m_mainWidget);

    m_mainUi->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    connect(m_mainUi->togglePasswordButton, SIGNAL(toggled(bool)), m_mainUi->passwordEdit, SLOT(setShowPassword(bool)));
    connect(m_mainUi->tooglePasswordGeneratorButton, SIGNAL(toggled(bool)), SLOT(togglePasswordGeneratorButton(bool)));
    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));
    m_mainUi->passwordRepeatEdit->enableVerifyMode(m_mainUi->passwordEdit);
    connect(m_mainUi->passwordGenerator, SIGNAL(newPassword(QString)), SLOT(setGeneratedPassword(QString)));

    m_mainUi->expirePresets->setMenu(createPresetsMenu());
    connect(m_mainUi->expirePresets->menu(), SIGNAL(triggered(QAction*)), this, SLOT(useExpiryPreset(QAction*)));

    m_mainUi->passwordGenerator->hide();
    m_mainUi->passwordGenerator->reset();
}

void EditEntryWidget::setupAdvanced()
{
    m_advancedUi->setupUi(m_advancedWidget);
    add(tr("Advanced"), m_advancedWidget);

    m_attachmentsModel->setEntryAttachments(m_entryAttachments);
    m_advancedUi->attachmentsView->setModel(m_attachmentsModel);
    connect(m_advancedUi->saveAttachmentButton, SIGNAL(clicked()), SLOT(saveCurrentAttachment()));
    connect(m_advancedUi->addAttachmentButton, SIGNAL(clicked()), SLOT(insertAttachment()));
    connect(m_advancedUi->removeAttachmentButton, SIGNAL(clicked()), SLOT(removeCurrentAttachment()));

    m_attributesModel->setEntryAttributes(m_entryAttributes);
    m_advancedUi->attributesView->setModel(m_attributesModel);
    connect(m_advancedUi->addAttributeButton, SIGNAL(clicked()), SLOT(insertAttribute()));
    connect(m_advancedUi->editAttributeButton, SIGNAL(clicked()), SLOT(editCurrentAttribute()));
    connect(m_advancedUi->removeAttributeButton, SIGNAL(clicked()), SLOT(removeCurrentAttribute()));
    connect(m_advancedUi->attributesView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateCurrentAttribute()));
}

void EditEntryWidget::setupIcon()
{
    add(tr("Icon"), m_iconsWidget);
}

void EditEntryWidget::setupAutoType()
{
    m_autoTypeUi->setupUi(m_autoTypeWidget);
    add(tr("Auto-Type"), m_autoTypeWidget);

    m_autoTypeDefaultSequenceGroup->addButton(m_autoTypeUi->inheritSequenceButton);
    m_autoTypeDefaultSequenceGroup->addButton(m_autoTypeUi->customSequenceButton);
    m_autoTypeWindowSequenceGroup->addButton(m_autoTypeUi->defaultWindowSequenceButton);
    m_autoTypeWindowSequenceGroup->addButton(m_autoTypeUi->customWindowSequenceButton);
    m_autoTypeAssocModel->setAutoTypeAssociations(m_autoTypeAssoc);
    m_autoTypeUi->assocView->setModel(m_autoTypeAssocModel);
    m_autoTypeUi->assocView->setColumnHidden(1, true);
    connect(m_autoTypeUi->enableButton, SIGNAL(toggled(bool)), SLOT(updateAutoTypeEnabled()));
    connect(m_autoTypeUi->customSequenceButton, SIGNAL(toggled(bool)),
            m_autoTypeUi->sequenceEdit, SLOT(setEnabled(bool)));
    connect(m_autoTypeUi->customWindowSequenceButton, SIGNAL(toggled(bool)),
            m_autoTypeUi->windowSequenceEdit, SLOT(setEnabled(bool)));
    connect(m_autoTypeUi->assocAddButton, SIGNAL(clicked()), SLOT(insertAutoTypeAssoc()));
    connect(m_autoTypeUi->assocRemoveButton, SIGNAL(clicked()), SLOT(removeAutoTypeAssoc()));
    connect(m_autoTypeUi->assocView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(updateAutoTypeEnabled()));
    connect(m_autoTypeAssocModel, SIGNAL(modelReset()), SLOT(updateAutoTypeEnabled()));
    connect(m_autoTypeUi->assocView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(loadCurrentAssoc(QModelIndex)));
    connect(m_autoTypeAssocModel, SIGNAL(modelReset()), SLOT(clearCurrentAssoc()));
    connect(m_autoTypeUi->windowTitleCombo, SIGNAL(editTextChanged(QString)),
            SLOT(applyCurrentAssoc()));
    connect(m_autoTypeUi->defaultWindowSequenceButton, SIGNAL(toggled(bool)),
            SLOT(applyCurrentAssoc()));
    connect(m_autoTypeUi->windowSequenceEdit, SIGNAL(textChanged(QString)),
            SLOT(applyCurrentAssoc()));
}

void EditEntryWidget::setupProperties()
{
    add(tr("Properties"), m_editWidgetProperties);
}

void EditEntryWidget::setupHistory()
{
    m_historyUi->setupUi(m_historyWidget);
    add(tr("History"), m_historyWidget);

    m_sortModel->setSourceModel(m_historyModel);
    m_sortModel->setDynamicSortFilter(true);
    m_sortModel->setSortLocaleAware(true);
    m_sortModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_sortModel->setSortRole(Qt::UserRole);

    m_historyUi->historyView->setModel(m_sortModel);
    m_historyUi->historyView->setRootIsDecorated(false);

    connect(m_historyUi->historyView, SIGNAL(activated(QModelIndex)),
            SLOT(histEntryActivated(QModelIndex)));
    connect(m_historyUi->historyView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateHistoryButtons(QModelIndex,QModelIndex)));
    connect(m_historyUi->showButton, SIGNAL(clicked()), SLOT(showHistoryEntry()));
    connect(m_historyUi->restoreButton, SIGNAL(clicked()), SLOT(restoreHistoryEntry()));
    connect(m_historyUi->deleteButton, SIGNAL(clicked()), SLOT(deleteHistoryEntry()));
    connect(m_historyUi->deleteAllButton, SIGNAL(clicked()), SLOT(deleteAllHistoryEntries()));
}

void EditEntryWidget::emitHistoryEntryActivated(const QModelIndex& index)
{
    Q_ASSERT(!m_history);

    Entry* entry = m_historyModel->entryFromIndex(index);
    Q_EMIT historyEntryActivated(entry);
}

void EditEntryWidget::histEntryActivated(const QModelIndex& index)
{
    Q_ASSERT(!m_history);

    QModelIndex indexMapped = m_sortModel->mapToSource(index);
    if (indexMapped.isValid()) {
        emitHistoryEntryActivated(indexMapped);
    }
}

void EditEntryWidget::updateHistoryButtons(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    if (current.isValid()) {
        m_historyUi->showButton->setEnabled(true);
        m_historyUi->restoreButton->setEnabled(true);
        m_historyUi->deleteButton->setEnabled(true);
    }
    else {
        m_historyUi->showButton->setEnabled(false);
        m_historyUi->restoreButton->setEnabled(false);
        m_historyUi->deleteButton->setEnabled(false);
    }
}

void EditEntryWidget::useExpiryPreset(QAction* action)
{
    m_mainUi->expireCheck->setChecked(true);
    TimeDelta delta = action->data().value<TimeDelta>();
    QDateTime now = Tools::currentDateTimeUtc().toLocalTime();
    QDateTime expiryDateTime = now + delta;
    m_mainUi->expireDatePicker->setDateTime(expiryDateTime);
}

QString EditEntryWidget::entryTitle() const
{
    if (m_entry) {
        return m_entry->title();
    }
    else {
        return QString();
    }
}

void EditEntryWidget::loadEntry(Entry* entry, bool create, bool history, const QString& parentName,
                                Database* database)
{
    m_entry = entry;
    m_database = database;
    m_create = create;
    m_history = history;

    if (history) {
        setHeadline(QString("%1 > %2").arg(parentName, tr("Entry history")));
    }
    else {
        if (create) {
            setHeadline(QString("%1 > %2").arg(parentName, tr("Add entry")));
        }
        else {
            setHeadline(QString("%1 > %2 > %3").arg(parentName, entry->title(), tr("Edit entry")));
        }
    }

    setForms(entry);

    setCurrentRow(0);
    setRowHidden(m_historyWidget, m_history);
}

void EditEntryWidget::setForms(const Entry* entry, bool restore)
{
    m_mainUi->titleEdit->setReadOnly(m_history);
    m_mainUi->usernameEdit->setReadOnly(m_history);
    m_mainUi->urlEdit->setReadOnly(m_history);
    m_mainUi->passwordEdit->setReadOnly(m_history);
    m_mainUi->passwordRepeatEdit->setReadOnly(m_history);
    m_mainUi->expireCheck->setEnabled(!m_history);
    m_mainUi->expireDatePicker->setReadOnly(m_history);
    m_mainUi->notesEdit->setReadOnly(m_history);
    m_mainUi->tooglePasswordGeneratorButton->setChecked(false);
    m_mainUi->passwordGenerator->reset();
    m_advancedUi->addAttachmentButton->setEnabled(!m_history);
    m_advancedUi->removeAttachmentButton->setEnabled(!m_history);
    m_advancedUi->addAttributeButton->setEnabled(!m_history);
    m_advancedUi->editAttributeButton->setEnabled(false);
    m_advancedUi->removeAttributeButton->setEnabled(false);
    m_advancedUi->attributesEdit->setReadOnly(m_history);
    QAbstractItemView::EditTriggers editTriggers;
    if (m_history) {
        editTriggers = QAbstractItemView::NoEditTriggers;
    }
    else {
        editTriggers = QAbstractItemView::DoubleClicked;
    }
    m_advancedUi->attributesView->setEditTriggers(editTriggers);
    m_iconsWidget->setEnabled(!m_history);
    m_autoTypeUi->sequenceEdit->setReadOnly(m_history);
    m_autoTypeUi->windowTitleCombo->lineEdit()->setReadOnly(m_history);
    m_autoTypeUi->windowSequenceEdit->setReadOnly(m_history);
    m_historyWidget->setEnabled(!m_history);

    m_mainUi->titleEdit->setText(entry->title());
    m_mainUi->usernameEdit->setText(entry->username());
    m_mainUi->urlEdit->setText(entry->url());
    m_mainUi->passwordEdit->setText(entry->password());
    m_mainUi->passwordRepeatEdit->setText(entry->password());
    m_mainUi->expireCheck->setChecked(entry->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(entry->timeInfo().expiryTime().toLocalTime());
    m_mainUi->expirePresets->setEnabled(!m_history);
    m_mainUi->togglePasswordButton->setChecked(config()->get("security/passwordscleartext").toBool());

    m_mainUi->notesEdit->setPlainText(entry->notes());

    m_entryAttachments->copyDataFrom(entry->attachments());
    m_entryAttributes->copyCustomKeysFrom(entry->attributes());

    if (m_attributesModel->rowCount() != 0) {
        m_advancedUi->attributesView->setCurrentIndex(m_attributesModel->index(0, 0));
    }
    else {
        m_advancedUi->attributesEdit->setPlainText("");
        m_advancedUi->attributesEdit->setEnabled(false);
    }

    IconStruct iconStruct;
    iconStruct.uuid = entry->iconUuid();
    iconStruct.number = entry->iconNumber();
    m_iconsWidget->load(entry->uuid(), m_database, iconStruct);

    m_autoTypeUi->enableButton->setChecked(entry->autoTypeEnabled());
    if (entry->defaultAutoTypeSequence().isEmpty()) {
        m_autoTypeUi->inheritSequenceButton->setChecked(true);
        m_autoTypeUi->sequenceEdit->setText("");
    }
    else {
        m_autoTypeUi->customSequenceButton->setChecked(true);
        m_autoTypeUi->sequenceEdit->setText(entry->defaultAutoTypeSequence());
    }
    m_autoTypeUi->windowTitleCombo->lineEdit()->clear();
    m_autoTypeUi->defaultWindowSequenceButton->setChecked(true);
    m_autoTypeUi->windowSequenceEdit->setText("");
    m_autoTypeAssoc->copyDataFrom(entry->autoTypeAssociations());
    if (m_autoTypeAssoc->size() != 0) {
        m_autoTypeUi->assocView->setCurrentIndex(m_autoTypeAssocModel->index(0, 0));
    }
    if (!m_history) {
        m_autoTypeUi->windowTitleCombo->refreshWindowList();
    }
    updateAutoTypeEnabled();

    m_editWidgetProperties->setFields(entry->timeInfo(), entry->uuid());

    if (!m_history && !restore) {
        m_historyModel->setEntries(entry->historyItems());
        m_historyUi->historyView->sortByColumn(0, Qt::DescendingOrder);
    }
    if (m_historyModel->rowCount() > 0) {
        m_historyUi->deleteAllButton->setEnabled(true);
    }
    else {
        m_historyUi->deleteAllButton->setEnabled(false);
    }

    updateHistoryButtons(m_historyUi->historyView->currentIndex(), QModelIndex());

    m_mainUi->titleEdit->setFocus();
}

void EditEntryWidget::saveEntry()
{
    if (m_history) {
        m_entry = Q_NULLPTR;
        m_database = Q_NULLPTR;
        m_entryAttributes->clear();
        m_entryAttachments->clear();
        Q_EMIT editFinished(false);
        return;
    }

    if (!passwordsEqual()) {
        MessageBox::warning(this, tr("Error"), tr("Different passwords supplied."));
        return;
    }

    if (m_advancedUi->attributesView->currentIndex().isValid()) {
        QString key = m_attributesModel->keyByIndex(m_advancedUi->attributesView->currentIndex());
        m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(),
                               m_entryAttributes->isProtected(key));
    }

    m_currentAttribute = QPersistentModelIndex();

    // must stand before beginUpdate()
    // we don't want to create a new history item, if only the history has changed
    m_entry->removeHistoryItems(m_historyModel->deletedEntries());

    if (!m_create) {
        m_entry->beginUpdate();
    }

    m_entry->setTitle(m_mainUi->titleEdit->text());
    m_entry->setUsername(m_mainUi->usernameEdit->text());
    m_entry->setUrl(m_mainUi->urlEdit->text());
    m_entry->setPassword(m_mainUi->passwordEdit->text());
    m_entry->setExpires(m_mainUi->expireCheck->isChecked());
    m_entry->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

    m_entry->setNotes(m_mainUi->notesEdit->toPlainText());

    m_entry->attributes()->copyCustomKeysFrom(m_entryAttributes);
    m_entry->attachments()->copyDataFrom(m_entryAttachments);

    IconStruct iconStruct = m_iconsWidget->save();

    if (iconStruct.number < 0) {
        m_entry->setIcon(Entry::DefaultIconNumber);
    }
    else if (iconStruct.uuid.isNull()) {
        m_entry->setIcon(iconStruct.number);
    }
    else {
        m_entry->setIcon(iconStruct.uuid);
    }

    m_entry->setAutoTypeEnabled(m_autoTypeUi->enableButton->isChecked());
    if (m_autoTypeUi->inheritSequenceButton->isChecked()) {
        m_entry->setDefaultAutoTypeSequence(QString());
    }
    else {
        m_entry->setDefaultAutoTypeSequence(m_autoTypeUi->sequenceEdit->text());
    }

    m_autoTypeAssoc->removeEmpty();
    m_entry->autoTypeAssociations()->copyDataFrom(m_autoTypeAssoc);

    if (!m_create) {
        m_entry->endUpdate();
    }


    m_entry = Q_NULLPTR;
    m_database = Q_NULLPTR;
    m_entryAttributes->clear();
    m_entryAttachments->clear();
    m_autoTypeAssoc->clear();
    m_historyModel->clear();

    Q_EMIT editFinished(true);
}

void EditEntryWidget::cancel()
{
    if (m_history) {
        m_entry = Q_NULLPTR;
        m_database = Q_NULLPTR;
        m_entryAttributes->clear();
        m_entryAttachments->clear();
        Q_EMIT editFinished(false);
        return;
    }

    if (!m_entry->iconUuid().isNull() &&
            !m_database->metadata()->containsCustomIcon(m_entry->iconUuid())) {
        m_entry->setIcon(Entry::DefaultIconNumber);
    }

    m_entry = 0;
    m_database = 0;
    m_entryAttributes->clear();
    m_entryAttachments->clear();
    m_autoTypeAssoc->clear();
    m_historyModel->clear();

    Q_EMIT editFinished(false);
}

void EditEntryWidget::togglePasswordGeneratorButton(bool checked)
{
    m_mainUi->passwordGenerator->setVisible(checked);
}

bool EditEntryWidget::passwordsEqual()
{
    return m_mainUi->passwordEdit->text() == m_mainUi->passwordRepeatEdit->text();
}

void EditEntryWidget::setGeneratedPassword(const QString& password)
{
    m_mainUi->passwordEdit->setText(password);
    m_mainUi->passwordRepeatEdit->setText(password);

    m_mainUi->tooglePasswordGeneratorButton->setChecked(false);
}

void EditEntryWidget::insertAttribute()
{
    Q_ASSERT(!m_history);

    QString name = tr("New attribute");
    int i = 1;

    while (m_entryAttributes->keys().contains(name)) {
        name = QString("%1 %2").arg(tr("New attribute")).arg(i);
        i++;
    }

    m_entryAttributes->set(name, "");
    QModelIndex index = m_attributesModel->indexByKey(name);

    m_advancedUi->attributesView->setCurrentIndex(index);
    m_advancedUi->attributesView->edit(index);
}

void EditEntryWidget::editCurrentAttribute()
{
    Q_ASSERT(!m_history);

    QModelIndex index = m_advancedUi->attributesView->currentIndex();

    if (index.isValid()) {
        m_advancedUi->attributesView->edit(index);
    }
}

void EditEntryWidget::removeCurrentAttribute()
{
    Q_ASSERT(!m_history);

    QModelIndex index = m_advancedUi->attributesView->currentIndex();

    if (index.isValid()) {
        m_entryAttributes->remove(m_attributesModel->keyByIndex(index));
    }
}

void EditEntryWidget::updateCurrentAttribute()
{
    QModelIndex newIndex = m_advancedUi->attributesView->currentIndex();

    if (m_history) {
        if (newIndex.isValid()) {
            QString key = m_attributesModel->keyByIndex(newIndex);
            m_advancedUi->attributesEdit->setPlainText(m_entryAttributes->value(key));
            m_advancedUi->attributesEdit->setEnabled(true);
        }
        else {
            m_advancedUi->attributesEdit->setPlainText("");
            m_advancedUi->attributesEdit->setEnabled(false);
        }
    }
    else {
        if (m_currentAttribute != newIndex) {
            if (m_currentAttribute.isValid()) {
                QString key = m_attributesModel->keyByIndex(m_currentAttribute);
                m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(),
                                       m_entryAttributes->isProtected(key));
            }

            if (newIndex.isValid()) {
                QString key = m_attributesModel->keyByIndex(newIndex);
                m_advancedUi->attributesEdit->setPlainText(m_entryAttributes->value(key));
                m_advancedUi->attributesEdit->setEnabled(true);
            }
            else {
                m_advancedUi->attributesEdit->setPlainText("");
                m_advancedUi->attributesEdit->setEnabled(false);
            }

            m_advancedUi->editAttributeButton->setEnabled(newIndex.isValid());
            m_advancedUi->removeAttributeButton->setEnabled(newIndex.isValid());
            m_currentAttribute = newIndex;
        }
    }
}

void EditEntryWidget::insertAttachment()
{
    Q_ASSERT(!m_history);

    QString defaultDir = config()->get("LastAttachmentDir").toString();
    if (defaultDir.isEmpty() || !QDir(defaultDir).exists()) {
        defaultDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    }
    QString filename = fileDialog()->getOpenFileName(this, tr("Select file"), defaultDir);
    if (filename.isEmpty() || !QFile::exists(filename)) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        MessageBox::warning(this, tr("Error"),
                tr("Unable to open file").append(":\n").append(file.errorString()));
        return;
    }

    QByteArray data;
    if (!Tools::readAllFromDevice(&file, data)) {
        MessageBox::warning(this, tr("Error"),
                tr("Unable to open file").append(":\n").append(file.errorString()));
        return;
    }

    m_entryAttachments->set(QFileInfo(filename).fileName(), data);
}

void EditEntryWidget::saveCurrentAttachment()
{
    QModelIndex index = m_advancedUi->attachmentsView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString filename = m_attachmentsModel->keyByIndex(index);
    QString defaultDirName = config()->get("LastAttachmentDir").toString();
    if (defaultDirName.isEmpty() || !QDir(defaultDirName).exists()) {
        defaultDirName = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    }
    QDir dir(defaultDirName);
    QString savePath = fileDialog()->getSaveFileName(this, tr("Save attachment"),
                                                       dir.filePath(filename));
    if (!savePath.isEmpty()) {
        QByteArray attachmentData = m_entryAttachments->value(filename);

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            MessageBox::warning(this, tr("Error"),
                    tr("Unable to save the attachment:\n").append(file.errorString()));
            return;
        }
        if (file.write(attachmentData) != attachmentData.size()) {
            MessageBox::warning(this, tr("Error"),
                    tr("Unable to save the attachment:\n").append(file.errorString()));
            return;
        }
    }
}

void EditEntryWidget::removeCurrentAttachment()
{
    Q_ASSERT(!m_history);

    QModelIndex index = m_advancedUi->attachmentsView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString key = m_attachmentsModel->keyByIndex(index);
    m_entryAttachments->remove(key);
}

void EditEntryWidget::updateAutoTypeEnabled()
{
    bool autoTypeEnabled = m_autoTypeUi->enableButton->isChecked();
    bool validIndex = m_autoTypeUi->assocView->currentIndex().isValid() && m_autoTypeAssoc->size() != 0;

    m_autoTypeUi->enableButton->setEnabled(!m_history);
    m_autoTypeUi->inheritSequenceButton->setEnabled(!m_history && autoTypeEnabled);
    m_autoTypeUi->customSequenceButton->setEnabled(!m_history && autoTypeEnabled);
    m_autoTypeUi->sequenceEdit->setEnabled(autoTypeEnabled && m_autoTypeUi->customSequenceButton->isChecked());

    m_autoTypeUi->assocView->setEnabled(autoTypeEnabled);
    m_autoTypeUi->assocAddButton->setEnabled(!m_history);
    m_autoTypeUi->assocRemoveButton->setEnabled(!m_history && validIndex);

    m_autoTypeUi->windowTitleLabel->setEnabled(autoTypeEnabled && validIndex);
    m_autoTypeUi->windowTitleCombo->setEnabled(autoTypeEnabled && validIndex);
    m_autoTypeUi->defaultWindowSequenceButton->setEnabled(!m_history && autoTypeEnabled && validIndex);
    m_autoTypeUi->customWindowSequenceButton->setEnabled(!m_history && autoTypeEnabled && validIndex);
    m_autoTypeUi->windowSequenceEdit->setEnabled(autoTypeEnabled && validIndex
                                                 && m_autoTypeUi->customWindowSequenceButton->isChecked());
}

void EditEntryWidget::insertAutoTypeAssoc()
{
    AutoTypeAssociations::Association assoc;
    m_autoTypeAssoc->add(assoc);
    QModelIndex newIndex = m_autoTypeAssocModel->index(m_autoTypeAssoc->size() - 1, 0);
    m_autoTypeUi->assocView->setCurrentIndex(newIndex);
    loadCurrentAssoc(newIndex);
    m_autoTypeUi->windowTitleCombo->setFocus();
}

void EditEntryWidget::removeAutoTypeAssoc()
{
    QModelIndex currentIndex = m_autoTypeUi->assocView->currentIndex();

    if (currentIndex.isValid()) {
        m_autoTypeAssoc->remove(currentIndex.row());
    }
}

void EditEntryWidget::loadCurrentAssoc(const QModelIndex& current)
{
    if (current.isValid() && current.row() < m_autoTypeAssoc->size()) {
        AutoTypeAssociations::Association assoc = m_autoTypeAssoc->get(current.row());
        m_autoTypeUi->windowTitleCombo->setEditText(assoc.window);
        if (assoc.sequence.isEmpty()) {
            m_autoTypeUi->defaultWindowSequenceButton->setChecked(true);
        }
        else {
            m_autoTypeUi->customWindowSequenceButton->setChecked(true);
        }
        m_autoTypeUi->windowSequenceEdit->setText(assoc.sequence);

        updateAutoTypeEnabled();
    }
    else {
        clearCurrentAssoc();
    }
}

void EditEntryWidget::clearCurrentAssoc()
{
    m_autoTypeUi->windowTitleCombo->setEditText("");

    m_autoTypeUi->defaultWindowSequenceButton->setChecked(true);
    m_autoTypeUi->windowSequenceEdit->setText("");

    updateAutoTypeEnabled();
}

void EditEntryWidget::applyCurrentAssoc()
{
    QModelIndex index = m_autoTypeUi->assocView->currentIndex();

    if (!index.isValid() || m_autoTypeAssoc->size() == 0 || m_history) {
        return;
    }

    AutoTypeAssociations::Association assoc;
    assoc.window = m_autoTypeUi->windowTitleCombo->currentText();
    if (m_autoTypeUi->customWindowSequenceButton->isChecked()) {
        assoc.sequence = m_autoTypeUi->windowSequenceEdit->text();
    }

    m_autoTypeAssoc->update(index.row(), assoc);
}

void EditEntryWidget::showHistoryEntry()
{
    QModelIndex index = m_sortModel->mapToSource(m_historyUi->historyView->currentIndex());
    if (index.isValid()) {
        emitHistoryEntryActivated(index);
    }
}

void EditEntryWidget::restoreHistoryEntry()
{
    QModelIndex index = m_sortModel->mapToSource(m_historyUi->historyView->currentIndex());
    if (index.isValid()) {
        setForms(m_historyModel->entryFromIndex(index), true);
    }
}

void EditEntryWidget::deleteHistoryEntry()
{
    QModelIndex index = m_sortModel->mapToSource(m_historyUi->historyView->currentIndex());
    if (index.isValid()) {
        m_historyModel->deleteIndex(index);
        if (m_historyModel->rowCount() > 0) {
            m_historyUi->deleteAllButton->setEnabled(true);
        }
        else {
            m_historyUi->deleteAllButton->setEnabled(false);
        }
    }
}

void EditEntryWidget::deleteAllHistoryEntries()
{
    m_historyModel->deleteAll();
    if (m_historyModel->rowCount() > 0) {
        m_historyUi->deleteAllButton->setEnabled(true);
    }
    else {
        m_historyUi->deleteAllButton->setEnabled(false);
    }
}

QMenu* EditEntryWidget::createPresetsMenu()
{
    QMenu* expirePresetsMenu = new QMenu(this);
    expirePresetsMenu->addAction(tr("Tomorrow"))->setData(QVariant::fromValue(TimeDelta::fromDays(1)));
    expirePresetsMenu->addSeparator();
    expirePresetsMenu->addAction(tr("%n week(s)", 0, 1))->setData(QVariant::fromValue(TimeDelta::fromDays(7)));
    expirePresetsMenu->addAction(tr("%n week(s)", 0, 2))->setData(QVariant::fromValue(TimeDelta::fromDays(14)));
    expirePresetsMenu->addAction(tr("%n week(s)", 0, 3))->setData(QVariant::fromValue(TimeDelta::fromDays(21)));
    expirePresetsMenu->addSeparator();
    expirePresetsMenu->addAction(tr("%n month(s)", 0, 1))->setData(QVariant::fromValue(TimeDelta::fromMonths(1)));
    expirePresetsMenu->addAction(tr("%n month(s)", 0, 3))->setData(QVariant::fromValue(TimeDelta::fromMonths(3)));
    expirePresetsMenu->addAction(tr("%n month(s)", 0, 6))->setData(QVariant::fromValue(TimeDelta::fromMonths(6)));
    expirePresetsMenu->addSeparator();
    expirePresetsMenu->addAction(tr("1 year"))->setData(QVariant::fromValue(TimeDelta::fromYears(1)));
    return expirePresetsMenu;
}
