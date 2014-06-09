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

#include "KeePass1OpenWidget.h"

#include <QFile>
#include <QFileInfo>

#include "ui_DatabaseOpenWidget.h"
#include "core/Database.h"
#include "core/Metadata.h"
#include "format/KeePass1Reader.h"
#include "gui/MessageBox.h"

KeePass1OpenWidget::KeePass1OpenWidget(QWidget* parent)
    : DatabaseOpenWidget(parent)
{
    m_ui->labelHeadline->setText(tr("Import KeePass1 database"));
}

void KeePass1OpenWidget::openDatabase()
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

    QFile file(m_filename);
    if (!file.open(QIODevice::ReadOnly)) {
        // TODO: error message
        return;
    }
    if (m_db) {
        delete m_db;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_db = reader.readDatabase(&file, password, keyFileName);
    QApplication::restoreOverrideCursor();

    if (m_db) {
        m_db->metadata()->setName(QFileInfo(m_filename).completeBaseName());
        Q_EMIT editFinished(true);
    }
    else {
        MessageBox::warning(this, tr("Error"), tr("Unable to open the database.").append("\n")
                            .append(reader.errorString()));
        m_ui->editPassword->clear();
    }
}
