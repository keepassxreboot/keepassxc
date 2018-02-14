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
#include "ui_EditEntryWidgetAutoType.h"
#include "ui_EditEntryWidgetSSHAgent.h"
#include "ui_EditEntryWidgetHistory.h"
#include "ui_EditEntryWidgetMain.h"

#include <QDesktopServices>
#include <QStackedLayout>
#include <QStandardPaths>
#include <QMenu>
#include <QSortFilterProxyModel>
#include <QTemporaryFile>
#include <QMimeData>
#include <QEvent>

#include "autotype/AutoType.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/FilePath.h"
#include "core/Metadata.h"
#include "core/TimeDelta.h"
#include "core/Tools.h"
#ifdef WITH_XC_SSHAGENT
#include "sshagent/KeeAgentSettings.h"
#include "sshagent/OpenSSHKey.h"
#include "sshagent/SSHAgent.h"
#endif
#include "gui/EditWidgetIcons.h"
#include "gui/EditWidgetProperties.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "gui/Clipboard.h"
#include "gui/Font.h"
#include "gui/entry/AutoTypeAssociationsModel.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"
#include "gui/entry/EntryHistoryModel.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : EditWidget(parent)
    , m_entry(nullptr)
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_advancedUi(new Ui::EditEntryWidgetAdvanced())
    , m_autoTypeUi(new Ui::EditEntryWidgetAutoType())
    , m_sshAgentUi(new Ui::EditEntryWidgetSSHAgent())
    , m_historyUi(new Ui::EditEntryWidgetHistory())
    , m_mainWidget(new QWidget())
    , m_advancedWidget(new QWidget())
    , m_iconsWidget(new EditWidgetIcons())
    , m_autoTypeWidget(new QWidget())
    , m_sshAgentWidget(new QWidget())
    , m_editWidgetProperties(new EditWidgetProperties())
    , m_historyWidget(new QWidget())
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
#ifdef WITH_XC_SSHAGENT
    if (config()->get("SSHAgent", false).toBool()) {
        setupSSHAgent();
        m_sshAgentEnabled = true;
    } else {
        m_sshAgentEnabled = false;
    }
#endif
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

    m_advancedUi->attachmentsWidget->setReadOnly(false);
    m_advancedUi->attachmentsWidget->setButtonsVisible(true);

    connect(m_advancedUi->attachmentsWidget, &EntryAttachmentsWidget::errorOccurred, this, [this](const QString &error) {
        showMessage(error, MessageWidget::Error);
    });

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
    m_autoTypeUi->setupUi(m_autoTypeWidget);
    addPage(tr("Auto-Type"), FilePath::instance()->icon("actions", "key-enter"), m_autoTypeWidget);

    m_autoTypeDefaultSequenceGroup->addButton(m_autoTypeUi->inheritSequenceButton);
    m_autoTypeDefaultSequenceGroup->addButton(m_autoTypeUi->customSequenceButton);
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
    connect(m_autoTypeUi->customWindowSequenceButton, SIGNAL(toggled(bool)),
            SLOT(applyCurrentAssoc()));
    connect(m_autoTypeUi->windowSequenceEdit, SIGNAL(textChanged(QString)),
            SLOT(applyCurrentAssoc()));
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

#ifdef WITH_XC_SSHAGENT
void EditEntryWidget::setupSSHAgent()
{
    m_sshAgentUi->setupUi(m_sshAgentWidget);

    QFont fixedFont = Font::fixedFont();
    m_sshAgentUi->fingerprintTextLabel->setFont(fixedFont);
    m_sshAgentUi->commentTextLabel->setFont(fixedFont);
    m_sshAgentUi->publicKeyEdit->setFont(fixedFont);

    connect(m_sshAgentUi->attachmentRadioButton, SIGNAL(clicked(bool)), SLOT(updateSSHAgentKeyInfo()));
    connect(m_sshAgentUi->attachmentComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateSSHAgentAttachment()));
    connect(m_sshAgentUi->externalFileRadioButton, SIGNAL(clicked(bool)), SLOT(updateSSHAgentKeyInfo()));
    connect(m_sshAgentUi->externalFileEdit, SIGNAL(textChanged(QString)), SLOT(updateSSHAgentKeyInfo()));
    connect(m_sshAgentUi->browseButton, SIGNAL(clicked()), SLOT(browsePrivateKey()));
    connect(m_sshAgentUi->addToAgentButton, SIGNAL(clicked()), SLOT(addKeyToAgent()));
    connect(m_sshAgentUi->removeFromAgentButton, SIGNAL(clicked()), SLOT(removeKeyFromAgent()));
    connect(m_sshAgentUi->decryptButton, SIGNAL(clicked()), SLOT(decryptPrivateKey()));
    connect(m_sshAgentUi->copyToClipboardButton, SIGNAL(clicked()), SLOT(copyPublicKey()));

    addPage(tr("SSH Agent"), FilePath::instance()->icon("apps", "utilities-terminal"), m_sshAgentWidget);
}

void EditEntryWidget::updateSSHAgent()
{
    KeeAgentSettings settings;
    settings.fromXml(m_advancedUi->attachmentsWidget->getAttachment("KeeAgent.settings"));

    m_sshAgentUi->addKeyToAgentCheckBox->setChecked(settings.addAtDatabaseOpen());
    m_sshAgentUi->removeKeyFromAgentCheckBox->setChecked(settings.removeAtDatabaseClose());
    m_sshAgentUi->requireUserConfirmationCheckBox->setChecked(settings.useConfirmConstraintWhenAdding());
    m_sshAgentUi->lifetimeCheckBox->setChecked(settings.useLifetimeConstraintWhenAdding());
    m_sshAgentUi->lifetimeSpinBox->setValue(settings.lifetimeConstraintDuration());
    m_sshAgentUi->attachmentComboBox->clear();
    m_sshAgentUi->addToAgentButton->setEnabled(false);
    m_sshAgentUi->removeFromAgentButton->setEnabled(false);
    m_sshAgentUi->copyToClipboardButton->setEnabled(false);

    m_sshAgentUi->attachmentComboBox->addItem("");

    auto attachments = m_advancedUi->attachmentsWidget->entryAttachments();
    for (const QString& fileName : attachments->keys()) {
        if (fileName == "KeeAgent.settings") {
            continue;
        }

        m_sshAgentUi->attachmentComboBox->addItem(fileName);
    }

    m_sshAgentUi->attachmentComboBox->setCurrentText(settings.attachmentName());
    m_sshAgentUi->externalFileEdit->setText(settings.fileName());

    if (settings.selectedType() == "attachment") {
        m_sshAgentUi->attachmentRadioButton->setChecked(true);
    } else {
        m_sshAgentUi->externalFileRadioButton->setChecked(true);
    }

    m_sshAgentSettings = settings;

    updateSSHAgentKeyInfo();
}

void EditEntryWidget::updateSSHAgentAttachment()
{
    m_sshAgentUi->attachmentRadioButton->setChecked(true);
    updateSSHAgentKeyInfo();
}

void EditEntryWidget::updateSSHAgentKeyInfo()
{
    m_sshAgentUi->addToAgentButton->setEnabled(false);
    m_sshAgentUi->removeFromAgentButton->setEnabled(false);
    m_sshAgentUi->copyToClipboardButton->setEnabled(false);
    m_sshAgentUi->fingerprintTextLabel->setText(tr("n/a"));
    m_sshAgentUi->commentTextLabel->setText(tr("n/a"));
    m_sshAgentUi->decryptButton->setEnabled(false);
    m_sshAgentUi->publicKeyEdit->document()->setPlainText("");

    OpenSSHKey key;

    if (!getOpenSSHKey(key)) {
        return;
    }

    m_sshAgentUi->fingerprintTextLabel->setText(key.fingerprint());

    if (key.encrypted()) {
        m_sshAgentUi->commentTextLabel->setText(tr("(encrypted)"));
        m_sshAgentUi->decryptButton->setEnabled(true);
    } else {
        m_sshAgentUi->commentTextLabel->setText(key.comment());
    }

    m_sshAgentUi->publicKeyEdit->document()->setPlainText(key.publicKey());

    // enable agent buttons only if we have an agent running
    if (SSHAgent::instance()->isAgentRunning()) {
        m_sshAgentUi->addToAgentButton->setEnabled(true);
        m_sshAgentUi->removeFromAgentButton->setEnabled(true);
    }

    m_sshAgentUi->copyToClipboardButton->setEnabled(true);
}

void EditEntryWidget::saveSSHAgentConfig()
{
    KeeAgentSettings settings;
    QString privateKeyPath = m_sshAgentUi->attachmentComboBox->currentText();

    settings.setAddAtDatabaseOpen(m_sshAgentUi->addKeyToAgentCheckBox->isChecked());
    settings.setRemoveAtDatabaseClose(m_sshAgentUi->removeKeyFromAgentCheckBox->isChecked());
    settings.setUseConfirmConstraintWhenAdding(m_sshAgentUi->requireUserConfirmationCheckBox->isChecked());
    settings.setUseLifetimeConstraintWhenAdding(m_sshAgentUi->lifetimeCheckBox->isChecked());
    settings.setLifetimeConstraintDuration(m_sshAgentUi->lifetimeSpinBox->value());

    if (m_sshAgentUi->attachmentRadioButton->isChecked()) {
        settings.setSelectedType("attachment");
    } else {
        settings.setSelectedType("file");
    }
    settings.setAttachmentName(m_sshAgentUi->attachmentComboBox->currentText());
    settings.setFileName(m_sshAgentUi->externalFileEdit->text());

    // we don't use this as we don't run an agent but for compatibility we set it if necessary
    settings.setAllowUseOfSshKey(settings.addAtDatabaseOpen() || settings.removeAtDatabaseClose());

    // we don't use this either but we don't want it to dirty flag the config
    settings.setSaveAttachmentToTempFile(m_sshAgentSettings.saveAttachmentToTempFile());

    if (settings.isDefault()) {
        m_advancedUi->attachmentsWidget->removeAttachment("KeeAgent.settings");
    } else if (settings != m_sshAgentSettings) {
        m_advancedUi->attachmentsWidget->setAttachment("KeeAgent.settings", settings.toXml());
    }

    m_sshAgentSettings = settings;
}

void EditEntryWidget::browsePrivateKey()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select private key"), "");
    if (!fileName.isEmpty()) {
        m_sshAgentUi->externalFileEdit->setText(fileName);
        m_sshAgentUi->externalFileRadioButton->setChecked(true);
        updateSSHAgentKeyInfo();
    }
}

bool EditEntryWidget::getOpenSSHKey(OpenSSHKey& key)
{
    QByteArray privateKeyData;

    if (m_sshAgentUi->attachmentRadioButton->isChecked()) {
        privateKeyData = m_advancedUi->attachmentsWidget->getAttachment(m_sshAgentUi->attachmentComboBox->currentText());
    } else {
        QFile localFile(m_sshAgentUi->externalFileEdit->text());

        if (localFile.fileName().isEmpty()) {
            return false;
        }

        if (localFile.size() > 1024 * 1024) {
            showMessage(tr("File too large to be a private key"), MessageWidget::Error);
            return false;
        }

        if (!localFile.open(QIODevice::ReadOnly)) {
            showMessage(tr("Failed to open private key"), MessageWidget::Error);
            return false;
        }

        privateKeyData = localFile.readAll();
    }

    if (privateKeyData.length() == 0) {
        return false;
    }

    if (!key.parse(privateKeyData)) {
        showMessage(key.errorString(), MessageWidget::Error);
        return false;
    }

    if (key.comment().isEmpty()) {
        key.setComment(m_entry->username());
    }

    return true;
}

void EditEntryWidget::addKeyToAgent()
{
    OpenSSHKey key;

    if (!getOpenSSHKey(key)) {
        return;
    }

    if (!key.openPrivateKey(m_entry->password())) {
        showMessage(key.errorString(), MessageWidget::Error);
    } else {
        m_sshAgentUi->commentTextLabel->setText(key.comment());
        m_sshAgentUi->publicKeyEdit->document()->setPlainText(key.publicKey());
    }

    quint32 lifetime = 0;
    bool confirm = m_sshAgentUi->requireUserConfirmationCheckBox->isChecked();

    if (m_sshAgentUi->lifetimeCheckBox->isChecked()) {
        lifetime = m_sshAgentUi->lifetimeSpinBox->value();
    }

    SSHAgent::instance()->addIdentity(key, lifetime, confirm);

    if (m_sshAgentUi->removeKeyFromAgentCheckBox->isChecked()) {
        SSHAgent::instance()->removeIdentityAtLock(key, m_entry->uuid());
    }
}

void EditEntryWidget::removeKeyFromAgent()
{
    OpenSSHKey key;

    if (getOpenSSHKey(key)) {
        SSHAgent::instance()->removeIdentity(key);
    }
}

void EditEntryWidget::decryptPrivateKey()
{
    OpenSSHKey key;

    if (!getOpenSSHKey(key)) {
        return;
    }

    if (!key.openPrivateKey(m_entry->password())) {
        showMessage(key.errorString(), MessageWidget::Error);
    } else {
        m_sshAgentUi->commentTextLabel->setText(key.comment());
        m_sshAgentUi->publicKeyEdit->document()->setPlainText(key.publicKey());
    }
}

void EditEntryWidget::copyPublicKey()
{
    clipboard()->setText(m_sshAgentUi->publicKeyEdit->document()->toPlainText());
}
#endif

void EditEntryWidget::useExpiryPreset(QAction* action)
{
    m_mainUi->expireCheck->setChecked(true);
    TimeDelta delta = action->data().value<TimeDelta>();
    QDateTime now = QDateTime::currentDateTime();
    QDateTime expiryDateTime = now + delta;
    m_mainUi->expireDatePicker->setDateTime(expiryDateTime);
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
    m_saved = false;

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

    m_advancedUi->attachmentsWidget->setReadOnly(m_history);
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

    m_advancedUi->attachmentsWidget->setEntryAttachments(entry->attachments());
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

    m_autoTypeUi->enableButton->setChecked(entry->autoTypeEnabled());
    if (entry->defaultAutoTypeSequence().isEmpty()) {
        m_autoTypeUi->inheritSequenceButton->setChecked(true);
    }
    else {
        m_autoTypeUi->customSequenceButton->setChecked(true);
    }
    m_autoTypeUi->sequenceEdit->setText(entry->effectiveAutoTypeSequence());
    m_autoTypeUi->windowTitleCombo->lineEdit()->clear();
    m_autoTypeUi->customWindowSequenceButton->setChecked(false);
    m_autoTypeUi->windowSequenceEdit->setText("");
    m_autoTypeAssoc->copyDataFrom(entry->autoTypeAssociations());
    m_autoTypeAssocModel->setEntry(entry);
    if (m_autoTypeAssoc->size() != 0) {
        m_autoTypeUi->assocView->setCurrentIndex(m_autoTypeAssocModel->index(0, 0));
    }
    if (!m_history) {
        m_autoTypeUi->windowTitleCombo->refreshWindowList();
    }
    updateAutoTypeEnabled();

#ifdef WITH_XC_SSHAGENT
    if (m_sshAgentEnabled) {
        updateSSHAgent();
    }
#endif

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

    m_autoTypeAssoc->removeEmpty();

#ifdef WITH_XC_SSHAGENT
    if (m_sshAgentEnabled) {
        saveSSHAgentConfig();
    }
#endif

    if (!m_create) {
        m_entry->beginUpdate();
    }

    updateEntryData(m_entry);
    m_saved = true;

    if (!m_create) {
        m_entry->endUpdate();
    }

#ifdef WITH_XC_SSHAGENT
    if (m_sshAgentEnabled) {
        updateSSHAgent();
    }
#endif
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
    entry->attachments()->copyDataFrom(m_advancedUi->attachmentsWidget->entryAttachments());

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

    entry->setAutoTypeEnabled(m_autoTypeUi->enableButton->isChecked());
    if (m_autoTypeUi->inheritSequenceButton->isChecked()) {
        entry->setDefaultAutoTypeSequence(QString());
    }
    else {
        if (AutoType::verifyAutoTypeSyntax(m_autoTypeUi->sequenceEdit->text())) {
            entry->setDefaultAutoTypeSequence(m_autoTypeUi->sequenceEdit->text());
        }
    }

    entry->autoTypeAssociations()->copyDataFrom(m_autoTypeAssoc);
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

    emit editFinished(m_saved);
}

void EditEntryWidget::clear()
{
    m_entry = nullptr;
    m_database = nullptr;
    m_entryAttributes->clear();
    m_advancedUi->attachmentsWidget->clearAttachments();
    m_autoTypeAssoc->clear();
    m_historyModel->clear();
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
            m_autoTypeUi->customWindowSequenceButton->setChecked(false);
            m_autoTypeUi->windowSequenceEdit->setText(m_entry->effectiveAutoTypeSequence());
        } else {
            m_autoTypeUi->customWindowSequenceButton->setChecked(true);
            m_autoTypeUi->windowSequenceEdit->setText(assoc.sequence);
        }

        updateAutoTypeEnabled();
    } else {
        clearCurrentAssoc();
    }
}

void EditEntryWidget::clearCurrentAssoc()
{
    m_autoTypeUi->windowTitleCombo->setEditText("");

    m_autoTypeUi->customWindowSequenceButton->setChecked(false);
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
