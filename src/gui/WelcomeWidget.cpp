/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QStyle>

#include "config-keepassx.h"
#include "core/Config.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"

namespace {
const int RawEntryRole = Qt::UserRole + 1;
}

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

    m_ui->iconLabel->setPixmap(icons()->applicationIcon().pixmap(64));
    m_ui->buttonNewDatabase->setIcon(icons()->icon("document-new"));
    m_ui->buttonNewDatabase->setStyleSheet("text-align:center;");
    m_ui->buttonOpenDatabase->setIcon(icons()->icon("document-open"));
    m_ui->buttonOpenDatabase->setStyleSheet("text-align:center;");
    m_ui->buttonImport->setIcon(icons()->icon("document-import"));
    m_ui->buttonImport->setStyleSheet("text-align:center;");

    refreshLastDatabases();

    connect(m_ui->buttonNewDatabase, SIGNAL(clicked()), SIGNAL(newDatabase()));
    connect(m_ui->buttonOpenDatabase, SIGNAL(clicked()), SIGNAL(openDatabase()));
    connect(m_ui->buttonImport, SIGNAL(clicked()), SIGNAL(importFile()));
    connect(m_ui->recentListWidget,
            SIGNAL(itemActivated(QListWidgetItem*)),
            this,
            SLOT(openDatabaseFromFile(QListWidgetItem*)));
}

WelcomeWidget::~WelcomeWidget() = default;

void WelcomeWidget::openDatabaseFromFile(QListWidgetItem* item)
{
    if (!item) {
        return;
    }
    QString raw = item->data(RawEntryRole).toString();
    if (raw.isEmpty()) {
        raw = item->text();
    }
    if (raw.isEmpty()) {
        return;
    }
    emit openDatabaseFile(raw);
}

void WelcomeWidget::removeFromLastDatabases(QListWidgetItem* item)
{
    if (!item) {
        return;
    }

    QString raw = item->data(RawEntryRole).toString();
    if (raw.isEmpty()) {
        raw = item->text();
    }
    if (raw.isEmpty()) {
        return;
    }

    QString displayName = raw;
    if (raw.startsWith(QLatin1String("drive:"))) {
        QStringList parts = raw.mid(6).split(QLatin1Char('|'));
        if (parts.size() >= 2) {
            displayName = parts[1];
        }
    }

    auto result = MessageBox::question(this,
        tr("Remove from recent files"),
        tr("Remove \"%1\" from the recent files list?").arg(displayName),
        MessageBox::Yes | MessageBox::No,
        MessageBox::No);

    if (result != MessageBox::Yes) {
        return;
    }

    if (config()->get(Config::RememberLastDatabases).toBool()) {
        QStringList lastDatabases = config()->get(Config::LastDatabases).toStringList();
        lastDatabases.removeOne(raw);
        config()->set(Config::LastDatabases, lastDatabases);
    }
    refreshLastDatabases();
}

void WelcomeWidget::refreshLastDatabases()
{
    m_ui->recentListWidget->clear();
    const QStringList lastDatabases = config()->get(Config::LastDatabases).toStringList();
    for (const QString& entry : lastDatabases) {
        auto itm = new QListWidgetItem;
        itm->setData(RawEntryRole, entry);
        itm->setSizeHint(QSize(0, 26));

        auto* row = new QWidget();
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 1, 4, 1);
        rowLayout->setSpacing(6);

        if (entry.startsWith(QLatin1String("drive:"))) {
            QStringList parts = entry.mid(6).split(QLatin1Char('|'));
            if (parts.size() >= 2) {
                auto* iconLabel = new QLabel();
                iconLabel->setPixmap(QIcon(":/icons/application/scalable/actions/google-drive.svg").pixmap(16, 16));
                rowLayout->addWidget(iconLabel);
                rowLayout->addWidget(new QLabel(parts[1]), 1);
            } else {
                rowLayout->addWidget(new QLabel(entry), 1);
            }
        } else {
            QFontMetrics fm(font());
            QString elided = fm.elidedText(entry, Qt::ElideMiddle, 300);
            rowLayout->addWidget(new QLabel(elided), 1);
        }

        auto* removeBtn = new QPushButton();
        removeBtn->setIcon(icons()->icon("dialog-close"));
        removeBtn->setFixedSize(20, 20);
        removeBtn->setFlat(true);
        removeBtn->setToolTip(tr("Remove from recent files"));
        connect(removeBtn, &QPushButton::clicked, this, [this, itm]() {
            removeFromLastDatabases(itm);
        });
        rowLayout->addWidget(removeBtn);

        m_ui->recentListWidget->addItem(itm);
        m_ui->recentListWidget->setItemWidget(itm, row);
    }

    bool recent_visibility = (m_ui->recentListWidget->count() > 0);
    m_ui->startLabel->setVisible(!recent_visibility);
    m_ui->recentListWidget->setVisible(recent_visibility);
    m_ui->recentLabel->setVisible(recent_visibility);
}

void WelcomeWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_ui->recentListWidget->hasFocus()) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            openDatabaseFromFile(m_ui->recentListWidget->currentItem());
        } else if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            removeFromLastDatabases(m_ui->recentListWidget->currentItem());
        }
    }

    QWidget::keyPressEvent(event);
}

void WelcomeWidget::showEvent(QShowEvent* event)
{
    refreshLastDatabases();
    QWidget::showEvent(event);
}
