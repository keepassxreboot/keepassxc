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

#include <QtGui/QListWidget>
#include <QtGui/QStackedLayout>

#include "core/Entry.h"
#include "core/Group.h"
#include "gui/EntryAttachmentsModel.h"
#include "gui/EntryAttributesModel.h"

EditEntryWidget::EditEntryWidget(QWidget* parent)
    : QWidget(parent)
    , m_entry(0)
    , m_ui(new Ui::EditEntryWidget())
    , m_mainUi(new Ui::EditEntryWidgetMain())
    , m_notesUi(new Ui::EditEntryWidgetNotes())
    , m_advancedUi(new Ui::EditEntryWidgetAdvanced())
    , m_mainWidget(new QWidget(this))
    , m_notesWidget(new QWidget(this))
    , m_advancedWidget(new QWidget(this))
{
    m_ui->setupUi(this);

    QFont headerLabelFont = m_ui->headerLabel->font();
    headerLabelFont.setBold(true);
    headerLabelFont.setPointSize(headerLabelFont.pointSize() + 2);
    m_ui->headerLabel->setFont(headerLabelFont);

    m_ui->categoryList->addItem(tr("Entry"));
    m_ui->categoryList->addItem(tr("Description"));
    m_ui->categoryList->addItem(tr("Advanced"));

    m_mainUi->setupUi(m_mainWidget);
    m_ui->stackedWidget->addWidget(m_mainWidget);

    m_notesUi->setupUi(m_notesWidget);
    m_ui->stackedWidget->addWidget(m_notesWidget);

    m_advancedUi->setupUi(m_advancedWidget);
    m_ui->stackedWidget->addWidget(m_advancedWidget);

    m_attachmentsModel = new EntryAttachmentsModel(m_advancedWidget);
    m_advancedUi->attachmentsView->setModel(m_attachmentsModel);
    m_entryAttributes = new EntryAttributes(this);

    m_attributesModel = new EntryAttributesModel(m_advancedWidget);
    m_advancedUi->attributesView->setModel(m_attributesModel);
    m_entryAttachments = new EntryAttachments(this);

    Q_ASSERT(m_ui->categoryList->model()->rowCount() == m_ui->stackedWidget->count());

    connect(m_ui->categoryList, SIGNAL(currentRowChanged(int)), m_ui->stackedWidget, SLOT(setCurrentIndex(int)));

    connect(m_mainUi->togglePasswordButton, SIGNAL(toggled(bool)), SLOT(togglePassword(bool)));
    connect(m_mainUi->expireCheck, SIGNAL(toggled(bool)), m_mainUi->expireDatePicker, SLOT(setEnabled(bool)));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(saveEntry()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(cancel()));
}

EditEntryWidget::~EditEntryWidget()
{
}

void EditEntryWidget::loadEntry(Entry* entry, bool create, const QString& groupName)
{
    m_entry = entry;

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
    m_mainUi->expireCheck->setChecked(entry->timeInfo().expires());
    m_mainUi->expireDatePicker->setDateTime(entry->timeInfo().expiryTime());
    m_mainUi->togglePasswordButton->setChecked(true);

    m_notesUi->notesEdit->setPlainText(entry->notes());

    m_entryAttributes->copyCustomKeysFrom(entry->attributes());
    m_attributesModel->setEntryAttributes(m_entryAttributes);
    m_entryAttachments->copyFrom(entry->attachments());
    m_attachmentsModel->setEntryAttachments(m_entryAttachments);

    m_ui->categoryList->setCurrentRow(0);
}

void EditEntryWidget::saveEntry()
{
    m_entry->setTitle(m_mainUi->titleEdit->text());
    m_entry->setUsername(m_mainUi->usernameEdit->text());
    m_entry->setUrl(m_mainUi->urlEdit->text());
    // TODO check password repeat field
    m_entry->setPassword(m_mainUi->passwordEdit->text());
    m_entry->setExpires(m_mainUi->expireCheck->isChecked());
    QDateTime dateTime(m_mainUi->expireDatePicker->dateTime());
    dateTime.setTimeSpec(Qt::UTC);
    m_entry->setExpiryTime(dateTime);

    m_entry->setNotes(m_notesUi->notesEdit->toPlainText());

    m_entry->attributes()->copyCustomKeysFrom(m_entryAttributes);
    m_entryAttributes->clear();
    m_entry->attachments()->copyFrom(m_entryAttachments);
    m_entryAttachments->clear();

    m_entry = 0;
    Q_EMIT editFinished(true);
}

void EditEntryWidget::cancel()
{
    m_entry = 0;
    Q_EMIT editFinished(false);
}

void EditEntryWidget::togglePassword(bool checked)
{
    m_mainUi->passwordEdit->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
    m_mainUi->passwordRepeatEdit->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}
