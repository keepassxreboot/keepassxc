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
#include "http/qhttp/qhttpclient.hpp"
#include "http/qhttp/qhttpclientresponse.hpp"

using namespace qhttp::client;
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
#ifdef WITH_XC_NETWORKING
    , m_fallbackToGoogle(true)
    , m_redirectCount(0)
#endif
#ifdef WITH_XC_HTTP
    , m_httpClient(nullptr)
#endif
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
        }
        else {
            Q_ASSERT(false);
        }
    }
    else {
        QModelIndex index = m_ui->customIconsView->currentIndex();
        if (index.isValid()) {
            iconStruct.uuid = m_customIconModel->uuidFromIndex(m_ui->customIconsView->currentIndex());
        }
        else {
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
    }
    else {
        QModelIndex index = m_customIconModel->indexFromUuid(iconUuid);
        if (index.isValid()) {
            m_ui->customIconsView->setCurrentIndex(index);
            m_ui->customIconsRadio->setChecked(true);
        }
        else {
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
    resetFaviconDownload();
#else
    Q_UNUSED(url);
    m_ui->faviconButton->setVisible(false);
#endif
}

void EditWidgetIcons::downloadFavicon()
{
#ifdef WITH_XC_NETWORKING
    QUrl url = QUrl(m_url);
    url.setPath("/favicon.ico");
    fetchFavicon(url);
#endif
}

#ifdef WITH_XC_NETWORKING
void EditWidgetIcons::fetchFavicon(const QUrl& url)
{
    if (nullptr == m_httpClient) {
        m_httpClient = new QHttpClient(this);
    }

    bool requestMade = m_httpClient->request(qhttp::EHTTP_GET, url, [this, url](QHttpResponse* response) {
        if (m_database == nullptr) {
            return;
        }

        response->collectData();
        response->onEnd([this, response, &url]() {
            int status = response->status();
            if (200 == status) {
                QImage image;
                image.loadFromData(response->collectedData());

                if (!image.isNull()) {
                    addCustomIcon(image);
                    resetFaviconDownload();
                } else {
                    fetchFaviconFromGoogle(url.host());
                }
            } else if (301 == status || 302 == status) {
                // Check if server has sent a redirect
                QUrl possibleRedirectUrl(response->headers().value("location", ""));
                if (!possibleRedirectUrl.isEmpty() && possibleRedirectUrl != m_redirectUrl && m_redirectCount < 3) {
                    resetFaviconDownload(false);
                    m_redirectUrl = possibleRedirectUrl;
                    ++m_redirectCount;
                    fetchFavicon(m_redirectUrl);
                } else {
                    // website is trying to redirect to itself or
                    // maximum number of redirects has been reached, fall back to Google
                    fetchFaviconFromGoogle(url.host());
                }
            } else {
                fetchFaviconFromGoogle(url.host());
            }
        });
    });

    if (!requestMade) {
        resetFaviconDownload();
        return;
    }

    m_httpClient->setConnectingTimeOut(5000, [this]() {
        QUrl tempurl = QUrl(m_url);
        if (tempurl.scheme() == "http") {
            resetFaviconDownload();
            emit messageEditEntry(tr("Unable to fetch favicon.") + "\n" +
                                  tr("Hint: You can enable Google as a fallback under Tools>Settings>Security"),
                                  MessageWidget::Error);
        } else {
            tempurl.setScheme("http");
            m_url = tempurl.url();
            tempurl.setPath("/favicon.ico");
            fetchFavicon(tempurl);
        }
    });

    m_ui->faviconButton->setDisabled(true);
}

void EditWidgetIcons::fetchFaviconFromGoogle(const QString& domain)
{
    if (config()->get("security/IconDownloadFallbackToGoogle", false).toBool() && m_fallbackToGoogle) {
        resetFaviconDownload();
        m_fallbackToGoogle = false;
        QUrl faviconUrl = QUrl("https://www.google.com/s2/favicons");
        faviconUrl.setQuery("domain=" + QUrl::toPercentEncoding(domain));
        fetchFavicon(faviconUrl);
    } else {
        resetFaviconDownload();
        emit messageEditEntry(tr("Unable to fetch favicon."), MessageWidget::Error);
    }
}

void EditWidgetIcons::resetFaviconDownload(bool clearRedirect)
{
    if (clearRedirect) {
        m_redirectUrl.clear();
        m_redirectCount = 0;
    }

    if (nullptr != m_httpClient) {
        m_httpClient->deleteLater();
        m_httpClient = nullptr;
    }

    m_fallbackToGoogle = true;
    m_ui->faviconButton->setDisabled(false);
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

void EditWidgetIcons::addCustomIcon(const QImage &icon)
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
        }
        else {
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
        }
        else {
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
