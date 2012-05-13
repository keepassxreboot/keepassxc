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
#include "ui_EditEntryWidget.h"
#include "ui_EditEntryWidgetAdvanced.h"
#include "ui_EditEntryWidgetMain.h"
#include "ui_EditEntryWidgetNotes.h"
#include "ui_EditEntryWidgetIcons.h"

#include <QtGui/QDesktopServices>
#include <QtGui/QStackedLayout>
#include <QtGui/QMessageBox>

#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/IconModels.h"
#include "gui/EntryAttachmentsModel.h"
#include "gui/EntryAttributesModel.h"
#include "gui/FileDialog.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_entry(0)
    , m_ui(new Ui::EditEntryWidget())
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_notesUi(new Ui::EditEntryWidgetNotes())
    , m_advancedUi(new Ui::EditEntryWidgetAdvanced())
    , m_iconsUi(new Ui::EditEntryWidgetIcons())
    , m_mainWidget(new QWidget(this))
    , m_notesWidget(new QWidget(this))
    , m_advancedWidget(new QWidget(this))
    , m_iconsWidget(new QWidget(this))
{
    m_ui->setupUi(this);

    QFont headerLabelFont = m_ui->headerLabel->font();
    headerLabelFont.setBold(true);
    headerLabelFont.setPointSize(headerLabelFont.pointSize() + 2);
    m_ui->headerLabel->setFont(headerLabelFont);

    m_ui->categoryList->addItem(tr("Entry"));
    m_ui->categoryList->addItem(tr("Description"));
    m_ui->categoryList->addItem(tr("Advanced"));
    m_ui->categoryList->addItem(tr("Icon"));

    m_mainUi->setupUi(m_mainWidget);
    m_ui->stackedWidget->addWidget(m_mainWidget);

    m_notesUi->setupUi(m_notesWidget);
    m_ui->stackedWidget->addWidget(m_notesWidget);

    m_advancedUi->setupUi(m_advancedWidget);
    m_ui->stackedWidget->addWidget(m_advancedWidget);

    m_iconsUi->setupUi(m_iconsWidget);
    m_ui->stackedWidget->addWidget(m_iconsWidget);

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

    Q_ASSERT(m_ui->categoryList->model()->rowCount() == m_ui->stackedWidget->count());

    connect(m_ui->categoryList, SIGNAL(currentRowChanged(int)), m_ui->stackedWidget, SLOT(setCurrentIndex(int)));

    connect(m_mainUi->togglePasswordButton, SIGNAL(toggled(bool)), SLOT(togglePassword(bool)));
    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));
    connect(m_mainUi->passwordEdit, SIGNAL(textEdited(QString)), SLOT(setPasswordCheckColors()));
    connect(m_mainUi->passwordRepeatEdit, SIGNAL(textEdited(QString)), SLOT(setPasswordCheckColors()));

    m_defaultIconModel = new DefaultIconModel(m_iconsWidget);
    m_iconsUi->defaultIconsView->setModel(m_defaultIconModel);

    m_customIconModel = new CustomIconModel(m_iconsWidget);
    m_iconsUi->customIconsView->setModel(m_customIconModel);

    connect(m_iconsUi->defaultIconsView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(updateRadioButtonDefaultIcons()));
    connect(m_iconsUi->customIconsView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(updateRadioButtonCustomIcons()));
    connect(m_iconsUi->defaultIconsRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateWidgetsDefaultIcons(bool)));
    connect(m_iconsUi->customIconsRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateWidgetsCustomIcons(bool)));
    connect(m_iconsUi->addButton, SIGNAL(clicked()), SLOT(addCustomIcon()));
    connect(m_iconsUi->deleteButton, SIGNAL(clicked()), SLOT(removeCustomIcon()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(saveEntry()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(cancel()));
}

EditEntryWidget::~EditEntryWidget()
{
}

const QColor EditEntryWidget::normalColor = Qt::white;
const QColor EditEntryWidget::correctSoFarColor = QColor(255, 205, 15);
const QColor EditEntryWidget::errorColor = QColor(255, 125, 125);

void EditEntryWidget::loadEntry(Entry* entry, bool create, const QString& groupName,
                                Database* database)
{
    m_entry = entry;
    m_database = database;
    m_create = create;

    if (create) {
        m_ui->headerLabel->setText(groupName+" > "+tr("Add entry"));
    }
    else {
        m_ui->headerLabel->setText(groupName+" > "+tr("Edit entry"));
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

    m_ui->categoryList->setCurrentRow(0);

    if (m_attributesModel->rowCount() != 0) {
        m_advancedUi->attributesView->setCurrentIndex(m_attributesModel->index(0, 0));
    }
    else {
        m_advancedUi->attributesEdit->setPlainText("");
        m_advancedUi->attributesEdit->setEnabled(false);
    }

    if (database) {
        m_iconsWidget->setEnabled(true);

        m_customIconModel->setIcons(database->metadata()->customIcons(),
                                    database->metadata()->customIconsOrder());

        Uuid iconUuid = entry->iconUuid();
        if (iconUuid.isNull()) {
            int iconNumber = entry->iconNumber();
            m_iconsUi->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(iconNumber, 0));
            m_iconsUi->defaultIconsRadio->setChecked(true);
        }
        else {
            QModelIndex index = m_customIconModel->indexFromUuid(iconUuid);
            if (index.isValid()) {
                m_iconsUi->customIconsView->setCurrentIndex(index);
                m_iconsUi->customIconsRadio->setChecked(true);
            }
            else {
                m_iconsUi->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(0, 0));
                m_iconsUi->defaultIconsRadio->setChecked(true);
            }
        }
    }
    else {
        m_iconsWidget->setEnabled(false);
    }

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

    if (m_iconsUi->defaultIconsRadio->isChecked()) {
        QModelIndex index = m_iconsUi->defaultIconsView->currentIndex();
        if (index.isValid()) {
            m_entry->setIcon(index.row());
        }
        else {
            m_entry->setIcon(0);
        }
    }
    else {
        QModelIndex index = m_iconsUi->customIconsView->currentIndex();
        if (index.isValid()) {
            m_entry->setIcon(m_customIconModel->uuidFromIndex(m_iconsUi->customIconsView->currentIndex()));
        }
        else {
            m_entry->setIcon(0);
        }
    }

    if (!m_create) {
        m_entry->endUpdate();
    }

    m_entry = 0;
    m_entryAttributes->clear();
    m_entryAttachments->clear();

    Q_EMIT editFinished(true);
}

void EditEntryWidget::cancel()
{
    m_entry = 0;
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
    QPalette pal;
    if (passwordsEqual()) {
        pal.setColor(QPalette::Base, normalColor);
    }
    else {
        if (m_mainUi->passwordEdit->text().startsWith(m_mainUi->passwordRepeatEdit->text())) {
            pal.setColor(QPalette::Base, correctSoFarColor);
        }
        else {
            pal.setColor(QPalette::Base, errorColor);
        }
    }
    m_mainUi->passwordRepeatEdit->setPalette(pal);
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

void EditEntryWidget::addCustomIcon()
{
    if (m_database) {
        QString filter = QString("%1 (%2);;%3 (*.*)").arg(tr("Images"),
                    Tools::imageReaderFilter(), tr("All files"));

        QString filename = QFileDialog::getOpenFileName(
                    this, tr("Select Image"), "", filter);
        if (!filename.isEmpty()) {
            QImage image(filename);
            if (!image.isNull()) {
                Uuid uuid = Uuid::random();
                m_database->metadata()->addCustomIcon(uuid, image.scaled(16, 16));
                m_customIconModel->setIcons(m_database->metadata()->customIcons(),
                                            m_database->metadata()->customIconsOrder());
                QModelIndex index = m_customIconModel->indexFromUuid(uuid);
                m_iconsUi->customIconsView->setCurrentIndex(index);
            }
            else {
                // TODO: show error
            }
        }
    }
}

void EditEntryWidget::removeCustomIcon()
{
    if (m_database) {
        QModelIndex index = m_iconsUi->customIconsView->currentIndex();
        if (index.isValid()) {
            Uuid iconUuid = m_customIconModel->uuidFromIndex(index);
            int iconUsedCount = 0;

            QList<Entry*> allEntries = m_database->rootGroup()->entriesRecursive(true);
            QListIterator<Entry*> iEntries(allEntries);
            while (iEntries.hasNext()) {
                Entry* entry = iEntries.next();
                if (iconUuid == entry->iconUuid() && entry != m_entry) {
                    iconUsedCount++;
                }
            }

            QList<const Group*> allGroups = m_database->rootGroup()->groupsRecursive(true);
            QListIterator<const Group*> iGroups(allGroups);
            while (iGroups.hasNext()) {
                const Group* group = iGroups.next();
                if (iconUuid == group->iconUuid()) {
                    iconUsedCount++;
                }
            }

            if (iconUsedCount == 0) {
                m_database->metadata()->removeCustomIcon(iconUuid);
                m_customIconModel->setIcons(m_database->metadata()->customIcons(),
                                            m_database->metadata()->customIconsOrder());
                if (m_customIconModel->rowCount() > 0) {
                    m_iconsUi->customIconsView->setCurrentIndex(m_customIconModel->index(0, 0));
                }
                else {
                    updateRadioButtonDefaultIcons();
                }
            }
            else {
                QMessageBox::information(this, tr("Can't delete icon!"),
                                         tr("Can't delete icon. Still used by %1 items.")
                                         .arg(iconUsedCount));
            }
        }
    }
}

void EditEntryWidget::updateWidgetsDefaultIcons(bool check)
{
    if (check) {
        QModelIndex index = m_iconsUi->defaultIconsView->currentIndex();
        if (!index.isValid()) {
            m_iconsUi->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(0, 0));
        }
        else {
            m_iconsUi->defaultIconsView->setCurrentIndex(index);
        }
        m_iconsUi->customIconsView->selectionModel()->clearSelection();
        m_iconsUi->addButton->setEnabled(false);
        m_iconsUi->deleteButton->setEnabled(false);
    }
}

void EditEntryWidget::updateWidgetsCustomIcons(bool check)
{
    if (check) {
        QModelIndex index = m_iconsUi->customIconsView->currentIndex();
        if (!index.isValid()) {
            m_iconsUi->customIconsView->setCurrentIndex(m_customIconModel->index(0, 0));
        }
        else {
            m_iconsUi->customIconsView->setCurrentIndex(index);
        }
        m_iconsUi->defaultIconsView->selectionModel()->clearSelection();
        m_iconsUi->addButton->setEnabled(true);
        m_iconsUi->deleteButton->setEnabled(true);
    }
}

void EditEntryWidget::updateRadioButtonDefaultIcons()
{
    m_iconsUi->defaultIconsRadio->setChecked(true);
}

void EditEntryWidget::updateRadioButtonCustomIcons()
{
    m_iconsUi->customIconsRadio->setChecked(true);
}
