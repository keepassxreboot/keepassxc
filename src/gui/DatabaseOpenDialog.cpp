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

#include "DatabaseOpenDialog.h"
#include "ui_DatabaseOpenDialog.h"

#include <QtGui/QMessageBox>

#include "core/Config.h"
#include "gui/FileDialog.h"
#include "format/KeePass2Reader.h"
#include "keys/FileKey.h"
#include "keys/PasswordKey.h"

DatabaseOpenDialog::DatabaseOpenDialog(QFile* file, QString filename, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::DatabaseOpenDialog())
    , m_db(0)
    , m_file(file)
    , m_filename(filename)
{
    m_ui->setupUi(this);

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    connect(m_ui->buttonTogglePassword, SIGNAL(toggled(bool)), SLOT(togglePassword(bool)));
    connect(m_ui->buttonBrowseFile, SIGNAL(clicked()), SLOT(browseKeyFile()));

    connect(m_ui->editPassword, SIGNAL(textChanged(QString)), SLOT(activatePassword()));
    connect(m_ui->comboKeyFile, SIGNAL(editTextChanged(QString)), SLOT(activateKeyFile()));

    connect(m_ui->checkPassword, SIGNAL(toggled(bool)), SLOT(setOkButtonEnabled()));
    connect(m_ui->checkKeyFile, SIGNAL(toggled(bool)), SLOT(setOkButtonEnabled()));
    connect(m_ui->comboKeyFile, SIGNAL(editTextChanged(QString)), SLOT(setOkButtonEnabled()));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(openDatabase()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    QHash<QString, QVariant> lastKeyFiles = config()->get("LastKeyFiles").toHash();
    if (lastKeyFiles.contains(m_filename)) {
        m_ui->checkKeyFile->setChecked(true);
        m_ui->comboKeyFile->addItem(lastKeyFiles[m_filename].toString());
    }

    m_ui->editPassword->setFocus();
}

DatabaseOpenDialog::~DatabaseOpenDialog()
{
}

Database* DatabaseOpenDialog::database()
{
    return m_db;
}

void DatabaseOpenDialog::enterKey(const QString& pw, const QString& keyFile)
{
    if (!pw.isNull()) {
        m_ui->editPassword->setText(pw);
    }
    if (!keyFile.isEmpty()) {
        m_ui->checkKeyFile->setText(keyFile);
    }

    openDatabase();
}

void DatabaseOpenDialog::openDatabase()
{
    KeePass2Reader reader;
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
            QMessageBox::warning(this, tr("Error"), tr("Can't open key file:\n%1").arg(errorMsg));
            return;
        }
        masterKey.addKey(key);
        lastKeyFiles[m_filename] = keyFilename;
    }
    else {
        lastKeyFiles.remove(m_filename);
    }

    config()->set("LastKeyFiles", lastKeyFiles);

    m_file->reset();
    m_db = reader.readDatabase(m_file, masterKey);

    if (m_db) {
        accept();
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("Unable to open the database.\n%1").arg(reader.errorString()));
        m_ui->editPassword->clear();
    }
}

void DatabaseOpenDialog::togglePassword(bool checked)
{
    m_ui->editPassword->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}

void DatabaseOpenDialog::activatePassword()
{
    m_ui->checkPassword->setChecked(true);
}

void DatabaseOpenDialog::activateKeyFile()
{
    m_ui->checkKeyFile->setChecked(true);
}

void DatabaseOpenDialog::setOkButtonEnabled()
{
    bool enable = m_ui->checkPassword->isChecked() || (m_ui->checkKeyFile->isChecked() && !m_ui->comboKeyFile->currentText().isEmpty());

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void DatabaseOpenDialog::browseKeyFile()
{
    QString filters = QString("%1 (*);;%2 (*.key)").arg(tr("All files"), tr("Key files"));
    QString filename = fileDialog()->getOpenFileName(this, tr("Select key file"), QString(), filters);

    if (!filename.isEmpty()) {
        m_ui->comboKeyFile->lineEdit()->setText(filename);
    }
}
