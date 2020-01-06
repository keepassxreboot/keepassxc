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

#include <QDesktopServices>
#include <QFont>
#include <QSharedPointer>
#include <QtConcurrentRun>

DatabaseOpenWidget::DatabaseOpenWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseOpenWidget())
    , m_db(nullptr)
{
    m_ui->setupUi(this);

    m_ui->messageWidget->setHidden(true);

    QFont font;
    font.setPointSize(font.pointSize() + 4);
    font.setBold(true);
    m_ui->labelHeadline->setFont(font);
    m_ui->labelHeadline->setText(tr("Unlock KeePassXC Database"));

    m_ui->comboKeyFile->lineEdit()->addAction(m_ui->keyFileClearIcon, QLineEdit::TrailingPosition);

    m_ui->buttonTogglePassword->setIcon(filePath()->onOffIcon("actions", "password-show"));
    connect(m_ui->buttonTogglePassword, SIGNAL(toggled(bool)), m_ui->editPassword, SLOT(setShowPassword(bool)));
    connect(m_ui->buttonTogglePassword, SIGNAL(toggled(bool)), m_ui->editPassword, SLOT(setFocus()));
    connect(m_ui->buttonBrowseFile, SIGNAL(clicked()), SLOT(browseKeyFile()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(openDatabase()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    m_ui->hardwareKeyLabelHelp->setIcon(filePath()->icon("actions", "system-help").pixmap(QSize(12, 12)));
    connect(m_ui->hardwareKeyLabelHelp, SIGNAL(clicked(bool)), SLOT(openHardwareKeyHelp()));
    m_ui->keyFileLabelHelp->setIcon(filePath()->icon("actions", "system-help").pixmap(QSize(12, 12)));
    connect(m_ui->keyFileLabelHelp, SIGNAL(clicked(bool)), SLOT(openKeyFileHelp()));

    connect(m_ui->comboKeyFile->lineEdit(), SIGNAL(textChanged(QString)), SLOT(handleKeyFileComboEdited()));
    connect(m_ui->comboKeyFile, SIGNAL(currentIndexChanged(int)), SLOT(handleKeyFileComboChanged()));
    m_ui->keyFileClearIcon->setIcon(filePath()->icon("actions", "edit-clear-locationbar-rtl"));
    m_ui->keyFileClearIcon->setVisible(false);
    connect(m_ui->keyFileClearIcon, SIGNAL(triggered(bool)), SLOT(clearKeyFileEdit()));

#ifdef WITH_XC_YUBIKEY
    m_ui->yubikeyProgress->setVisible(false);
    QSizePolicy sp = m_ui->yubikeyProgress->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_ui->yubikeyProgress->setSizePolicy(sp);

    connect(m_ui->buttonRedetectYubikey, SIGNAL(clicked()), SLOT(pollYubikey()));
#else
    m_ui->hardwareKeyLabel->setVisible(false);
    m_ui->hardwareKeyLabelHelp->setVisible(false);
    m_ui->buttonRedetectYubikey->setVisible(false);
    m_ui->comboChallengeResponse->setVisible(false);
    m_ui->yubikeyProgress->setVisible(false);
#endif

#ifndef WITH_XC_TOUCHID
    m_ui->touchIDContainer->setVisible(false);
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
    m_ui->fileNameLabel->setRawText(m_filename);

    m_ui->comboKeyFile->addItem(tr("Select key file..."), -1);
    m_ui->comboKeyFile->setCurrentIndex(0);
    m_ui->keyFileClearIcon->setVisible(false);
    m_keyFileComboEdited = false;

    if (config()->get("RememberLastKeyFiles").toBool()) {
        QHash<QString, QVariant> lastKeyFiles = config()->get("LastKeyFiles").toHash();
        if (lastKeyFiles.contains(m_filename)) {
            m_ui->comboKeyFile->addItem(lastKeyFiles[m_filename].toString());
            m_ui->comboKeyFile->setCurrentIndex(1);
        }
    }

    QHash<QString, QVariant> useTouchID = config()->get("UseTouchID").toHash();
    m_ui->checkTouchID->setChecked(useTouchID.value(m_filename, false).toBool());
}

void DatabaseOpenWidget::clearForms()
{
    m_ui->editPassword->setText("");
    m_ui->comboKeyFile->clear();
    m_ui->comboKeyFile->setEditText("");
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
    m_ui->editPassword->setText(pw);
    m_ui->comboKeyFile->setCurrentIndex(-1);
    m_ui->comboKeyFile->setEditText(keyFile);
    openDatabase();
}

void DatabaseOpenWidget::openDatabase()
{
    QSharedPointer<CompositeKey> masterKey = databaseKey();
    if (!masterKey) {
        return;
    }

    m_ui->editPassword->setShowPassword(false);
    m_ui->buttonTogglePassword->setChecked(false);
    QCoreApplication::processEvents();

    m_db.reset(new Database());
    QString error;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_ui->passwordFormFrame->setEnabled(false);
    QCoreApplication::processEvents();
    bool ok = m_db->open(m_filename, masterKey, &error, false);
    QApplication::restoreOverrideCursor();
    m_ui->passwordFormFrame->setEnabled(true);

    if (!ok) {
        if (m_ui->editPassword->text().isEmpty() && !m_retryUnlockWithEmptyPassword) {
            QScopedPointer<QMessageBox> msgBox(new QMessageBox(this));
            msgBox->setIcon(QMessageBox::Critical);
            msgBox->setWindowTitle(tr("Unlock failed and no password given"));
            msgBox->setText(tr("Unlocking the database failed and you did not enter a password.\n"
                               "Do you want to retry with an \"empty\" password instead?\n\n"
                               "To prevent this error from appearing, you must go to "
                               "\"Database Settings / Security\" and reset your password."));
            auto btn = msgBox->addButton(tr("Retry with empty password"), QMessageBox::ButtonRole::AcceptRole);
            msgBox->setDefaultButton(btn);
            msgBox->addButton(QMessageBox::Cancel);
            msgBox->exec();

            if (msgBox->clickedButton() == btn) {
                m_retryUnlockWithEmptyPassword = true;
                openDatabase();
                return;
            }
        }
        m_retryUnlockWithEmptyPassword = false;
        m_ui->messageWidget->showMessage(error, MessageWidget::MessageType::Error);
        // Focus on the password field and select the input for easy retry
        m_ui->editPassword->selectAll();
        m_ui->editPassword->setFocus();
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
        m_ui->messageWidget->showMessage(error, MessageWidget::Error);
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

    if (!m_ui->editPassword->text().isEmpty() || m_retryUnlockWithEmptyPassword) {
        masterKey->addKey(QSharedPointer<PasswordKey>::create(m_ui->editPassword->text()));
    }

#ifdef WITH_XC_TOUCHID
    // check if TouchID is available and enabled for unlocking the database
    if (m_ui->checkTouchID->isChecked() && TouchID::getInstance().isAvailable()
        && m_ui->editPassword->text().isEmpty()) {
        // clear empty password from composite key
        masterKey->clear();

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
    lastKeyFiles.remove(m_filename);

    auto key = QSharedPointer<FileKey>::create();
    QString keyFilename = m_ui->comboKeyFile->currentText();
    if (!m_ui->comboKeyFile->currentText().isEmpty() && m_keyFileComboEdited) {
        QString errorMsg;
        if (!key->load(keyFilename, &errorMsg)) {
            m_ui->messageWidget->showMessage(tr("Failed to open key file: %1").arg(errorMsg), MessageWidget::Error);
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
    }

    if (config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastKeyFiles", lastKeyFiles);
    }

#ifdef WITH_XC_YUBIKEY
    QHash<QString, QVariant> lastChallengeResponse = config()->get("LastChallengeResponse").toHash();
    lastChallengeResponse.remove(m_filename);

    int selectionIndex = m_ui->comboChallengeResponse->currentIndex();
    if (selectionIndex > 0) {
        int comboPayload = m_ui->comboChallengeResponse->itemData(selectionIndex).toInt();

        // read blocking mode from LSB and slot index number from second LSB
        bool blocking = comboPayload & 1;
        int slot = comboPayload >> 1;
        auto crKey = QSharedPointer<YkChallengeResponseKey>(new YkChallengeResponseKey(slot, blocking));
        masterKey->addChallengeResponseKey(crKey);
        lastChallengeResponse[m_filename] = true;
    }

    if (config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastChallengeResponse", lastChallengeResponse);
    }
#endif

    return masterKey;
}

void DatabaseOpenWidget::reject()
{
    emit dialogFinished(false);
}

void DatabaseOpenWidget::browseKeyFile()
{
    QString filters = QString("%1 (*);;%2 (*.key)").arg(tr("All files"), tr("Key files"));
    if (!config()->get("RememberLastKeyFiles").toBool()) {
        fileDialog()->setNextForgetDialog();
    }
    QString filename = fileDialog()->getOpenFileName(this, tr("Select key file"), QString(), filters);

    if (QFileInfo(filename).canonicalFilePath() == QFileInfo(m_filename).canonicalFilePath()) {
        MessageBox::warning(this,
                            tr("Cannot use database file as key file"),
                            tr("You cannot use your database file as a key file.\nIf you do not have a key file, "
                               "please leave the field empty."),
                            MessageBox::Button::Ok);
        filename = "";
    }

    if (!filename.isEmpty()) {
        m_ui->comboKeyFile->setCurrentIndex(-1);
        m_ui->comboKeyFile->setEditText(filename);
    }
}

void DatabaseOpenWidget::clearKeyFileEdit()
{
    m_ui->comboKeyFile->setCurrentIndex(0);
    // make sure that handler is called even if 0 was the current index already
    handleKeyFileComboChanged();
}

void DatabaseOpenWidget::handleKeyFileComboEdited()
{
    m_keyFileComboEdited = true;
    m_ui->keyFileClearIcon->setVisible(true);
}

void DatabaseOpenWidget::handleKeyFileComboChanged()
{
    m_keyFileComboEdited = m_ui->comboKeyFile->currentIndex() != 0;
    m_ui->keyFileClearIcon->setVisible(m_keyFileComboEdited);
}

void DatabaseOpenWidget::pollYubikey()
{
    m_ui->buttonRedetectYubikey->setEnabled(false);
    m_ui->comboChallengeResponse->setEnabled(false);
    m_ui->comboChallengeResponse->clear();
    m_ui->comboChallengeResponse->addItem(tr("Select slot..."), -1);
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
            m_ui->comboChallengeResponse->setCurrentIndex(1);
        }
    }
}

void DatabaseOpenWidget::yubikeyDetectComplete()
{
    m_ui->comboChallengeResponse->setEnabled(true);
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

void DatabaseOpenWidget::openHardwareKeyHelp()
{
    QDesktopServices::openUrl(QUrl("https://keepassxc.org/docs#faq-cat-yubikey"));
}

void DatabaseOpenWidget::openKeyFileHelp()
{
    QDesktopServices::openUrl(QUrl("https://keepassxc.org/docs#faq-cat-keyfile"));
}
