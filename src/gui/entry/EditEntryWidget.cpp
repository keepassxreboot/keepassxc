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
#include "ui_EditEntryWidgetMain.h"
#include "ui_EditEntryWidgetNotes.h"
#include "ui_EditWidget.h"
#include "ui_EditWidgetIcons.h"

#include <QtGui/QDesktopServices>
#include <QtGui/QStackedLayout>
#include <QtGui/QMessageBox>

#include "core/Entry.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/EditWidgetIcons.h"
#include "gui/FileDialog.h"
#include "gui/entry/EntryAttachmentsModel.h"
#include "gui/entry/EntryAttributesModel.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : EditWidget(parent)
    , m_entry(0)
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_notesUi(new Ui::EditEntryWidgetNotes())
    , m_advancedUi(new Ui::EditEntryWidgetAdvanced())
    , m_mainWidget(new QWidget())
    , m_notesWidget(new QWidget())
    , m_advancedWidget(new QWidget())
    , m_iconsWidget(new EditWidgetIcons())
{
    QFont headerLabelFont = headlineLabel()->font();
    headerLabelFont.setBold(true);
    headerLabelFont.setPointSize(headerLabelFont.pointSize() + 2);
    headlineLabel()->setFont(headerLabelFont);

    m_mainUi->setupUi(m_mainWidget);
    add(tr("Entry"), m_mainWidget);

    m_notesUi->setupUi(m_notesWidget);
    add(tr("Description"), m_notesWidget);

    m_advancedUi->setupUi(m_advancedWidget);
    add(tr("Advanced"), m_advancedWidget);

    add(tr("Icon"), m_iconsWidget);

    m_entryAttachments = new EntryAttachments(this);
    m_attachmentsModel = new EntryAttachmentsModel(m_advancedWidget);
    m_attachmentsModel->setEntryAttachments(m_entryAttachments);
    m_advancedUi->attachmentsView->setModel(m_attachmentsModel);
    connect(m_advancedUi->saveAttachmentButton, SIGNAL(clicked()), SLOT(saveCurrentAttachment()));
    connect(m_advancedUi->addAttachmentButton, SIGNAL(clicked()), SLOT(insertAttachment()));
    connect(m_advancedUi->removeAttachmentButton, SIGNAL(clicked()), SLOT(removeCurrentAttachment()));

    m_entryAttributes = new EntryAttributes(this);
    m_attributesModel = new EntryAttributesModel(m_advancedWidget);
    m_attributesModel->setEntryAttributes(m_entryAttributes);
    m_advancedUi->attributesView->setModel(m_attributesModel);
    connect(m_advancedUi->addAttributeButton, SIGNAL(clicked()), SLOT(insertAttribute()));
    connect(m_advancedUi->editAttributeButton, SIGNAL(clicked()), SLOT(editCurrentAttribute()));
    connect(m_advancedUi->removeAttributeButton, SIGNAL(clicked()), SLOT(removeCurrentAttribute()));
    connect(m_advancedUi->attributesView->selectionModel(),
            SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(updateCurrentAttribute()));

    connect(m_mainUi->togglePasswordButton, SIGNAL(toggled(bool)), SLOT(togglePassword(bool)));
    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));
    connect(m_mainUi->passwordEdit, SIGNAL(textEdited(QString)), SLOT(setPasswordCheckColors()));
    connect(m_mainUi->passwordRepeatEdit, SIGNAL(textEdited(QString)), SLOT(setPasswordCheckColors()));

    connect(this, SIGNAL(accepted()), SLOT(saveEntry()));
    connect(this, SIGNAL(rejected()), SLOT(cancel()));
}

EditEntryWidget::~EditEntryWidget()
{
}

const QColor EditEntryWidget::CorrectSoFarColor = QColor(255, 205, 15);
const QColor EditEntryWidget::ErrorColor = QColor(255, 125, 125);

void EditEntryWidget::loadEntry(Entry* entry, bool create, const QString& groupName,
                                Database* database)
{
    m_entry = entry;
    m_database = database;
    m_create = create;

    if (create) {
        headlineLabel()->setText(groupName+" > "+tr("Add entry"));
    }
    else {
        headlineLabel()->setText(groupName+" > "+tr("Edit entry"));
    }

    m_mainUi->titleEdit->setText(entry->title());
    m_mainUi->usernameEdit->setText(entry->username());
    m_mainUi->urlEdit->setText(entry->url());
    m_mainUi->passwordEdit->setText(entry->password());
    m_mainUi->passwordRepeatEdit->setText(entry->password());
    setPasswordCheckColors();
    m_mainUi->expireCheck->setChecked(entry->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(entry->timeInfo().expiryTime().toLocalTime());
    m_mainUi->togglePasswordButton->setChecked(true);

    m_notesUi->notesEdit->setPlainText(entry->notes());

    m_entryAttributes->copyCustomKeysFrom(entry->attributes());
    *m_entryAttachments = *entry->attachments();

    setCurrentRow(0);

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
    m_iconsWidget->load(entry->uuid(), database, iconStruct);

    m_mainUi->titleEdit->setFocus();
}

void EditEntryWidget::saveEntry()
{
    if (!passwordsEqual()) {
        QMessageBox::warning(this, tr("Error"), tr("Different passwords supplied."));
        return;
    }

    if (m_advancedUi->attributesView->currentIndex().isValid()) {
        QString key = m_attributesModel->keyByIndex(m_advancedUi->attributesView->currentIndex());
        m_entryAttributes->set(key, m_advancedUi->attributesEdit->toPlainText(),
                               m_entryAttributes->isProtected(key));
    }

    m_currentAttribute = QPersistentModelIndex();

    if (!m_create) {
        m_entry->beginUpdate();
    }

    m_entry->setTitle(m_mainUi->titleEdit->text());
    m_entry->setUsername(m_mainUi->usernameEdit->text());
    m_entry->setUrl(m_mainUi->urlEdit->text());
    m_entry->setPassword(m_mainUi->passwordEdit->text());
    m_entry->setExpires(m_mainUi->expireCheck->isChecked());
    m_entry->setExpiryTime(m_mainUi->expireDatePicker->dateTime().toUTC());

    m_entry->setNotes(m_notesUi->notesEdit->toPlainText());

    m_entry->attributes()->copyCustomKeysFrom(m_entryAttributes);
    *m_entry->attachments() = *m_entryAttachments;

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

    if (!m_create) {
        m_entry->endUpdate();
    }

    m_entry = 0;
    m_database = 0;
    m_entryAttributes->clear();
    m_entryAttachments->clear();

    Q_EMIT editFinished(true);
}

void EditEntryWidget::cancel()
{
    if (!m_entry->iconUuid().isNull() &&
            !m_database->metadata()->containsCustomIcon(m_entry->iconUuid())) {
        m_entry->setIcon(Entry::DefaultIconNumber);
    }

    m_entry = 0;
    m_database = 0;
    m_entryAttributes->clear();
    m_entryAttachments->clear();

    Q_EMIT editFinished(false);
}

void EditEntryWidget::togglePassword(bool checked)
{
    m_mainUi->passwordEdit->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
    m_mainUi->passwordRepeatEdit->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}

bool EditEntryWidget::passwordsEqual()
{
    return m_mainUi->passwordEdit->text() == m_mainUi->passwordRepeatEdit->text();
}

void EditEntryWidget::setPasswordCheckColors()
{
    if (passwordsEqual()) {
        m_mainUi->passwordRepeatEdit->setStyleSheet("");
    }
    else {
        QString stylesheet = "QLineEdit { background: %1; }";

        if (m_mainUi->passwordEdit->text().startsWith(m_mainUi->passwordRepeatEdit->text())) {
            stylesheet = stylesheet.arg(CorrectSoFarColor.name());
        }
        else {
            stylesheet = stylesheet.arg(ErrorColor.name());
        }

        m_mainUi->passwordRepeatEdit->setStyleSheet(stylesheet);
    }
}

void EditEntryWidget::insertAttribute()
{
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
    QModelIndex index = m_advancedUi->attributesView->currentIndex();

    if (index.isValid()) {
        m_advancedUi->attributesView->edit(index);
    }
}

void EditEntryWidget::removeCurrentAttribute()
{
    QModelIndex index = m_advancedUi->attributesView->currentIndex();

    if (index.isValid()) {
        m_entryAttributes->remove(m_attributesModel->keyByIndex(index));
    }
}

void EditEntryWidget::updateCurrentAttribute()
{
    QModelIndex newIndex = m_advancedUi->attributesView->currentIndex();

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

void EditEntryWidget::insertAttachment()
{
    // TODO: save last used dir
    QString filename = fileDialog()->getOpenFileName(this, tr("Select file"),
                QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
    if (filename.isEmpty() || !QFile::exists(filename)) {
        return;
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error"),
                tr("Unable to open file:\n").append(file.errorString()));
        return;
    }

    QByteArray data;
    if (!Tools::readAllFromDevice(&file, data)) {
        QMessageBox::warning(this, tr("Error"),
                tr("Unable to open file:\n").append(file.errorString()));
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
    // TODO: save last used dir
    QDir dir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation));
    QString savePath = fileDialog()->getSaveFileName(this, tr("Save attachment"),
                                                       dir.filePath(filename));
    if (!savePath.isEmpty()) {
        QByteArray attachmentData = m_entryAttachments->value(filename);

        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Error"),
                    tr("Unable to save the attachment:\n").append(file.errorString()));
            return;
        }
        if (file.write(attachmentData) != attachmentData.size()) {
            QMessageBox::warning(this, tr("Error"),
                    tr("Unable to save the attachment:\n").append(file.errorString()));
            return;
        }
    }
}

void EditEntryWidget::removeCurrentAttachment()
{
    QModelIndex index = m_advancedUi->attachmentsView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QString key = m_attachmentsModel->keyByIndex(index);
    m_entryAttachments->remove(key);
}
