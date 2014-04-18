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
    , m_database(Q_NULLPTR)
    , m_defaultIconModel(new DefaultIconModel(this))
    , m_customIconModel(new CustomIconModel(this))
    , m_networkAccessMngr(new QNetworkAccessManager(this))
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

IconStruct EditWidgetIcons::save()
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

    m_database = Q_NULLPTR;
    m_currentUuid = Uuid();
    return iconStruct;
}

void EditWidgetIcons::load(Uuid currentUuid, Database* database, IconStruct iconStruct, const QString &url)
{
    Q_ASSERT(database);
    Q_ASSERT(!currentUuid.isNull());

    m_database = database;
    m_currentUuid = currentUuid;
    setUrl(url);

    m_customIconModel->setIcons(database->metadata()->customIcons(),
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

static QStringList getHost(QUrl url, const QString& path)
{
    QStringList hosts;

    QString host = url.host();
    for(;;) {
        QString s = host;
        if (url.port() >= 0)
            s += ":" + QString::number(url.port());
        hosts << s + path;

        const int first_dot = host.indexOf( '.' );
        const int last_dot = host.lastIndexOf( '.' );
        if( ( first_dot != -1 ) && ( last_dot != -1 ) && ( first_dot != last_dot ) )
            host.remove( 0, first_dot + 1 );
        else
            break;
    }
    return hosts;
}

void EditWidgetIcons::downloadFavicon()
{
    const QStringList pathes = getHost(QUrl(m_url), "/favicon.");
    const QStringList prefixes = QStringList() << "http://" << "https://";
    const QStringList suffixes = QStringList() << "ico" << "png" << "gif" << "jpg";

    Q_FOREACH (QString path, pathes)
        Q_FOREACH (QString prefix, prefixes)
            Q_FOREACH (QString suffix, suffixes)
                m_networkOperations << m_networkAccessMngr->get(QNetworkRequest(prefix + path + suffix));
    //TODO: progress indication
}

void EditWidgetIcons::abortFaviconDownload()
{
    Q_FOREACH (QNetworkReply *r, m_networkOperations)
        r->abort();
}

void EditWidgetIcons::onRequestFinished(QNetworkReply *reply)
{
    if (m_networkOperations.contains(reply)) {
        m_networkOperations.remove(reply);

        QImage image;
        if (!reply->error() && image.loadFromData(reply->readAll()) && !image.isNull()) {
            //Abort all other requests
            abortFaviconDownload();

            //Set the image
            Uuid uuid = Uuid::random();
            m_database->metadata()->addCustomIcon(uuid, image.scaled(16, 16));
            m_customIconModel->setIcons(m_database->metadata()->customIcons(),
                                        m_database->metadata()->customIconsOrder());
            QModelIndex index = m_customIconModel->indexFromUuid(uuid);
            m_ui->customIconsView->setCurrentIndex(index);
            m_ui->customIconsRadio->setChecked(true);
        }
    }
    reply->deleteLater();
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
                m_customIconModel->setIcons(m_database->metadata()->customIcons(),
                                            m_database->metadata()->customIconsOrder());
                QModelIndex index = m_customIconModel->indexFromUuid(uuid);
                m_ui->customIconsView->setCurrentIndex(index);
            }
            else {
                // TODO: show error
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

            QList<Entry*> allEntries = m_database->rootGroup()->entriesRecursive(true);
            QList<Entry*> historyEntriesWithSameIcon;

            Q_FOREACH (Entry* entry, allEntries) {
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

            QList<const Group*> allGroups = m_database->rootGroup()->groupsRecursive(true);
            Q_FOREACH (const Group* group, allGroups) {
                if (iconUuid == group->iconUuid() && m_currentUuid != group->uuid()) {
                    iconUsedCount++;
                }
            }

            if (iconUsedCount == 0) {
                Q_FOREACH (Entry* entry, historyEntriesWithSameIcon) {
                    entry->setUpdateTimeinfo(false);
                    entry->setIcon(0);
                    entry->setUpdateTimeinfo(true);
                }

                m_database->metadata()->removeCustomIcon(iconUuid);
                m_customIconModel->setIcons(m_database->metadata()->customIcons(),
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
