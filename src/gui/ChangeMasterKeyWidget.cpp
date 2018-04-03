/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "ChangeMasterKeyWidget.h"
#include "ui_ChangeMasterKeyWidget.h"

#include "MainWindow.h"
#include "core/FilePath.h"
#include "crypto/Random.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "keys/YkChallengeResponseKey.h"

#include "config-keepassx.h"

#include <QSharedPointer>
#include <QtConcurrentRun>

ChangeMasterKeyWidget::ChangeMasterKeyWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::ChangeMasterKeyWidget())
{
    m_ui->setupUi(this);

    m_ui->messageWidget->setHidden(true);

    m_ui->togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    m_ui->repeatPasswordEdit->enableVerifyMode(m_ui->enterPasswordEdit);

    connect(m_ui->passwordGroup, SIGNAL(clicked(bool)), SLOT(setOkEnabled()));
    connect(m_ui->togglePasswordButton, SIGNAL(toggled(bool)), m_ui->enterPasswordEdit, SLOT(setShowPassword(bool)));

    connect(m_ui->keyFileGroup, SIGNAL(clicked(bool)), SLOT(setOkEnabled()));
    connect(m_ui->createKeyFileButton, SIGNAL(clicked()), SLOT(createKeyFile()));
    connect(m_ui->browseKeyFileButton, SIGNAL(clicked()), SLOT(browseKeyFile()));
    connect(m_ui->keyFileCombo, SIGNAL(editTextChanged(QString)), SLOT(setOkEnabled()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(generateKey()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

#ifdef WITH_XC_YUBIKEY
    m_ui->yubikeyProgress->setVisible(false);
    QSizePolicy sp = m_ui->yubikeyProgress->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_ui->yubikeyProgress->setSizePolicy(sp);

    connect(m_ui->challengeResponseGroup, SIGNAL(clicked(bool)), SLOT(challengeResponseGroupToggled(bool)));
    connect(m_ui->challengeResponseGroup, SIGNAL(clicked(bool)), SLOT(setOkEnabled()));
    connect(m_ui->buttonRedetectYubikey, SIGNAL(clicked()), SLOT(pollYubikey()));

    connect(YubiKey::instance(), SIGNAL(detected(int, bool)), SLOT(yubikeyDetected(int, bool)), Qt::QueuedConnection);
    connect(YubiKey::instance(), SIGNAL(notFound()), SLOT(noYubikeyFound()), Qt::QueuedConnection);
#else
    m_ui->challengeResponseGroup->setVisible(false);
#endif
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
            m_ui->messageWidget->showMessage(tr("Unable to create key file: %1").arg(errorMsg), MessageWidget::Error);
        } else {
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

#ifdef WITH_XC_YUBIKEY
    m_ui->challengeResponseGroup->setChecked(false);
    m_ui->comboChallengeResponse->clear();
#endif

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
                if (MessageBox::warning(this,
                                        tr("Empty password"),
                                        tr("Do you really want to use an empty string as password?"),
                                        QMessageBox::Yes | QMessageBox::No)
                    != QMessageBox::Yes) {
                    return;
                }
            }
            m_key.addKey(PasswordKey(m_ui->enterPasswordEdit->text()));
        } else {
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
            m_ui->messageWidget->showMessage(tr("Failed to set %1 as the key file:\n%2").arg(fileKeyName, errorMsg),
                                             MessageWidget::Error);
            return;
        }
        if (fileKey.type() != FileKey::Hashed) {
            QMessageBox::warning(this,
                                 tr("Legacy key file format"),
                                 tr("You are using a legacy key file format which may become\n"
                                    "unsupported in the future.\n\n"
                                    "Please consider generating a new key file."),
                                 QMessageBox::Ok);
        }
        m_key.addKey(fileKey);
    }

#ifdef WITH_XC_YUBIKEY
    if (m_ui->challengeResponseGroup->isChecked()) {
        int selectionIndex = m_ui->comboChallengeResponse->currentIndex();
        int comboPayload = m_ui->comboChallengeResponse->itemData(selectionIndex).toInt();

        if (0 == comboPayload) {
            m_ui->messageWidget->showMessage(tr("Changing master key failed: no YubiKey inserted."),
                                             MessageWidget::Error);
            return;
        }

        // read blocking mode from LSB and slot index number from second LSB
        bool blocking = comboPayload & 1;
        int slot = comboPayload >> 1;
        auto key = QSharedPointer<YkChallengeResponseKey>(new YkChallengeResponseKey(slot, blocking));
        m_key.addChallengeResponseKey(key);
    }
#endif

    m_ui->messageWidget->hideMessage();
    emit editFinished(true);
}

void ChangeMasterKeyWidget::reject()
{
    emit editFinished(false);
}

void ChangeMasterKeyWidget::challengeResponseGroupToggled(bool checked)
{
    if (checked)
        pollYubikey();
}

void ChangeMasterKeyWidget::pollYubikey()
{
    m_ui->buttonRedetectYubikey->setEnabled(false);
    m_ui->comboChallengeResponse->setEnabled(false);
    m_ui->comboChallengeResponse->clear();
    m_ui->yubikeyProgress->setVisible(true);
    setOkEnabled();

    // YubiKey init is slow, detect asynchronously to not block the UI
    QtConcurrent::run(YubiKey::instance(), &YubiKey::detect);
}

void ChangeMasterKeyWidget::yubikeyDetected(int slot, bool blocking)
{
    YkChallengeResponseKey yk(slot, blocking);
    // add detected YubiKey to combo box and encode blocking mode in LSB, slot number in second LSB
    m_ui->comboChallengeResponse->addItem(yk.getName(), QVariant((slot << 1) | blocking));
    m_ui->comboChallengeResponse->setEnabled(m_ui->challengeResponseGroup->isChecked());
    m_ui->buttonRedetectYubikey->setEnabled(m_ui->challengeResponseGroup->isChecked());
    m_ui->yubikeyProgress->setVisible(false);
    setOkEnabled();
}

void ChangeMasterKeyWidget::noYubikeyFound()
{
    m_ui->buttonRedetectYubikey->setEnabled(m_ui->challengeResponseGroup->isChecked());
    m_ui->yubikeyProgress->setVisible(false);
    setOkEnabled();
}

void ChangeMasterKeyWidget::setOkEnabled()
{
    bool ok = m_ui->passwordGroup->isChecked()
              || (m_ui->challengeResponseGroup->isChecked() && !m_ui->comboChallengeResponse->currentText().isEmpty())
              || (m_ui->keyFileGroup->isChecked() && !m_ui->keyFileCombo->currentText().isEmpty());

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void ChangeMasterKeyWidget::setCancelEnabled(bool enabled)
{
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(enabled);
}
