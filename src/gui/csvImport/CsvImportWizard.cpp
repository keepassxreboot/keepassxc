/*
 *  Copyright (C) 2016 Enrico Mariotti <enricomariotti@yahoo.it>
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

#include "CsvImportWizard.h"

#include <QApplication>
#include <QLabel>

#include "gui/MessageBox.h"

CsvImportWizard::CsvImportWizard(QWidget* parent)
    : DialogyWidget(parent)
{
    m_layout = new QGridLayout(this);
    m_layout->addWidget(m_parse = new CsvImportWidget(this), 0, 0);

    connect(m_parse, SIGNAL(editFinished(bool)), this, SLOT(parseFinished(bool)));
}

CsvImportWizard::~CsvImportWizard()
{
}

void CsvImportWizard::load(const QString& filename, Database* database)
{
    m_db = database;
    m_parse->load(filename, database);
}

void CsvImportWizard::keyFinished(bool accepted, CompositeKey key)
{
    if (!accepted) {
        emit importFinished(false);
        return;
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool result = m_db->setKey(key);
    QApplication::restoreOverrideCursor();

    if (!result) {
        MessageBox::critical(this, tr("Error"), tr("Unable to calculate master key"));
        emit importFinished(false);
    }
}

void CsvImportWizard::parseFinished(bool accepted)
{
    emit importFinished(accepted);
}
