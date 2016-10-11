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

#include "EditWidgetIcons.h"
#include "ui_EditWidgetIcons.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QFileDialog>

#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/IconModels.h"
#include "gui/MessageBox.h"

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
    , m_networkAccessMngr(new QNetworkAccessManager(this))
    , m_networkOperation(nullptr)
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
    connect(m_ui->addButton, SIGNAL(clicked()), SLOT(addCustomIcon()));
    connect(m_ui->deleteButton, SIGNAL(clicked()), SLOT(removeCustomIcon()));
    connect(m_ui->faviconButton, SIGNAL(clicked()), SLOT(downloadFavicon()));
    connect(m_networkAccessMngr, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(onRequestFinished(QNetworkReply*)) );

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

void EditWidgetIcons::load(Uuid currentUuid, Database* database, IconStruct iconStruct, const QString &url)
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

void EditWidgetIcons::setUrl(const QString &url)
{
    m_url = url;
    m_ui->faviconButton->setVisible(!url.isEmpty());
    abortFaviconDownload();
}

void EditWidgetIcons::downloadFavicon()
{
    QUrl url = QUrl(m_url);
    url.setPath("/favicon.ico");
    fetchFavicon(url);
}

void EditWidgetIcons::fetchFavicon(QUrl url)
{
    if (m_networkOperation == nullptr) {
        m_networkOperation = m_networkAccessMngr->get(QNetworkRequest(url));
        m_ui->faviconButton->setDisabled(true);
    }
}

void EditWidgetIcons::fetchFaviconFromGoogle(QString domain)
{
     if (m_fallbackToGoogle) {
        abortFaviconDownload();
        m_fallbackToGoogle = false;
        fetchFavicon(QUrl("http://www.google.com/s2/favicons?domain=" + domain));
    }
    else {
        abortFaviconDownload();
        MessageBox::warning(this, tr("Error"), tr("Unable to fetch favicon."));
    }
}

void EditWidgetIcons::abortFaviconDownload(bool clearRedirect)
{
    if (m_networkOperation != nullptr) {
        m_networkOperation->abort();
        m_networkOperation->deleteLater();
        m_networkOperation = nullptr;
    }
    
    if (clearRedirect) {
        if (!m_redirectUrl.isEmpty()) {
            m_redirectUrl.clear();
        }
        m_redirectCount = 0;
    }
    
    m_fallbackToGoogle = true;
    m_ui->faviconButton->setDisabled(false);
}

void EditWidgetIcons::onRequestFinished(QNetworkReply *reply)
{
    if (!reply->error()) {    
        QImage image;
        image.loadFromData(reply->readAll());

        if (!image.isNull()) {
            //Set the image
            Uuid uuid = Uuid::random();
            m_database->metadata()->addCustomIcon(uuid, image.scaled(16, 16));
            m_customIconModel->setIcons(m_database->metadata()->customIconsScaledPixmaps(),
                                        m_database->metadata()->customIconsOrder());
            QModelIndex index = m_customIconModel->indexFromUuid(uuid);
            m_ui->customIconsView->setCurrentIndex(index);
            m_ui->customIconsRadio->setChecked(true);
            
            abortFaviconDownload();
        }
        else {
            // Check if server has sent a redirect
            QUrl possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (!possibleRedirectUrl.isEmpty() && possibleRedirectUrl != m_redirectUrl && m_redirectCount < 3) {
                abortFaviconDownload(false);
                m_redirectUrl = possibleRedirectUrl;
                ++m_redirectCount;
                fetchFavicon(m_redirectUrl);
            }
            else { // Webpage is trying to redirect back to itself or the maximum number of redirects has been reached, fallback to Google
                fetchFaviconFromGoogle(reply->url().host());
            }
        }
    }
    else { // Request Error e.g. 404, fallback to Google
        fetchFaviconFromGoogle(reply->url().host());
    }
}

void EditWidgetIcons::addCustomIcon()
{
    if (m_database) {
        QString filter = QString("%1 (%2);;%3 (*)").arg(tr("Images"),
                    Tools::imageReaderFilter(), tr("All files"));

        QString filename = QFileDialog::getOpenFileName(
                    this, tr("Select Image"), "", filter);
        if (!filename.isEmpty()) {
            QImage image(filename);
            if (!image.isNull()) {
                Uuid uuid = Uuid::random();
                m_database->metadata()->addCustomIcon(uuid, image.scaled(16, 16));
                m_customIconModel->setIcons(m_database->metadata()->customIconsScaledPixmaps(),
                                            m_database->metadata()->customIconsOrder());
                QModelIndex index = m_customIconModel->indexFromUuid(uuid);
                m_ui->customIconsView->setCurrentIndex(index);
            }
            else {
                MessageBox::critical(this, tr("Error"), tr("Can't read icon"));
            }
        }
    }
}

void EditWidgetIcons::removeCustomIcon()
{
    if (m_database) {
        QModelIndex index = m_ui->customIconsView->currentIndex();
        if (index.isValid()) {
            Uuid iconUuid = m_customIconModel->uuidFromIndex(index);
            int iconUsedCount = 0;

            const QList<Entry*> allEntries = m_database->rootGroup()->entriesRecursive(true);
            QList<Entry*> historyEntriesWithSameIcon;

            for (Entry* entry : allEntries) {
                bool isHistoryEntry = !entry->group();
                if (iconUuid == entry->iconUuid()) {
                    if (isHistoryEntry) {
                        historyEntriesWithSameIcon << entry;
                    }
                    else if (m_currentUuid != entry->uuid()) {
                        iconUsedCount++;
                    }
                }
            }

            const QList<Group*> allGroups = m_database->rootGroup()->groupsRecursive(true);
            for (const Group* group : allGroups) {
                if (iconUuid == group->iconUuid() && m_currentUuid != group->uuid()) {
                    iconUsedCount++;
                }
            }

            if (iconUsedCount == 0) {
                for (Entry* entry : asConst(historyEntriesWithSameIcon)) {
                    entry->setUpdateTimeinfo(false);
                    entry->setIcon(0);
                    entry->setUpdateTimeinfo(true);
                }

                m_database->metadata()->removeCustomIcon(iconUuid);
                m_customIconModel->setIcons(m_database->metadata()->customIconsScaledPixmaps(),
                                            m_database->metadata()->customIconsOrder());
                if (m_customIconModel->rowCount() > 0) {
                    m_ui->customIconsView->setCurrentIndex(m_customIconModel->index(0, 0));
                }
                else {
                    updateRadioButtonDefaultIcons();
                }
            }
            else {
                MessageBox::information(this, tr("Can't delete icon!"),
                                        tr("Can't delete icon. Still used by %1 items.")
                                        .arg(iconUsedCount));
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
