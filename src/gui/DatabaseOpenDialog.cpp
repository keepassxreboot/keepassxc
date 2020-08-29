/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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
#include "DatabaseOpenWidget.h"
#include "DatabaseWidget.h"
#include "core/Database.h"

DatabaseOpenDialog::DatabaseOpenDialog(QWidget* parent)
    : QDialog(parent)
    , m_view(new DatabaseOpenWidget(this))
{
    setWindowTitle(tr("Unlock Database - KeePassXC"));
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
    connect(m_view, SIGNAL(dialogFinished(bool)), this, SLOT(complete(bool)));
    auto* layout = new QVBoxLayout();
    layout->setMargin(0);
    setLayout(layout);
    layout->addWidget(m_view);
    setMinimumWidth(700);
}

void DatabaseOpenDialog::setFilePath(const QString& filePath)
{
    m_view->load(filePath);
}

/**
 * Set target DatabaseWidget to which signals are connected.
 *
 * @param dbWidget database widget
 */
void DatabaseOpenDialog::setTargetDatabaseWidget(DatabaseWidget* dbWidget)
{
    if (m_dbWidget) {
        disconnect(this, nullptr, m_dbWidget, nullptr);
    }
    m_dbWidget = dbWidget;
    connect(this, &DatabaseOpenDialog::dialogFinished, dbWidget, &DatabaseWidget::unlockDatabase);
}

void DatabaseOpenDialog::setIntent(DatabaseOpenDialog::Intent intent)
{
    m_intent = intent;
}

DatabaseOpenDialog::Intent DatabaseOpenDialog::intent() const
{
    return m_intent;
}

void DatabaseOpenDialog::clearForms()
{
    m_view->clearForms();
    m_db.reset();
    m_intent = Intent::None;
    if (m_dbWidget) {
        disconnect(this, nullptr, m_dbWidget, nullptr);
        m_dbWidget = nullptr;
    }
}

QSharedPointer<Database> DatabaseOpenDialog::database()
{
    return m_db;
}

void DatabaseOpenDialog::complete(bool accepted)
{
    // save DB, since DatabaseOpenWidget will reset its data after accept() is called
    m_db = m_view->database();

    if (accepted) {
        accept();
    } else {
        reject();
    }
    emit dialogFinished(accepted, m_dbWidget);
    clearForms();
}
