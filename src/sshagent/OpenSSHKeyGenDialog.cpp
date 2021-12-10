/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "OpenSSHKeyGenDialog.h"
#include "OpenSSHKey.h"
#include "OpenSSHKeyGen.h"
#include "gui/Icons.h"
#include "ui_OpenSSHKeyGenDialog.h"
#include <QHostInfo>
#include <QProcessEnvironment>

OpenSSHKeyGenDialog::OpenSSHKeyGenDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::OpenSSHKeyGenDialog())
    , m_key(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowIcon(icons()->icon("password-generator"));

    m_ui->setupUi(this);

    m_ui->typeComboBox->clear();
    m_ui->typeComboBox->addItem("Ed25519");
    m_ui->typeComboBox->addItem("RSA");
    m_ui->typeComboBox->addItem("ECDSA");

    QString user = QProcessEnvironment::systemEnvironment().value("USER");
    m_ui->commentLineEdit->setText(user + "@" + QHostInfo::localHostName());

    connect(m_ui->typeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(typeChanged()));

    typeChanged();
}

// Required for QScopedPointer
OpenSSHKeyGenDialog::~OpenSSHKeyGenDialog()
{
}

void OpenSSHKeyGenDialog::typeChanged()
{
    m_ui->bitsComboBox->clear();

    if (m_ui->typeComboBox->currentText() == QString("Ed25519")) {
        m_ui->bitsComboBox->addItem("32");
    } else if (m_ui->typeComboBox->currentText() == QString("RSA")) {
        m_ui->bitsComboBox->addItem("2048");
        m_ui->bitsComboBox->addItem("3072");
        m_ui->bitsComboBox->addItem("4096");
        m_ui->bitsComboBox->setCurrentText("3072");
    } else if (m_ui->typeComboBox->currentText() == QString("ECDSA")) {
        m_ui->bitsComboBox->addItem("256");
        m_ui->bitsComboBox->addItem("384");
        m_ui->bitsComboBox->addItem("521");
        m_ui->bitsComboBox->setCurrentText("256");
    }
}

void OpenSSHKeyGenDialog::accept()
{
    // disable form and try to process this update before blocking in key generation
    setEnabled(false);
    QCoreApplication::processEvents();

    int bits = m_ui->bitsComboBox->currentText().toInt();

    if (m_ui->typeComboBox->currentText() == QString("Ed25519")) {
        OpenSSHKeyGen::generateEd25519(*m_key);
    } else if (m_ui->typeComboBox->currentText() == QString("RSA")) {
        OpenSSHKeyGen::generateRSA(*m_key, bits);
    } else if (m_ui->typeComboBox->currentText() == QString("ECDSA")) {
        OpenSSHKeyGen::generateECDSA(*m_key, bits);
    } else {
        reject();
        return;
    }

    m_key->setComment(m_ui->commentLineEdit->text());
    QDialog::accept();
}

void OpenSSHKeyGenDialog::setKey(OpenSSHKey* key)
{
    m_key = key;
}
