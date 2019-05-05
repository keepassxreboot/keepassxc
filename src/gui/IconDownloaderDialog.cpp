/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "IconDownloaderDialog.h"

#include <QtConcurrent>
#include <QMutexLocker>
#include <QTimer>

#include "core/AsyncTask.h"
#include "core/Config.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/IconModels.h"
#include "ui_IconDownloaderDialog.h"

#ifdef WITH_XC_NETWORKING
#include <QHostInfo>
#include <QNetworkAccessManager>
#include <QtNetwork>
#endif

#ifdef Q_OS_MACOS
#include "gui/macutils/MacUtils.h"
#endif

IconDownloaderDialog::IconDownloaderDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::IconDownloaderDialog())
    , m_dataModel(new QStandardItemModel(this))
    , m_db(nullptr)
    , m_customIconModel(new CustomIconModel(this))
    , m_parent(parent)
{
    m_ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setAttribute(Qt::WA_DeleteOnClose);
    m_ui->tableView->setModel(m_dataModel);
    m_ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_ui->progressBar->setMinimumHeight(20);
    m_dataModel->clear();
    m_dataModel->setHorizontalHeaderLabels({"URL", "Status"});

    connect(this, SIGNAL(entryUpdated()), parent, SIGNAL(databaseModified()));
}

IconDownloaderDialog::~IconDownloaderDialog()
{
    m_futureList.waitForFinished();
}

void IconDownloaderDialog::raiseWindow()
{
#ifdef Q_OS_MACOS
    macUtils()->raiseOwnWindow();
    Tools::wait(500);
#endif
    show();
    activateWindow();
    raise();
}

void IconDownloaderDialog::downloadFavicon(const QSharedPointer<Database>& database, Entry* entry)
{
    m_db = database;
#ifdef WITH_XC_NETWORKING
    QScopedPointer<IconDownloader> downloader(new IconDownloader());
    connect(downloader.data(), SIGNAL(iconError(Entry*)), this, SLOT(iconError(Entry*)));
    connect(downloader.data(), SIGNAL(fallbackNotEnabled()), this, SLOT(fallbackNotEnabled()));
    connect(downloader.data(), SIGNAL(iconReceived(const QImage&, Entry*)),
            this, SLOT(iconReceived(const QImage&, Entry*)));
    connect(m_ui->abortButton, SIGNAL(clicked()), downloader.data(), SLOT(abortRequest()));
    connect(m_parent, SIGNAL(databaseLocked()), downloader.data(), SLOT(abortRequest()));
    connect(m_parent, SIGNAL(databaseLocked()), this, SLOT(close()));

    m_mutex.lock();
    m_dataModel->appendRow(QList<QStandardItem*>() << new QStandardItem(entry->url())
                                                   << new QStandardItem(tr("Loading")));
    m_mutex.unlock();

    QTimer timer;
    connect(&timer, SIGNAL(timeout()), downloader.data(), SLOT(abortRequest()));
    if (config()->get("DownloadFavicon", false).toBool()) {
        timer.start(config()->get("DownloadFaviconTimeout").toInt() * 1000);
    }

    downloader->downloadFavicon(entry);

    QEventLoop loop;
    connect(downloader.data(), SIGNAL(iconReceived(const QImage&, Entry*)), &loop, SLOT(quit()));
    loop.exec();
    timer.stop();
#endif
}

void IconDownloaderDialog::downloadFavicons(const QSharedPointer<Database>& database, const QList<Entry*>& entries)
{
    if (entries.count() == 1) {
        connect(this, SIGNAL(messageEditEntry(QString, MessageWidget::MessageType)),
            m_parent, SLOT(showMessage(QString, MessageWidget::MessageType)));
        downloadFavicon(database, entries.first());
        return;
    }

    raiseWindow();
    auto result = AsyncTask::runAndWaitForFuture(
        [&]() { return downloadAllFavicons(database, entries); });
    Q_UNUSED(result);
}

bool IconDownloaderDialog::downloadAllFavicons(const QSharedPointer<Database>& database, const QList<Entry*>& entries)
{
    m_db = database;

    // Set progress bar
    int maximum = 0;
    for (const auto& e : entries) {
        if (!e->url().isEmpty() && e->iconUuid().isNull()) {
            ++maximum;
        }
    }
    m_ui->progressBar->setMinimum(0);
    m_ui->progressBar->setMaximum(maximum);

    for (const auto& e : entries) {
        if (!e->url().isEmpty() && e->iconUuid().isNull()) {
            QFuture<void> fut = QtConcurrent::run(this, &IconDownloaderDialog::downloadFavicon, database, e);
            m_futureList.addFuture(fut);
        }
    }

    m_futureList.waitForFinished();
    m_ui->abortButton->setEnabled(false);
    return true;
}

void IconDownloaderDialog::iconReceived(const QImage& icon, Entry* entry)
{
    m_mutex.lock();
    m_ui->progressBar->setValue(m_ui->progressBar->value() + 1);
    m_ui->label->setText(tr("Downloading favicon %1 of %n…", 0,
                            m_ui->progressBar->maximum()).arg(m_ui->progressBar->value()));
    m_mutex.unlock();
   
    if (icon.isNull() || !entry) {
        updateTable(entry, tr("Error"));
        emit messageEditEntry(tr("Unable to fetch favicon."), MessageWidget::Error);
        return;
    }

    if (!addCustomIcon(icon, entry)) {
        updateTable(entry, tr("Custom icon already exists"));
        return;
    }

    updateTable(entry, tr("Ok"));
}

bool IconDownloaderDialog::addCustomIcon(const QImage& icon, Entry* entry)
{
    bool added = false;
    if (m_db && !icon.isNull()) {
        // Don't add an icon larger than 128x128, but retain original size if smaller
        auto scaledicon = icon;
        if (icon.width() > 128 || icon.height() > 128) {
            scaledicon = icon.scaled(128, 128);
        }

        QUuid uuid = m_db->metadata()->findCustomIcon(scaledicon);
        if (uuid.isNull()) {
            uuid = QUuid::createUuid();
            m_db->metadata()->addCustomIcon(uuid, scaledicon);
            m_customIconModel->setIcons(m_db->metadata()->customIconsScaledPixmaps(),
                                        m_db->metadata()->customIconsOrder());
            added = true;
        } else {
            emit messageEditEntry(tr("Custom icon already exists"), MessageWidget::Information);
        }

        if (entry) {
            entry->setIcon(uuid);
        }

        emit entryUpdated();
    }

    return added;
}

void IconDownloaderDialog::fallbackNotEnabled()
{
#ifdef Q_OS_MACOS
    const QString settingsPath = tr("Preferences -> Security");
#else
    const QString settingsPath = tr("Tools -> Settings -> Security");
#endif

     emit messageEditEntry(
        tr("Unable to fetch favicon.") + "\n"
            + tr("You can enable the DuckDuckGo website icon service under %1").arg(settingsPath),
        MessageWidget::Error);
}

void IconDownloaderDialog::iconError(Entry* entry)
{
    updateTable(entry, tr("Unable to fetch favicon."));
    emit messageEditEntry(tr("Unable to fetch favicon."), MessageWidget::Error);
}

void IconDownloaderDialog::updateTable(Entry* entry, const QString& message)
{
    if (!entry) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_dataModel->rowCount(); ++i) {
        if (m_dataModel->item(i, 0)->text() == entry->url()) {
            m_dataModel->item(i, 1)->setText(message);
        }
    }
}

void IconDownloaderDialog::closeEvent(QCloseEvent* event)
{
    emit m_ui->abortButton->clicked();
    m_ui->abortButton->setEnabled(false);
    m_futureList.waitForFinished();
    event->accept();
}
