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


CsvImportWizard::CsvImportWizard(QWidget *parent)
    : DialogyWidget(parent)
{
    m_layout = new QGridLayout(this);
    m_pages = new QStackedWidget(parent);
    m_layout->addWidget(m_pages, 0, 0);

    m_pages->addWidget(key = new ChangeMasterKeyWidget(m_pages));
    m_pages->addWidget(parse = new CsvImportWidget(m_pages));
    key->headlineLabel()->setText(tr("Import CSV file"));
    QFont headLineFont = key->headlineLabel()->font();
    headLineFont.setBold(true);
    headLineFont.setPointSize(headLineFont.pointSize() + 2);
    key->headlineLabel()->setFont(headLineFont);

    connect(key, SIGNAL(editFinished(bool)), this, SLOT(keyFinished(bool)));
    connect(parse, SIGNAL(editFinished(bool)), this, SLOT(parseFinished(bool)));
}

CsvImportWizard::~CsvImportWizard()
{}

void CsvImportWizard::load(const QString& filename, Database* database)
{
    m_db = database;
    parse->load(filename, database);
    key->clearForms();
}

void CsvImportWizard::keyFinished(bool accepted)
{
    if (!accepted) {
        emit(importFinished(false));
        return;
    }

    m_pages->setCurrentIndex(m_pages->currentIndex()+1);

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool result = m_db->setKey(key->newMasterKey());
    QApplication::restoreOverrideCursor();

    if (!result) {
        MessageBox::critical(this, tr("Error"), tr("Unable to calculate master key"));
        emit(importFinished(false));
    }
}

void CsvImportWizard::parseFinished(bool accepted)
{
    emit(importFinished(accepted));
}
