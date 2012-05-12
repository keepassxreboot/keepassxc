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

#include "KeePass1OpenDialog.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtGui/QMessageBox>

#include "ui_DatabaseOpenDialog.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "format/KeePass1Reader.h"

KeePass1OpenDialog::KeePass1OpenDialog(QFile* file, const QString& filename, QWidget* parent)
    : DatabaseOpenDialog(file, filename, parent)
{
    setWindowTitle(tr("Import KeePass1 database"));
}

KeePass1OpenDialog::~KeePass1OpenDialog()
{
    delete m_file;
}

void KeePass1OpenDialog::openDatabase()
{
    KeePass1Reader reader;

    QString password;
    QString keyFileName;

    if (m_ui->checkPassword->isChecked()) {
        password = m_ui->editPassword->text();
    }

    if (m_ui->checkKeyFile->isChecked()) {
        keyFileName = m_ui->comboKeyFile->currentText();
    }

    m_file->reset();
    m_db = reader.readDatabase(m_file, password, keyFileName);

    if (m_db) {
        m_db->metadata()->setName(QFileInfo(m_filename).completeBaseName());
        accept();
    }
    else {
        QMessageBox::warning(this, tr("Error"), tr("Unable to open the database.\n%1")
                             .arg(reader.errorString()));
        m_ui->editPassword->clear();
    }
}
