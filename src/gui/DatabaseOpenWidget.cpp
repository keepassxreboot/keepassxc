/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include <QtConcurrentRun>

#include "DatabaseOpenWidget.h"
#include "ui_DatabaseOpenWidget.h"

#include "core/Config.h"
#include "core/Database.h"
#include "core/FilePath.h"
#include "gui/MainWindow.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include "format/KeePass2Reader.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"
#include "keys/YkChallengeResponseKey.h"
#include "crypto/Random.h"


DatabaseOpenWidget::DatabaseOpenWidget(QWidget* parent)
    : DialogyWidget(parent)
    , m_ui(new Ui::DatabaseOpenWidget())
    , m_db(nullptr)
{
    m_ui->setupUi(this);

    QFont font = m_ui->labelHeadline->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 2);
    m_ui->labelHeadline->setFont(font);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    m_ui->buttonTogglePassword->setIcon(filePath()->onOffIcon("actions", "password-show"));
    connect(m_ui->buttonTogglePassword, SIGNAL(toggled(bool)),
            m_ui->editPassword, SLOT(setShowPassword(bool)));
    connect(m_ui->buttonBrowseFile, SIGNAL(clicked()), SLOT(browseKeyFile()));

    connect(m_ui->editPassword, SIGNAL(textChanged(QString)), SLOT(activatePassword()));
    connect(m_ui->comboKeyFile, SIGNAL(editTextChanged(QString)), SLOT(activateKeyFile()));
    connect(m_ui->comboChallengeResponse, SIGNAL(activated(int)), SLOT(activateChallengeResponse()));

    connect(m_ui->checkPassword, SIGNAL(toggled(bool)), SLOT(setOkButtonEnabled()));
    connect(m_ui->checkKeyFile, SIGNAL(toggled(bool)), SLOT(setOkButtonEnabled()));
    connect(m_ui->comboKeyFile, SIGNAL(editTextChanged(QString)), SLOT(setOkButtonEnabled()));
    connect(m_ui->checkChallengeResponse, SIGNAL(toggled(bool)), SLOT(setOkButtonEnabled()));
    connect(m_ui->comboChallengeResponse, SIGNAL(activated(int)), SLOT(setOkButtonEnabled()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(openDatabase()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    connect(YubiKey::instance(), SIGNAL(detected(int,bool)),
                                 SLOT(ykDetected(int,bool)),
                                 Qt::QueuedConnection);
}

DatabaseOpenWidget::~DatabaseOpenWidget()
{
}

void DatabaseOpenWidget::showEvent(QShowEvent* event)
{
    DialogyWidget::showEvent(event);
    m_ui->editPassword->setFocus();
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

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    /* YubiKey init is slow, detect asynchronously to not block the UI */
    m_ui->comboChallengeResponse->clear();
    QtConcurrent::run(YubiKey::instance(), &YubiKey::detect);

    m_ui->editPassword->setFocus();
}

Database* DatabaseOpenWidget::database()
{
    return m_db;
}

void DatabaseOpenWidget::enterKey(const QString& pw, const QString& keyFile)
{
    if (!pw.isNull()) {
        m_ui->editPassword->setText(pw);
    }
    if (!keyFile.isEmpty()) {
        m_ui->comboKeyFile->setEditText(keyFile);
    }

    openDatabase();
}

void DatabaseOpenWidget::openDatabase()
{
    KeePass2Reader reader;
    CompositeKey masterKey = databaseKey();

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        MessageBox::warning(this, tr("Error"), tr("Unable to open the database.").append("\n")
                            .append(file.errorString()));
        return;
    }
    if (m_db) {
        delete m_db;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_db = reader.readDatabase(&file, masterKey);
    QApplication::restoreOverrideCursor();

    if (m_db) {
        Q_EMIT editFinished(true);
    }
    else {
        MessageBox::warning(this, tr("Error"), tr("Unable to open the database.").append("\n")
                            .append(reader.errorString()));
        m_ui->editPassword->clear();
    }
}

CompositeKey DatabaseOpenWidget::databaseKey()
{
    CompositeKey masterKey;

    if (m_ui->checkPassword->isChecked()) {
        masterKey.addKey(PasswordKey(m_ui->editPassword->text()));
    }

    QHash<QString, QVariant> lastKeyFiles = config()->get("LastKeyFiles").toHash();

    if (m_ui->checkKeyFile->isChecked()) {
        FileKey key;
        QString keyFilename = m_ui->comboKeyFile->currentText();
        QString errorMsg;
        if (!key.load(keyFilename, &errorMsg)) {
            MessageBox::warning(this, tr("Error"), tr("Can't open key file").append(":\n").append(errorMsg));
            return CompositeKey();
        }
        masterKey.addKey(key);
        lastKeyFiles[m_filename] = keyFilename;
    }
    else {
        lastKeyFiles.remove(m_filename);
    }

    if (config()->get("RememberLastKeyFiles").toBool()) {
        config()->set("LastKeyFiles", lastKeyFiles);
    }


    if (m_ui->checkChallengeResponse->isChecked()) {
        int i = m_ui->comboChallengeResponse->currentIndex();
        i = m_ui->comboChallengeResponse->itemData(i).toInt();
        YkChallengeResponseKey key(i);

        masterKey.addChallengeResponseKey(key);
    }

    return masterKey;
}

void DatabaseOpenWidget::reject()
{
    Q_EMIT editFinished(false);
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

void DatabaseOpenWidget::setOkButtonEnabled()
{
    bool enable = m_ui->checkPassword->isChecked() || m_ui->checkChallengeResponse->isChecked()
            || (m_ui->checkKeyFile->isChecked() && !m_ui->comboKeyFile->currentText().isEmpty());

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void DatabaseOpenWidget::browseKeyFile()
{
    QString filters = QString("%1 (*);;%2 (*.key)").arg(tr("All files"), tr("Key files"));
    QString filename = fileDialog()->getOpenFileName(this, tr("Select key file"), QString(), filters);

    if (!filename.isEmpty()) {
        m_ui->comboKeyFile->lineEdit()->setText(filename);
    }
}

void DatabaseOpenWidget::ykDetected(int slot, bool blocking)
{
    YkChallengeResponseKey yk(slot, blocking);
    m_ui->comboChallengeResponse->addItem(yk.getName(), QVariant(slot));
    m_ui->comboChallengeResponse->setEnabled(true);
    m_ui->checkChallengeResponse->setEnabled(true);
}
