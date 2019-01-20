/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "DatabaseOpenWidget.h"
#include "ui_DatabaseOpenWidget.h"

#include "core/Config.h"
#include "core/Database.h"
#include "core/FilePath.h"
#include "crypto/Random.h"
#include "format/KeePass2Reader.h"
#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "keys/YkChallengeResponseKey.h"
#include "touchid/TouchID.h"

#include "config-keepassx.h"

#include <QSharedPointer>
#include <QtConcurrentRun>

DatabaseOpenWidget::DatabaseOpenWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseOpenWidget())
    , m_db(nullptr)
{
    m_ui->setupUi(this);

    m_ui->messageWidget->setHidden(true);
    m_ui->checkPassword->setChecked(true);

    QFont font = m_ui->labelHeadline->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 2);
    m_ui->labelHeadline->setFont(font);

    m_ui->buttonTogglePassword->setIcon(filePath()->onOffIcon("actions", "password-show"));
    connect(m_ui->buttonTogglePassword, SIGNAL(toggled(bool)), m_ui->editPassword, SLOT(setShowPassword(bool)));
    connect(m_ui->buttonBrowseFile, SIGNAL(clicked()), SLOT(browseKeyFile()));

    connect(m_ui->editPassword, SIGNAL(textChanged(QString)), SLOT(activatePassword()));
    connect(m_ui->comboKeyFile, SIGNAL(editTextChanged(QString)), SLOT(activateKeyFile()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(openDatabase()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

#ifdef WITH_XC_YUBIKEY
    m_ui->yubikeyProgress->setVisible(false);
    QSizePolicy sp = m_ui->yubikeyProgress->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_ui->yubikeyProgress->setSizePolicy(sp);

    connect(m_ui->buttonRedetectYubikey, SIGNAL(clicked()), SLOT(pollYubikey()));
    connect(m_ui->comboChallengeResponse, SIGNAL(activated(int)), SLOT(activateChallengeResponse()));
#else
    m_ui->checkChallengeResponse->setVisible(false);
    m_ui->buttonRedetectYubikey->setVisible(false);
    m_ui->comboChallengeResponse->setVisible(false);
    m_ui->yubikeyProgress->setVisible(false);
#endif

#ifdef Q_OS_MACOS
    // add random padding to layouts to align widgets properly
    m_ui->dialogButtonsLayout->setContentsMargins(10, 0, 15, 0);
    m_ui->gridLayout->setContentsMargins(10, 0, 0, 0);
    m_ui->labelLayout->setContentsMargins(10, 0, 10, 0);
#endif

#ifndef WITH_XC_TOUCHID
    m_ui->checkTouchID->setVisible(false);
#else
    if (!TouchID::getInstance().isAvailable()) {
        m_ui->checkTouchID->setVisible(false);
    }
#endif
}

DatabaseOpenWidget::~DatabaseOpenWidget()
{
}

void DatabaseOpenWidget::showEvent(QShowEvent* event)
{
    DialogyWidget::showEvent(event);
    m_ui->editPassword->setFocus();

#ifdef WITH_XC_YUBIKEY
    // showEvent() may be called twice, so make sure we are only polling once
    if (!m_yubiKeyBeingPolled) {
        // clang-format off
        connect(YubiKey::instance(), SIGNAL(detected(int,bool)), SLOT(yubikeyDetected(int,bool)), Qt::QueuedConnection);
        connect(YubiKey::instance(), SIGNAL(detectComplete()), SLOT(yubikeyDetectComplete()), Qt::QueuedConnection);
        connect(YubiKey::instance(), SIGNAL(notFound()), SLOT(noYubikeyFound()), Qt::QueuedConnection);
        // clang-format on

        pollYubikey();
        m_yubiKeyBeingPolled = true;
    }
#endif
}

void DatabaseOpenWidget::hideEvent(QHideEvent* event)
{
    DialogyWidget::hideEvent(event);

#ifdef WITH_XC_YUBIKEY
    // Don't listen to any Yubikey events if we are hidden
    disconnect(YubiKey::instance(), nullptr, this, nullptr);
    m_yubiKeyBeingPolled = false;
#endif

    if (isVisible()) {
        return;
    }

    clearForms();
}

void DatabaseOpenWidget::load(const QString& filename)
{
    m_filename = filename;

    m_ui->labelFilename->setText(filename);

    if (config()->get("RememberLastKeyFiles").toBool()) {
        QHash<QString, QVariant> lastKeyFiles = config()->get("LastKeyFiles").toHash();
        if (lastKeyFiles.contains(m_filename)) {
            m_ui->checkKeyFile->setChecked(true);
            m_ui->comboKeyFile->addItem(lastKeyFiles[m_filename].toString());
        }
    }

    QHash<QString, QVariant> useTouchID = config()->get("UseTouchID").toHash();
    m_ui->checkTouchID->setChecked(useTouchID.value(m_filename, false).toBool());

    m_ui->editPassword->setFocus();
}

void DatabaseOpenWidget::clearForms()
{
    m_ui->editPassword->setText("");
    m_ui->comboKeyFile->clear();
    m_ui->comboKeyFile->setEditText("");
    m_ui->checkPassword->setChecked(true);
    m_ui->checkKeyFile->setChecked(false);
    m_ui->checkChallengeResponse->setChecked(false);
    m_ui->checkTouchID->setChecked(false);
    m_ui->buttonTogglePassword->setChecked(false);
    m_db.reset();
}

QSharedPointer<Database> DatabaseOpenWidget::database()
{
    return m_db;
}

void DatabaseOpenWidget::enterKey(const QString& pw, const QString& keyFile)
{
    if (!pw.isEmpty()) {
        m_ui->editPassword->setText(pw);
    }
    if (!keyFile.isEmpty()) {
        m_ui->comboKeyFile->setEditText(keyFile);
    }

    openDatabase();
}

void DatabaseOpenWidget::openDatabase()
{
    QSharedPointer<CompositeKey> masterKey = databaseKey();
    if (!masterKey) {
        return;
    }

    if (!m_ui->editPassword->isPasswordVisible()) {
        m_ui->editPassword->setShowPassword(false);
    }
    QCoreApplication::processEvents();

    m_db.reset(new Database());
    QString error;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool ok = m_db->open(m_filename, masterKey, &error, false);
    QApplication::restoreOverrideCursor();
    if (!ok) {
        m_ui->messageWidget->showMessage(tr("Unable to open the database:\n%1").arg(error),
                                         MessageWidget::MessageType::Error);
        return;
    }

    if (m_db) {
#ifdef WITH_XC_TOUCHID
        QHash<QString, QVariant> useTouchID = config()->get("UseTouchID").toHash();

        // check if TouchID can & should be used to unlock the database next time
        if (m_ui->checkTouchID->isChecked() && TouchID::getInstance().isAvailable()) {
            // encrypt and store key blob
            if (TouchID::getInstance().storeKey(m_filename, PasswordKey(m_ui->editPassword->text()).rawKey())) {
                useTouchID.insert(m_filename, true);
            }
        } else {
            // when TouchID not available or unchecked, reset for the current database
            TouchID::getInstance().reset(m_filename);
            useTouchID.insert(m_filename, false);
        }

        config()->set("UseTouchID", useTouchID);
#endif

        if (m_ui->messageWidget->isVisible()) {
            m_ui->messageWidget->animatedHide();
        }
        emit dialogFinished(true);
    } else {
        m_ui->messageWidget->showMessage(tr("Unable to open the database:\n%1").arg(error), MessageWidget::Error);
        m_ui->editPassword->setText("");

#ifdef WITH_XC_TOUCHID
        // unable to unlock database, reset TouchID for the current database
        TouchID::getInstance().reset(m_filename);
#endif
    }
}

QSharedPointer<CompositeKey> DatabaseOpenWidget::databaseKey()
{
    auto masterKey = QSharedPointer<CompositeKey>::create();

    if (m_ui->checkPassword->isChecked()) {
        masterKey->addKey(QSharedPointer<PasswordKey>::create(m_ui->editPassword->text()));
    }

#ifdef WITH_XC_TOUCHID
    // check if TouchID is available and enabled for unlocking the database
    if (m_ui->checkTouchID->isChecked() && TouchID::getInstance().isAvailable() && masterKey->isEmpty()) {
        // try to get, decrypt and use PasswordKey
        QSharedPointer<QByteArray> passwordKey = TouchID::getInstance().getKey(m_filename);
        if (passwordKey != NULL) {
            // check if the user cancelled the operation
            if (passwordKey.isNull())
                return QSharedPointer<CompositeKey>();

            masterKey->addKey(PasswordKey::fromRawKey(*passwordKey));
        }
    }
#endif

    QHash<QString, QVariant> lastKeyFiles = config()->get("LastKeyFiles").toHash();
    QHash<QString, QVariant> lastChallengeResponse = config()->get("LastChallengeResponse").toHash();

    if (m_ui->checkKeyFile->isChecked()) {
        auto key = QSharedPointer<FileKey>::create();
        QString keyFilename = m_ui->comboKeyFile->currentText();
        QString errorMsg;
        if (!key->load(keyFilename, &errorMsg)) {
            m_ui->messageWidget->showMessage(tr("Can't open key file:\n%1").arg(errorMsg), MessageWidget::Error);
            return {};
        }
        if (key->type() != FileKey::Hashed && !config()->get("Messages/NoLegacyKeyFileWarning").toBool()) {
            QMessageBox legacyWarning;
            legacyWarning.setWindowTitle(tr("Legacy key file format"));
            legacyWarning.setText(tr("You are using a legacy key file format which may become\n"
                                     "unsupported in the future.\n\n"
                                     "Please consider generating a new key file."));
            legacyWarning.setIcon(QMessageBox::Icon::Warning);
            legacyWarning.addButton(QMessageBox::Ok);
            legacyWarning.setDefaultButton(QMessageBox::Ok);
            legacyWarning.setCheckBox(new QCheckBox(tr("Don't show this warning again")));

            connect(legacyWarning.checkBox(), &QCheckBox::stateChanged, [](int state) {
                config()->set("Messages/NoLegacyKeyFileWarning", state == Qt::CheckState::Checked);
            });

            legacyWarning.exec();
        }
        masterKey->addKey(key);
        lastKeyFiles[m_filename] = keyFilename;
    } else {
        lastKeyFiles.remove(m_filename);
    }

    if (m_ui->checkChallengeResponse->isChecked()) {
        lastChallengeResponse[m_filename] = true;
    } else {
        lastChallengeResponse.remove(m_filename);
    }

    if (config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastKeyFiles", lastKeyFiles);
    }

#ifdef WITH_XC_YUBIKEY
    if (config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastChallengeResponse", lastChallengeResponse);
    }

    if (m_ui->checkChallengeResponse->isChecked()) {
        int selectionIndex = m_ui->comboChallengeResponse->currentIndex();
        int comboPayload = m_ui->comboChallengeResponse->itemData(selectionIndex).toInt();

        // read blocking mode from LSB and slot index number from second LSB
        bool blocking = comboPayload & 1;
        int slot = comboPayload >> 1;
        auto key = QSharedPointer<YkChallengeResponseKey>(new YkChallengeResponseKey(slot, blocking));
        masterKey->addChallengeResponseKey(key);
    }
#endif

    return masterKey;
}

void DatabaseOpenWidget::reject()
{
    emit dialogFinished(false);
}

void DatabaseOpenWidget::activatePassword()
{
    m_ui->checkPassword->setChecked(true);
}

void DatabaseOpenWidget::activateKeyFile()
{
    m_ui->checkKeyFile->setChecked(true);
}

void DatabaseOpenWidget::activateChallengeResponse()
{
    m_ui->checkChallengeResponse->setChecked(true);
}

void DatabaseOpenWidget::browseKeyFile()
{
    QString filters = QString("%1 (*);;%2 (*.key)").arg(tr("All files"), tr("Key files"));
    if (!config()->get("RememberLastKeyFiles").toBool()) {
        fileDialog()->setNextForgetDialog();
    }
    QString filename = fileDialog()->getOpenFileName(this, tr("Select key file"), QString(), filters);

    if (!filename.isEmpty()) {
        m_ui->comboKeyFile->lineEdit()->setText(filename);
    }
}

void DatabaseOpenWidget::pollYubikey()
{
    m_ui->buttonRedetectYubikey->setEnabled(false);
    m_ui->checkChallengeResponse->setEnabled(false);
    m_ui->checkChallengeResponse->setChecked(false);
    m_ui->comboChallengeResponse->setEnabled(false);
    m_ui->comboChallengeResponse->clear();
    m_ui->yubikeyProgress->setVisible(true);

    // YubiKey init is slow, detect asynchronously to not block the UI
    QtConcurrent::run(YubiKey::instance(), &YubiKey::detect);
}

void DatabaseOpenWidget::yubikeyDetected(int slot, bool blocking)
{
    YkChallengeResponseKey yk(slot, blocking);
    // add detected YubiKey to combo box and encode blocking mode in LSB, slot number in second LSB
    m_ui->comboChallengeResponse->addItem(yk.getName(), QVariant((slot << 1) | blocking));

    if (config()->get("RememberLastKeyFiles").toBool()) {
        QHash<QString, QVariant> lastChallengeResponse = config()->get("LastChallengeResponse").toHash();
        if (lastChallengeResponse.contains(m_filename)) {
            m_ui->checkChallengeResponse->setChecked(true);
        }
    }
}

void DatabaseOpenWidget::yubikeyDetectComplete()
{
    m_ui->comboChallengeResponse->setEnabled(true);
    m_ui->checkChallengeResponse->setEnabled(true);
    m_ui->buttonRedetectYubikey->setEnabled(true);
    m_ui->yubikeyProgress->setVisible(false);
    m_yubiKeyBeingPolled = false;
}

void DatabaseOpenWidget::noYubikeyFound()
{
    m_ui->buttonRedetectYubikey->setEnabled(true);
    m_ui->yubikeyProgress->setVisible(false);
    m_yubiKeyBeingPolled = false;
}
