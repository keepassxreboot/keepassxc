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

#include "EditWidgetIcons.h"
#include "ui_EditWidgetIcons.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileDialog>

#include "core/Config.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/IconModels.h"
#include "gui/MessageBox.h"

#ifdef WITH_XC_NETWORKING
#include <curl/curl.h>
#include "core/AsyncTask.h"
#undef MessageBox
#endif

IconStruct::IconStruct()
    : uuid(Uuid())
    , number(0)
{
}

EditWidgetIcons::EditWidgetIcons(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EditWidgetIcons())
    , m_database(nullptr)
    , m_defaultIconModel(new DefaultIconModel(this))
    , m_customIconModel(new CustomIconModel(this))
{
    m_ui->setupUi(this);

    m_ui->defaultIconsView->setModel(m_defaultIconModel);
    m_ui->customIconsView->setModel(m_customIconModel);

    connect(m_ui->defaultIconsView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(updateRadioButtonDefaultIcons()));
    connect(m_ui->customIconsView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(updateRadioButtonCustomIcons()));
    connect(m_ui->defaultIconsRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateWidgetsDefaultIcons(bool)));
    connect(m_ui->customIconsRadio, SIGNAL(toggled(bool)),
            this, SLOT(updateWidgetsCustomIcons(bool)));
    connect(m_ui->addButton, SIGNAL(clicked()), SLOT(addCustomIconFromFile()));
    connect(m_ui->deleteButton, SIGNAL(clicked()), SLOT(removeCustomIcon()));
    connect(m_ui->faviconButton, SIGNAL(clicked()), SLOT(downloadFavicon()));

    m_ui->faviconButton->setVisible(false);
}

EditWidgetIcons::~EditWidgetIcons()
{
}

IconStruct EditWidgetIcons::state()
{
    Q_ASSERT(m_database);
    Q_ASSERT(!m_currentUuid.isNull());

    IconStruct iconStruct;
    if (m_ui->defaultIconsRadio->isChecked()) {
        QModelIndex index = m_ui->defaultIconsView->currentIndex();
        if (index.isValid()) {
            iconStruct.number = index.row();
        } else {
            Q_ASSERT(false);
        }
    } else {
        QModelIndex index = m_ui->customIconsView->currentIndex();
        if (index.isValid()) {
            iconStruct.uuid = m_customIconModel->uuidFromIndex(m_ui->customIconsView->currentIndex());
        } else {
            iconStruct.number = -1;
        }
    }

    return iconStruct;
}

void EditWidgetIcons::reset()
{
    m_database = nullptr;
    m_currentUuid = Uuid();
}

void EditWidgetIcons::load(const Uuid& currentUuid, Database* database, const IconStruct& iconStruct, const QString& url)
{
    Q_ASSERT(database);
    Q_ASSERT(!currentUuid.isNull());

    m_database = database;
    m_currentUuid = currentUuid;
    setUrl(url);

    m_customIconModel->setIcons(database->metadata()->customIconsScaledPixmaps(),
                                database->metadata()->customIconsOrder());

    Uuid iconUuid = iconStruct.uuid;
    if (iconUuid.isNull()) {
        int iconNumber = iconStruct.number;
        m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(iconNumber, 0));
        m_ui->defaultIconsRadio->setChecked(true);
    } else {
        QModelIndex index = m_customIconModel->indexFromUuid(iconUuid);
        if (index.isValid()) {
            m_ui->customIconsView->setCurrentIndex(index);
            m_ui->customIconsRadio->setChecked(true);
        } else {
            m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(0, 0));
            m_ui->defaultIconsRadio->setChecked(true);
        }
    }
}

void EditWidgetIcons::setUrl(const QString& url)
{
#ifdef WITH_XC_NETWORKING
    m_url = url;
    m_ui->faviconButton->setVisible(!url.isEmpty());
#else
    Q_UNUSED(url);
    m_ui->faviconButton->setVisible(false);
#endif
}

void EditWidgetIcons::downloadFavicon()
{
#ifdef WITH_XC_NETWORKING
    m_ui->faviconButton->setDisabled(true);

    QUrl url = QUrl(m_url);
    url.setPath("/favicon.ico");
    // Attempt to simply load the favicon.ico file
    QImage image = fetchFavicon(url);
    if (!image.isNull()) {
        addCustomIcon(image);
    } else if (config()->get("security/IconDownloadFallbackToGoogle", false).toBool()) {
        QUrl faviconUrl = QUrl("https://www.google.com/s2/favicons");
        faviconUrl.setQuery("domain=" + QUrl::toPercentEncoding(url.host()));
        // Attempt to load favicon from Google
        image = fetchFavicon(faviconUrl);
        if (!image.isNull()) {
            addCustomIcon(image);
        } else {
            emit messageEditEntry(tr("Unable to fetch favicon."), MessageWidget::Error);
        }
    } else {
        emit messageEditEntry(tr("Unable to fetch favicon.") + "\n" +
                              tr("Hint: You can enable Google as a fallback under Tools>Settings>Security"),
                              MessageWidget::Error);
    }

    m_ui->faviconButton->setDisabled(false);
#endif
}

#ifdef WITH_XC_NETWORKING
namespace {
std::size_t writeCurlResponse(char* ptr, std::size_t size, std::size_t nmemb, void* data)
{
    QByteArray* response = static_cast<QByteArray*>(data);
    std::size_t realsize = size * nmemb;
    response->append(ptr, realsize);
    return realsize;
}
}

QImage EditWidgetIcons::fetchFavicon(const QUrl& url)
{
    QImage image;
    CURL* curl = curl_easy_init();
    if (curl) {
        QByteArray imagedata;
        QByteArray baUrl = url.url().toLatin1();

        curl_easy_setopt(curl, CURLOPT_URL, baUrl.data());
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imagedata);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCurlResponse);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
#ifdef Q_OS_WIN
        const QDir appDir = QFileInfo(QCoreApplication::applicationFilePath()).absoluteDir();
        if (appDir.exists("ssl\\certs")) {
            curl_easy_setopt(curl, CURLOPT_CAINFO, (appDir.absolutePath() + "\\ssl\\certs\\ca-bundle.crt").toLatin1().data());
        }
#endif

        // Perform the request in another thread
        CURLcode result = AsyncTask::runAndWaitForFuture([curl]() {
            return curl_easy_perform(curl);
        });

        if (result == CURLE_OK) {
            image.loadFromData(imagedata);
        }

        curl_easy_cleanup(curl);
    }

    return image;
}
#endif

void EditWidgetIcons::addCustomIconFromFile()
{
    if (m_database) {
        QString filter = QString("%1 (%2);;%3 (*)").arg(tr("Images"),
                    Tools::imageReaderFilter(), tr("All files"));

        QString filename = QFileDialog::getOpenFileName(
                    this, tr("Select Image"), "", filter);
        if (!filename.isEmpty()) {
            auto icon = QImage(filename);
            if (!icon.isNull()) {
                addCustomIcon(QImage(filename));
            } else {
                emit messageEditEntry(tr("Can't read icon"), MessageWidget::Error);
            }
        }
    }
}

void EditWidgetIcons::addCustomIcon(const QImage& icon)
{
    if (m_database) {
        Uuid uuid = m_database->metadata()->findCustomIcon(icon);
        if (uuid.isNull()) {
            uuid = Uuid::random();
            // Don't add an icon larger than 128x128, but retain original size if smaller
            if (icon.width() > 128 || icon.height() > 128) {
                m_database->metadata()->addCustomIcon(uuid, icon.scaled(128, 128));
            } else {
                m_database->metadata()->addCustomIcon(uuid, icon);
            }

            m_customIconModel->setIcons(m_database->metadata()->customIconsScaledPixmaps(),
                                        m_database->metadata()->customIconsOrder());
        } else {
            emit messageEditEntry(tr("Custom icon already exists"), MessageWidget::Information);
        }

        // Select the new or existing icon
        updateRadioButtonCustomIcons();
        QModelIndex index = m_customIconModel->indexFromUuid(uuid);
        m_ui->customIconsView->setCurrentIndex(index);
    }
}

void EditWidgetIcons::removeCustomIcon()
{
    if (m_database) {
        QModelIndex index = m_ui->customIconsView->currentIndex();
        if (index.isValid()) {
            Uuid iconUuid = m_customIconModel->uuidFromIndex(index);

            const QList<Entry*> allEntries = m_database->rootGroup()->entriesRecursive(true);
            QList<Entry*> entriesWithSameIcon;
            QList<Entry*> historyEntriesWithSameIcon;

            for (Entry* entry : allEntries) {
                if (iconUuid == entry->iconUuid()) {
                    // Check if this is a history entry (no assigned group)
                    if (!entry->group()) {
                        historyEntriesWithSameIcon << entry;
                    } else if (m_currentUuid != entry->uuid()) {
                        entriesWithSameIcon << entry;
                    }
                }
            }

            const QList<Group*> allGroups = m_database->rootGroup()->groupsRecursive(true);
            QList<Group*> groupsWithSameIcon;

            for (Group* group : allGroups) {
                if (iconUuid == group->iconUuid() && m_currentUuid != group->uuid()) {
                    groupsWithSameIcon << group;
                }
            }

            int iconUseCount = entriesWithSameIcon.size() + groupsWithSameIcon.size();
            if (iconUseCount > 0) {
                QMessageBox::StandardButton ans = MessageBox::question(this, tr("Confirm Delete"),
                                     tr("This icon is used by %1 entries, and will be replaced "
                                        "by the default icon. Are you sure you want to delete it?")
                                     .arg(iconUseCount), QMessageBox::Yes | QMessageBox::No);

                if (ans == QMessageBox::No) {
                    // Early out, nothing is changed
                    return;
                } else {
                    // Revert matched entries to the default entry icon
                    for (Entry* entry : asConst(entriesWithSameIcon)) {
                        entry->setIcon(Entry::DefaultIconNumber);
                    }

                    // Revert matched groups to the default group icon
                    for (Group* group : asConst(groupsWithSameIcon)) {
                        group->setIcon(Group::DefaultIconNumber);
                    }
                }
            }


            // Remove the icon from history entries
            for (Entry* entry : asConst(historyEntriesWithSameIcon)) {
                entry->setUpdateTimeinfo(false);
                entry->setIcon(0);
                entry->setUpdateTimeinfo(true);
            }

            // Remove the icon from the database
            m_database->metadata()->removeCustomIcon(iconUuid);
            m_customIconModel->setIcons(m_database->metadata()->customIconsScaledPixmaps(),
                                        m_database->metadata()->customIconsOrder());

            // Reset the current icon view
            updateRadioButtonDefaultIcons();

            if (m_database->resolveEntry(m_currentUuid) != nullptr) {
                m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(Entry::DefaultIconNumber));
            } else {
                m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(Group::DefaultIconNumber));
            }
        }
    }
}

void EditWidgetIcons::updateWidgetsDefaultIcons(bool check)
{
    if (check) {
        QModelIndex index = m_ui->defaultIconsView->currentIndex();
        if (!index.isValid()) {
            m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(0, 0));
        } else {
            m_ui->defaultIconsView->setCurrentIndex(index);
        }
        m_ui->customIconsView->selectionModel()->clearSelection();
        m_ui->addButton->setEnabled(false);
        m_ui->deleteButton->setEnabled(false);
    }
}

void EditWidgetIcons::updateWidgetsCustomIcons(bool check)
{
    if (check) {
        QModelIndex index = m_ui->customIconsView->currentIndex();
        if (!index.isValid()) {
            m_ui->customIconsView->setCurrentIndex(m_customIconModel->index(0, 0));
        } else {
            m_ui->customIconsView->setCurrentIndex(index);
        }
        m_ui->defaultIconsView->selectionModel()->clearSelection();
        m_ui->addButton->setEnabled(true);
        m_ui->deleteButton->setEnabled(true);
    }
}

void EditWidgetIcons::updateRadioButtonDefaultIcons()
{
    m_ui->defaultIconsRadio->setChecked(true);
}

void EditWidgetIcons::updateRadioButtonCustomIcons()
{
    m_ui->customIconsRadio->setChecked(true);
}
