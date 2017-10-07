/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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
#include "ui_EditEntryWidgetHistory.h"
#include "ui_EditEntryWidgetMain.h"

#include <QDesktopServices>
#include <QStackedLayout>
#include <QStandardPaths>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTemporaryFile>

#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/FilePath.h"
#include "core/Metadata.h"
#include "core/TimeDelta.h"
#include "core/Tools.h"
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetAutoType.h"
#include "gui/EditWidgetProperties.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryHistoryModel.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : EditWidget(parent)
    , m_entry(nullptr)
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_advancedUi(new Ui::EditEntryWidgetAdvanced())
    , m_historyUi(new Ui::EditEntryWidgetHistory())
    , m_mainWidget(new QWidget())
    , m_advancedWidget(new QWidget())
    , m_iconsWidget(new EditWidgetIcons())
    , m_editWidgetAutoType(new EditWidgetAutoType())
    , m_editWidgetProperties(new EditWidgetProperties())
    , m_historyWidget(new QWidget())
    , m_entryAttachments(new EntryAttachments(this))
    , m_attachmentsModel(new EntryAttachmentsModel(m_advancedWidget))
    , m_entryAttributes(new EntryAttributes(this))
    , m_attributesModel(new EntryAttributesModel(m_advancedWidget))
    , m_historyModel(new EntryHistoryModel(this))
    , m_sortModel(new QSortFilterProxyModel(this))
{
    setupMain();
    setupAdvanced();
    setupIcon();
    setupAutoType();
    setupProperties();
    setupHistory();

    connect(this, SIGNAL(accepted()), SLOT(acceptEntry()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));
    connect(this, SIGNAL(apply()), SLOT(saveEntry()));
    connect(m_iconsWidget, SIGNAL(messageEditEntry(QString, MessageWidget::MessageType)), SLOT(showMessage(QString, MessageWidget::MessageType)));
    connect(m_iconsWidget, SIGNAL(messageEditEntryDismiss()), SLOT(hideMessage()));
    
    m_mainUi->passwordGenerator->layout()->setContentsMargins(0, 0, 0, 0);
}

EditEntryWidget::~EditEntryWidget()
{
}

void EditEntryWidget::setupMain()
{
    m_mainUi->setupUi(m_mainWidget);
    addPage(tr("Entry"), FilePath::instance()->icon("actions", "document-edit"), m_mainWidget);

    m_mainUi->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    m_mainUi->togglePasswordGeneratorButton->setIcon(filePath()->icon("actions", "password-generator", false));
    connect(m_mainUi->togglePasswordButton, SIGNAL(toggled(bool)), m_mainUi->passwordEdit, SLOT(setShowPassword(bool)));
    connect(m_mainUi->togglePasswordGeneratorButton, SIGNAL(toggled(bool)), SLOT(togglePasswordGeneratorButton(bool)));
    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));
    connect(m_mainUi->notesEnabled, SIGNAL(toggled(bool)), this, SLOT(toggleHideNotes(bool)));
    m_mainUi->passwordRepeatEdit->enableVerifyMode(m_mainUi->passwordEdit);
    connect(m_mainUi->passwordGenerator, SIGNAL(appliedPassword(QString)), SLOT(setGeneratedPassword(QString)));

    m_mainUi->expirePresets->setMenu(createPresetsMenu());
    connect(m_mainUi->expirePresets->menu(), SIGNAL(triggered(QAction*)), this, SLOT(useExpiryPreset(QAction*)));

    QAction *action = new QAction(this);
    action->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(action, SIGNAL(triggered()), this, SLOT(saveEntry()));
    this->addAction(action);

    m_mainUi->passwordGenerator->hide();
    m_mainUi->passwordGenerator->reset();
}

void EditEntryWidget::setupAdvanced()
{
    m_advancedUi->setupUi(m_advancedWidget);
    addPage(tr("Advanced"), FilePath::instance()->icon("categories", "preferences-other"), m_advancedWidget);

    m_attachmentsModel->setEntryAttachments(m_entryAttachments);
    m_advancedUi->attachmentsView->setModel(m_attachmentsModel);
    connect(m_advancedUi->attachmentsView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateAttachmentButtonsEnabled(QModelIndex)));
    connect(m_advancedUi->attachmentsView, SIGNAL(doubleClicked(QModelIndex)), SLOT(openAttachment(QModelIndex)));
    connect(m_advancedUi->saveAttachmentButton, SIGNAL(clicked()), SLOT(saveCurrentAttachment()));
    connect(m_advancedUi->openAttachmentButton, SIGNAL(clicked()), SLOT(openCurrentAttachment()));
    connect(m_advancedUi->addAttachmentButton, SIGNAL(clicked()), SLOT(insertAttachment()));
    connect(m_advancedUi->removeAttachmentButton, SIGNAL(clicked()), SLOT(removeCurrentAttachment()));

    m_attributesModel->setEntryAttributes(m_entryAttributes);
    m_advancedUi->attributesView->setModel(m_attributesModel);
    connect(m_advancedUi->addAttributeButton, SIGNAL(clicked()), SLOT(insertAttribute()));
    connect(m_advancedUi->editAttributeButton, SIGNAL(clicked()), SLOT(editCurrentAttribute()));
    connect(m_advancedUi->removeAttributeButton, SIGNAL(clicked()), SLOT(removeCurrentAttribute()));
    connect(m_advancedUi->protectAttributeButton, SIGNAL(toggled(bool)), SLOT(protectCurrentAttribute(bool)));
    connect(m_advancedUi->revealAttributeButton, SIGNAL(clicked(bool)), SLOT(revealCurrentAttribute()));
    connect(m_advancedUi->attributesView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateCurrentAttribute()));
}

void EditEntryWidget::setupIcon()
{
    addPage(tr("Icon"), FilePath::instance()->icon("apps", "preferences-desktop-icons"), m_iconsWidget);
}

void EditEntryWidget::setupAutoType()
{
    addPage(tr("Auto-Type"), FilePath::instance()->icon("actions", "key-enter"), m_editWidgetAutoType);
}

void EditEntryWidget::setupProperties()
{
    addPage(tr("Properties"), FilePath::instance()->icon("actions", "document-properties"), m_editWidgetProperties);
}

void EditEntryWidget::setupHistory()
{
    m_historyUi->setupUi(m_historyWidget);
    addPage(tr("History"), FilePath::instance()->icon("actions", "view-history"), m_historyWidget);

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
    emit historyEntryActivated(entry);
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
    QDateTime now = QDateTime::currentDateTime();
    QDateTime expiryDateTime = now + delta;
    m_mainUi->expireDatePicker->setDateTime(expiryDateTime);
}

void EditEntryWidget::updateAttachmentButtonsEnabled(const QModelIndex& current)
{
    bool enable = current.isValid();

    m_advancedUi->saveAttachmentButton->setEnabled(enable);
    m_advancedUi->openAttachmentButton->setEnabled(enable);
    m_advancedUi->removeAttachmentButton->setEnabled(enable && !m_history);
}

void EditEntryWidget::toggleHideNotes(bool visible)
{
    m_mainUi->notesEdit->setVisible(visible);
    m_mainUi->notesHint->setVisible(!visible);
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
            setHeadline(QString("%1 > %2 > %3").arg(parentName,
                                                    entry->title(), tr("Edit entry")));
        }
    }

    setForms(entry);
    setReadOnly(m_history);

    setCurrentPage(0);
    setPageHidden(m_historyWidget, m_history || m_entry->historyItems().count() < 1);
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
    m_mainUi->notesEnabled->setChecked(!config()->get("security/hidenotes").toBool());
    m_mainUi->notesEdit->setReadOnly(m_history);
    m_mainUi->notesEdit->setVisible(!config()->get("security/hidenotes").toBool());
    m_mainUi->notesHint->setVisible(config()->get("security/hidenotes").toBool());
    m_mainUi->togglePasswordGeneratorButton->setChecked(false);
    m_mainUi->togglePasswordGeneratorButton->setDisabled(m_history);
    m_mainUi->passwordGenerator->reset();
    m_advancedUi->addAttachmentButton->setEnabled(!m_history);
    updateAttachmentButtonsEnabled(m_advancedUi->attachmentsView->currentIndex());
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
    m_editWidgetAutoType->setHistory(m_history);
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

    QList<int> sizes = m_advancedUi->attributesSplitter->sizes();
    sizes.replace(0, m_advancedUi->attributesSplitter->width() * 0.3);
    sizes.replace(1, m_advancedUi->attributesSplitter->width() * 0.7);
    m_advancedUi->attributesSplitter->setSizes(sizes);

    IconStruct iconStruct;
    iconStruct.uuid = entry->iconUuid();
    iconStruct.number = entry->iconNumber();
    m_iconsWidget->load(entry->uuid(), m_database, iconStruct, entry->webUrl());
    connect(m_mainUi->urlEdit, SIGNAL(textChanged(QString)), m_iconsWidget, SLOT(setUrl(QString)));

    m_editWidgetAutoType->setFields(entry->autoTypeEnabled(), entry->group()->effectiveAutoTypeEnabled(),
                                             entry->defaultAutoTypeSequence(), entry->effectiveAutoTypeSequence(),
                                             entry->autoTypeAssociations());

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
        clear();
        hideMessage();
        emit editFinished(false);
        return;
    }

    if (!passwordsEqual()) {
        showMessage(tr("Different passwords supplied."), MessageWidget::Error);
        return;
    }

    if (m_advancedUi->attributesView->currentIndex().isValid() && m_advancedUi->attributesEdit->isEnabled()) {
        QString key = m_attributesModel->keyByIndex(m_advancedUi->attributesView->currentIndex());
        m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(),
                               m_entryAttributes->isProtected(key));
    }

    m_currentAttribute = QPersistentModelIndex();

    // must stand before beginUpdate()
    // we don't want to create a new history item, if only the history has changed
    m_entry->removeHistoryItems(m_historyModel->deletedEntries());
    m_historyModel->clearDeletedEntries();

    m_editWidgetAutoType->removeEmptyAssocs();

    if (!m_create) {
        m_entry->beginUpdate();
    }

    updateEntryData(m_entry);

    if (!m_create) {
        m_entry->endUpdate();
    }
}

void EditEntryWidget::acceptEntry()
{
    // Check if passwords are mismatched first to prevent saving
    if (!passwordsEqual()) {
        showMessage(tr("Different passwords supplied."), MessageWidget::Error);
        return;
    }

    saveEntry();
    clear();
    emit editFinished(true);
}

void EditEntryWidget::updateEntryData(Entry* entry) const
{
    entry->attributes()->copyCustomKeysFrom(m_entryAttributes);
    entry->attachments()->copyDataFrom(m_entryAttachments);
    
    entry->setTitle(m_mainUi->titleEdit->text());
    entry->setUsername(m_mainUi->usernameEdit->text());
    entry->setUrl(m_mainUi->urlEdit->text());
    entry->setPassword(m_mainUi->passwordEdit->text());
    entry->setExpires(m_mainUi->expireCheck->isChecked());
    entry->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

    entry->setNotes(m_mainUi->notesEdit->toPlainText());

    IconStruct iconStruct = m_iconsWidget->state();

    if (iconStruct.number < 0) {
        entry->setIcon(Entry::DefaultIconNumber);
    }
    else if (iconStruct.uuid.isNull()) {
        entry->setIcon(iconStruct.number);
    }
    else {
        entry->setIcon(iconStruct.uuid);
    }

    entry->setAutoTypeEnabled(m_editWidgetAutoType->autoTypeEnabled());
    entry->setDefaultAutoTypeSequence(m_editWidgetAutoType->inheritSequenceEnabled() ?
                                          QString() : m_editWidgetAutoType->sequence());
    entry->autoTypeAssociations()->copyDataFrom(m_editWidgetAutoType->autoTypeAssociations());
}

void EditEntryWidget::cancel()
{
    if (m_history) {
        clear();
        hideMessage();
        emit editFinished(false);
        return;
    }

    if (!m_entry->iconUuid().isNull() &&
            !m_database->metadata()->containsCustomIcon(m_entry->iconUuid())) {
        m_entry->setIcon(Entry::DefaultIconNumber);
    }

    clear();

    emit editFinished(false);
}

void EditEntryWidget::clear()
{
    m_entry = nullptr;
    m_database = nullptr;
    m_entryAttributes->clear();
    m_entryAttachments->clear();
    m_historyModel->clear();
    m_editWidgetAutoType->clear();
    m_iconsWidget->reset();
    hideMessage();
}

bool EditEntryWidget::hasBeenModified() const
{
    // entry has been modified if a history item is to be deleted
    if (!m_historyModel->deletedEntries().isEmpty()) {
        return true;
    }

    // check if updating the entry would modify it
    QScopedPointer<Entry> entry(new Entry());
    entry->copyDataFrom(m_entry);

    entry->beginUpdate();
    updateEntryData(entry.data());
    return entry->endUpdate();
}

void EditEntryWidget::togglePasswordGeneratorButton(bool checked)
{
    m_mainUi->passwordGenerator->regeneratePassword();
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

    m_mainUi->togglePasswordGeneratorButton->setChecked(false);
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
        if (MessageBox::question(this, tr("Confirm Remove"), tr("Are you sure you want to remove this attribute?"),
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            m_entryAttributes->remove(m_attributesModel->keyByIndex(index));
        }
    }
}

void EditEntryWidget::updateCurrentAttribute()
{
    QModelIndex newIndex = m_advancedUi->attributesView->currentIndex();
    QString newKey = m_attributesModel->keyByIndex(newIndex);

    if (!m_history && m_currentAttribute != newIndex) {
        // Save changes to the currently selected attribute if editing is enabled
        if (m_currentAttribute.isValid() && m_advancedUi->attributesEdit->isEnabled()) {
            QString currKey = m_attributesModel->keyByIndex(m_currentAttribute);
            m_entryAttributes->set(currKey, m_advancedUi->attributesEdit->toPlainText(),
                                   m_entryAttributes->isProtected(currKey));
        }        
    }

    displayAttribute(newIndex, m_entryAttributes->isProtected(newKey));

    m_currentAttribute = newIndex;
}

void EditEntryWidget::displayAttribute(QModelIndex index, bool showProtected)
{
    // Block signals to prevent extra calls
    m_advancedUi->protectAttributeButton->blockSignals(true);

    if (index.isValid()) {
        QString key = m_attributesModel->keyByIndex(index);
        if (showProtected) {
            m_advancedUi->attributesEdit->setPlainText(tr("[PROTECTED]") + " " + tr("Press reveal to view or edit"));
            m_advancedUi->attributesEdit->setEnabled(false);
            m_advancedUi->revealAttributeButton->setEnabled(true);
            m_advancedUi->protectAttributeButton->setChecked(true);
        }
        else {
            m_advancedUi->attributesEdit->setPlainText(m_entryAttributes->value(key));
            m_advancedUi->attributesEdit->setEnabled(true);
            m_advancedUi->revealAttributeButton->setEnabled(false);
            m_advancedUi->protectAttributeButton->setChecked(false);
        }

        // Don't allow editing in history view
        m_advancedUi->protectAttributeButton->setEnabled(!m_history);
        m_advancedUi->editAttributeButton->setEnabled(!m_history);
        m_advancedUi->removeAttributeButton->setEnabled(!m_history);
    }
    else {
        m_advancedUi->attributesEdit->setPlainText("");
        m_advancedUi->attributesEdit->setEnabled(false);
        m_advancedUi->revealAttributeButton->setEnabled(false);
        m_advancedUi->protectAttributeButton->setChecked(false);
        m_advancedUi->protectAttributeButton->setEnabled(false);
        m_advancedUi->editAttributeButton->setEnabled(false);
        m_advancedUi->removeAttributeButton->setEnabled(false);
    }

    m_advancedUi->protectAttributeButton->blockSignals(false);
}

void EditEntryWidget::protectCurrentAttribute(bool state)
{
    QModelIndex index = m_advancedUi->attributesView->currentIndex();
    if (!m_history && index.isValid()) {
        QString key = m_attributesModel->keyByIndex(index);
        if (state) {
            // Save the current text and protect the attribute
            m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(), true);
        } else {
            // Unprotect the current attribute value (don't save text as it is obscured)
            m_entryAttributes->set(key, m_entryAttributes->value(key), false);
        }

        // Display the attribute
        displayAttribute(index, state);
    }
}

void EditEntryWidget::revealCurrentAttribute()
{
    if (! m_advancedUi->attributesEdit->isEnabled()) {
        QModelIndex index = m_advancedUi->attributesView->currentIndex();
        if (index.isValid()) {
            QString key = m_attributesModel->keyByIndex(index);
            m_advancedUi->attributesEdit->setPlainText(m_entryAttributes->value(key));
            m_advancedUi->attributesEdit->setEnabled(true);
        }
    }
}

void EditEntryWidget::insertAttachment()
{
    Q_ASSERT(!m_history);

    QString defaultDir = config()->get("LastAttachmentDir").toString();
    if (defaultDir.isEmpty() || !QDir(defaultDir).exists()) {
        defaultDir = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).value(0);
    }
    QString filename = fileDialog()->getOpenFileName(this, tr("Select file"), defaultDir);
    if (filename.isEmpty() || !QFile::exists(filename)) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        showMessage(tr("Unable to open file").append(":\n").append(file.errorString()), MessageWidget::Error);
        return;
    }

    QByteArray data;
    if (!Tools::readAllFromDevice(&file, data)) {
        showMessage(tr("Unable to open file").append(":\n").append(file.errorString()), MessageWidget::Error);
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
        defaultDirName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    QDir dir(defaultDirName);
    QString savePath = fileDialog()->getSaveFileName(this, tr("Save attachment"),
                                                       dir.filePath(filename));
    if (!savePath.isEmpty()) {
        QByteArray attachmentData = m_entryAttachments->value(filename);

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            showMessage(tr("Unable to save the attachment:\n").append(file.errorString()), MessageWidget::Error);
            return;
        }
        if (file.write(attachmentData) != attachmentData.size()) {
            showMessage(tr("Unable to save the attachment:\n").append(file.errorString()), MessageWidget::Error);
            return;
        }
    }
}

void EditEntryWidget::openAttachment(const QModelIndex& index)
{
    if (!index.isValid()) {
        Q_ASSERT(false);
        return;
    }

    QString filename = m_attachmentsModel->keyByIndex(index);
    QByteArray attachmentData = m_entryAttachments->value(filename);

    // tmp file will be removed once the database (or the application) has been closed
    QString tmpFileTemplate = QDir::temp().absoluteFilePath(QString("XXXXXX.").append(filename));
    QTemporaryFile* file = new QTemporaryFile(tmpFileTemplate, this);

    if (!file->open()) {
        showMessage(tr("Unable to save the attachment:\n").append(file->errorString()), MessageWidget::Error);
        return;
    }

    if (file->write(attachmentData) != attachmentData.size()) {
        showMessage(tr("Unable to save the attachment:\n").append(file->errorString()), MessageWidget::Error);
        return;
    }

    if (!file->flush()) {
        showMessage(tr("Unable to save the attachment:\n").append(file->errorString()), MessageWidget::Error);
        return;
    }

    file->close();

    QDesktopServices::openUrl(QUrl::fromLocalFile(file->fileName()));
}

void EditEntryWidget::openCurrentAttachment()
{
    QModelIndex index = m_advancedUi->attachmentsView->currentIndex();

    openAttachment(index);
}

void EditEntryWidget::removeCurrentAttachment()
{
    Q_ASSERT(!m_history);

    QModelIndex index = m_advancedUi->attachmentsView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QMessageBox::StandardButton ans = MessageBox::question(this, tr("Confirm Remove"),
                                                           tr("Are you sure you want to remove this attachment?"),
                                                           QMessageBox::Yes | QMessageBox::No);
    if (ans == QMessageBox::Yes) {
        QString key = m_attachmentsModel->keyByIndex(index);
        m_entryAttachments->remove(key);
    }
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
        } else {
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
