/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "WelcomeWidget.h"
#include "ui_WelcomeWidget.h"
#include <QKeyEvent>

#include "config-keepassx.h"
#include "core/Config.h"
#include "core/FilePath.h"

WelcomeWidget::WelcomeWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::WelcomeWidget())
{
    m_ui->setupUi(this);

    m_ui->welcomeLabel->setText(tr("Welcome to KeePassXC %1").arg(KEEPASSXC_VERSION));
    QFont welcomeLabelFont = m_ui->welcomeLabel->font();
    welcomeLabelFont.setBold(true);
    welcomeLabelFont.setPointSize(welcomeLabelFont.pointSize() + 4);
    m_ui->welcomeLabel->setFont(welcomeLabelFont);

    m_ui->iconLabel->setPixmap(filePath()->applicationIcon().pixmap(64));

    refreshLastDatabases();

    bool recent_visibility = (m_ui->recentListWidget->count() > 0);
    m_ui->startLabel->setVisible(!recent_visibility);
    m_ui->recentListWidget->setVisible(recent_visibility);
    m_ui->recentLabel->setVisible(recent_visibility);

    connect(m_ui->buttonNewDatabase, SIGNAL(clicked()), SIGNAL(newDatabase()));
    connect(m_ui->buttonOpenDatabase, SIGNAL(clicked()), SIGNAL(openDatabase()));
    connect(m_ui->buttonImportKeePass1, SIGNAL(clicked()), SIGNAL(importKeePass1Database()));
    connect(m_ui->buttonImportCSV, SIGNAL(clicked()), SIGNAL(importCsv()));
    connect(m_ui->recentListWidget,
            SIGNAL(itemActivated(QListWidgetItem*)),
            this,
            SLOT(openDatabaseFromFile(QListWidgetItem*)));
}

WelcomeWidget::~WelcomeWidget()
{
}

void WelcomeWidget::openDatabaseFromFile(QListWidgetItem* item)
{
    if (item->text().isEmpty()) {
        return;
    }
    emit openDatabaseFile(item->text());
}

void WelcomeWidget::refreshLastDatabases()
{
    m_ui->recentListWidget->clear();
    const QStringList lastDatabases = config()->get("LastDatabases", QVariant()).toStringList();
    for (const QString& database : lastDatabases) {
        QListWidgetItem* itm = new QListWidgetItem;
        itm->setText(database);
        m_ui->recentListWidget->addItem(itm);
    }
}

void WelcomeWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_ui->recentListWidget->hasFocus() && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)) {
        openDatabaseFromFile(m_ui->recentListWidget->currentItem());
    }

    QWidget::keyPressEvent(event);
}
