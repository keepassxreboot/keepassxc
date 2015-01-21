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

#include "ChangeMasterKeyWidget.h"
#include "ui_ChangeMasterKeyWidget.h"

#include "core/FilePath.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"

ChangeMasterKeyWidget::ChangeMasterKeyWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::ChangeMasterKeyWidget())
{
    m_ui->setupUi(this);

    m_ui->messageWidget->setHidden(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(generateKey()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), m_ui->enterPasswordEdit, SLOT(setShowPassword(bool)));
    m_ui->repeatPasswordEdit->enableVerifyMode(m_ui->enterPasswordEdit);
    connect(m_ui->createKeyFileButton, SIGNAL(clicked()), SLOT(createKeyFile()));
    connect(m_ui->browseKeyFileButton, SIGNAL(clicked()), SLOT(browseKeyFile()));
}

ChangeMasterKeyWidget::~ChangeMasterKeyWidget()
{
}

void ChangeMasterKeyWidget::createKeyFile()
{
    QString filters = QString("%1 (*.key);;%2 (*)").arg(tr("Key files"), tr("All files"));
    QString fileName = fileDialog()->getSaveFileName(this, tr("Create Key File..."), QString(), filters);

    if (!fileName.isEmpty()) {
        QString errorMsg;
        bool created = FileKey::create(fileName, &errorMsg);
        if (!created) {
            m_ui->messageWidget->showMessage(tr("Unable to create Key File : ").append(errorMsg), MessageWidget::Error);
        }
        else {
            m_ui->keyFileCombo->setEditText(fileName);
        }
    }
}

void ChangeMasterKeyWidget::browseKeyFile()
{
    QString filters = QString("%1 (*.key);;%2 (*)").arg(tr("Key files"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Select a key file"), QString(), filters);

    if (!fileName.isEmpty()) {
        m_ui->keyFileCombo->setEditText(fileName);
    }
}

void ChangeMasterKeyWidget::clearForms()
{
    m_key.clear();

    m_ui->passwordGroup->setChecked(true);
    m_ui->enterPasswordEdit->setText("");
    m_ui->repeatPasswordEdit->setText("");
    m_ui->keyFileGroup->setChecked(false);
    m_ui->togglePasswordButton->setChecked(false);
    // TODO: clear m_ui->keyFileCombo

    m_ui->enterPasswordEdit->setFocus();
}

CompositeKey ChangeMasterKeyWidget::newMasterKey()
{
    return m_key;
}

QLabel* ChangeMasterKeyWidget::headlineLabel()
{
    return m_ui->headlineLabel;
}

void ChangeMasterKeyWidget::generateKey()
{
    m_key.clear();

    if (m_ui->passwordGroup->isChecked()) {
        if (m_ui->enterPasswordEdit->text() == m_ui->repeatPasswordEdit->text()) {
            if (m_ui->enterPasswordEdit->text().isEmpty()) {
                if (MessageBox::question(this, tr("Question"),
                                         tr("Do you really want to use an empty string as password?"),
                                         QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
                    return;
                }
            }
            m_key.addKey(PasswordKey(m_ui->enterPasswordEdit->text()));
        }
        else {
            m_ui->messageWidget->showMessage(tr("Different passwords supplied."), MessageWidget::Error);
            m_ui->enterPasswordEdit->setText("");
            m_ui->repeatPasswordEdit->setText("");
            return;
        }
    }
    if (m_ui->keyFileGroup->isChecked()) {
        FileKey fileKey;
        QString errorMsg;
        QString fileKeyName = m_ui->keyFileCombo->currentText();
        if (!fileKey.load(fileKeyName, &errorMsg)) {
            m_ui->messageWidget->showMessage(
               tr("Failed to set %1 as the Key file:\n%2").arg(fileKeyName, errorMsg), MessageWidget::Error);
            return;
        }
        m_key.addKey(fileKey);
    }

    m_ui->messageWidget->hideMessage();
    Q_EMIT editFinished(true);
}


void ChangeMasterKeyWidget::reject()
{
    Q_EMIT editFinished(false);
}
