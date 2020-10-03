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

#include "core/Config.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/IconModels.h"
#include "gui/MessageBox.h"
#ifdef WITH_XC_NETWORKING
#include "gui/IconDownloader.h"
#endif

IconStruct::IconStruct()
    : uuid(QUuid())
    , number(0)
    , applyTo(ApplyIconToOptions::THIS_ONLY)
{
}

EditWidgetIcons::EditWidgetIcons(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::EditWidgetIcons())
    , m_db(nullptr)
    , m_applyIconTo(ApplyIconToOptions::THIS_ONLY)
    , m_defaultIconModel(new DefaultIconModel(this))
    , m_customIconModel(new CustomIconModel(this))
#ifdef WITH_XC_NETWORKING
    , m_downloader(new IconDownloader())
#endif
{
    m_ui->setupUi(this);

    m_ui->defaultIconsView->setModel(m_defaultIconModel);
    m_ui->customIconsView->setModel(m_customIconModel);

    m_ui->applyIconToPushButton->setMenu(createApplyIconToMenu());

    // clang-format off
    connect(m_ui->defaultIconsView, SIGNAL(clicked(QModelIndex)), this, SLOT(updateRadioButtonDefaultIcons()));
    connect(m_ui->customIconsView, SIGNAL(clicked(QModelIndex)), this, SLOT(updateRadioButtonCustomIcons()));
    connect(m_ui->defaultIconsRadio, SIGNAL(toggled(bool)), this, SLOT(updateWidgetsDefaultIcons(bool)));
    connect(m_ui->customIconsRadio, SIGNAL(toggled(bool)), this, SLOT(updateWidgetsCustomIcons(bool)));
    connect(m_ui->addButton, SIGNAL(clicked()), SLOT(addCustomIconFromFile()));
    connect(m_ui->deleteButton, SIGNAL(clicked()), SLOT(removeCustomIcon()));
    connect(m_ui->faviconButton, SIGNAL(clicked()), SLOT(downloadFavicon()));
    connect(m_ui->applyIconToPushButton->menu(), SIGNAL(triggered(QAction*)), SLOT(confirmApplyIconTo(QAction*)));

    connect(m_ui->defaultIconsRadio, SIGNAL(toggled(bool)), this, SIGNAL(widgetUpdated()));
    connect(m_ui->defaultIconsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SIGNAL(widgetUpdated()));
    connect(m_ui->customIconsView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SIGNAL(widgetUpdated()));
#ifdef WITH_XC_NETWORKING
    connect(m_downloader.data(),
            SIGNAL(finished(const QString&, const QImage&)),
            SLOT(iconReceived(const QString&, const QImage&)));
#endif
    // clang-format on

    m_ui->faviconButton->setVisible(false);
    m_ui->addButton->setEnabled(true);
}

EditWidgetIcons::~EditWidgetIcons()
{
}

IconStruct EditWidgetIcons::state()
{
    Q_ASSERT(m_db);
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

    iconStruct.applyTo = m_applyIconTo;

    return iconStruct;
}

void EditWidgetIcons::reset()
{
    m_db.reset();
    m_currentUuid = QUuid();
}

void EditWidgetIcons::load(const QUuid& currentUuid,
                           const QSharedPointer<Database>& database,
                           const IconStruct& iconStruct,
                           const QString& url)
{
    Q_ASSERT(database);
    Q_ASSERT(!currentUuid.isNull());

    m_db = database;
    m_currentUuid = currentUuid;
    setUrl(url);

    m_customIconModel->setIcons(database->metadata()->customIconsPixmaps(IconSize::Default),
                                database->metadata()->customIconsOrder());

    QUuid iconUuid = iconStruct.uuid;
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

    m_applyIconTo = ApplyIconToOptions::THIS_ONLY;
    m_ui->applyIconToPushButton->menu()->defaultAction()->activate(QAction::Trigger);
}

void EditWidgetIcons::setShowApplyIconToButton(bool state)
{
    m_ui->applyIconToPushButton->setVisible(state);
}

QMenu* EditWidgetIcons::createApplyIconToMenu()
{
    auto* applyIconToMenu = new QMenu(this);
    QAction* defaultAction = applyIconToMenu->addAction(tr("Apply to this group only"));
    defaultAction->setData(QVariant::fromValue(ApplyIconToOptions::THIS_ONLY));
    applyIconToMenu->setDefaultAction(defaultAction);
    applyIconToMenu->addSeparator();
    applyIconToMenu->addAction(tr("Also apply to child groups"))
        ->setData(QVariant::fromValue(ApplyIconToOptions::CHILD_GROUPS));
    applyIconToMenu->addAction(tr("Also apply to child entries"))
        ->setData(QVariant::fromValue(ApplyIconToOptions::CHILD_ENTRIES));
    applyIconToMenu->addAction(tr("Also apply to all children"))
        ->setData(QVariant::fromValue(ApplyIconToOptions::ALL_CHILDREN));
    return applyIconToMenu;
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
    if (!m_url.isEmpty()) {
        m_downloader->setUrl(m_url);
        m_downloader->download();
    }
#endif
}

void EditWidgetIcons::iconReceived(const QString& url, const QImage& icon)
{
#ifdef WITH_XC_NETWORKING
    Q_UNUSED(url);
    if (icon.isNull()) {
        QString message(tr("Unable to fetch favicon."));
        if (!config()->get(Config::Security_IconDownloadFallback).toBool()) {
            message.append("\n").append(
                tr("You can enable the DuckDuckGo website icon service under Tools -> Settings -> Security"));
        }
        emit messageEditEntry(message, MessageWidget::Error);
        return;
    }

    if (!addCustomIcon(icon)) {
        emit messageEditEntry(tr("Existing icon selected."), MessageWidget::Information);
    }
#else
    Q_UNUSED(url);
    Q_UNUSED(icon);
#endif
}

void EditWidgetIcons::abortRequests()
{
#ifdef WITH_XC_NETWORKING
    if (m_downloader) {
        m_downloader->abortDownload();
    }
#endif
}

void EditWidgetIcons::addCustomIconFromFile()
{
    if (!m_db) {
        return;
    }

    QString filter = QString("%1 (%2);;%3 (*)").arg(tr("Images"), Tools::imageReaderFilter(), tr("All files"));

    auto filenames = QFileDialog::getOpenFileNames(this, tr("Select Image(s)"), "", filter);
    if (!filenames.empty()) {
        QStringList errornames;
        int numexisting = 0;
        for (const auto& filename : filenames) {
            if (!filename.isEmpty()) {
                auto icon = QImage(filename);
                if (icon.isNull()) {
                    errornames << filename;
                } else if (!addCustomIcon(icon)) {
                    // Icon already exists in database
                    ++numexisting;
                }
            }
        }

        int numloaded = filenames.size() - errornames.size() - numexisting;
        QString msg;

        if (numloaded > 0) {
            msg = tr("Successfully loaded %1 of %n icon(s)", "", filenames.size()).arg(numloaded);
        } else {
            msg = tr("No icons were loaded");
        }

        if (numexisting > 0) {
            msg += "\n" + tr("%n icon(s) already exist in the database", "", numexisting);
        }

        if (!errornames.empty()) {
            // Show the first 8 icons that failed to load
            errornames = errornames.mid(0, 8);
            emit messageEditEntry(msg + "\n" + tr("The following icon(s) failed:", "", errornames.size()) + "\n"
                                      + errornames.join("\n"),
                                  MessageWidget::Error);
        } else if (numloaded > 0) {
            emit messageEditEntry(msg, MessageWidget::Positive);
        } else {
            emit messageEditEntry(msg, MessageWidget::Information);
        }
    }
}

bool EditWidgetIcons::addCustomIcon(const QImage& icon)
{
    bool added = false;
    if (m_db) {
        // Don't add an icon larger than 128x128, but retain original size if smaller
        auto scaledicon = icon;
        if (icon.width() > 128 || icon.height() > 128) {
            scaledicon = icon.scaled(128, 128);
        }

        QUuid uuid = m_db->metadata()->findCustomIcon(scaledicon);
        if (uuid.isNull()) {
            uuid = QUuid::createUuid();
            m_db->metadata()->addCustomIcon(uuid, scaledicon);
            m_customIconModel->setIcons(m_db->metadata()->customIconsPixmaps(IconSize::Default),
                                        m_db->metadata()->customIconsOrder());
            added = true;
        }

        // Select the new or existing icon
        updateRadioButtonCustomIcons();
        QModelIndex index = m_customIconModel->indexFromUuid(uuid);
        m_ui->customIconsView->setCurrentIndex(index);

        emit widgetUpdated();
    }

    return added;
}

void EditWidgetIcons::removeCustomIcon()
{
    if (m_db) {
        QModelIndex index = m_ui->customIconsView->currentIndex();
        if (index.isValid()) {
            QUuid iconUuid = m_customIconModel->uuidFromIndex(index);

            const QList<Entry*> allEntries = m_db->rootGroup()->entriesRecursive(true);
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

            const QList<Group*> allGroups = m_db->rootGroup()->groupsRecursive(true);
            QList<Group*> groupsWithSameIcon;

            for (Group* group : allGroups) {
                if (iconUuid == group->iconUuid() && m_currentUuid != group->uuid()) {
                    groupsWithSameIcon << group;
                }
            }

            int iconUseCount = entriesWithSameIcon.size() + groupsWithSameIcon.size();
            if (iconUseCount > 0) {

                auto result = MessageBox::question(this,
                                                   tr("Confirm Delete"),
                                                   tr("This icon is used by %n entry(s), and will be replaced "
                                                      "by the default icon. Are you sure you want to delete it?",
                                                      "",
                                                      iconUseCount),
                                                   MessageBox::Delete | MessageBox::Cancel,
                                                   MessageBox::Cancel);

                if (result == MessageBox::Cancel) {
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
            m_db->metadata()->removeCustomIcon(iconUuid);
            m_customIconModel->setIcons(m_db->metadata()->customIconsPixmaps(IconSize::Default),
                                        m_db->metadata()->customIconsOrder());

            // Reset the current icon view
            updateRadioButtonDefaultIcons();

            if (m_db->rootGroup()->findEntryByUuid(m_currentUuid) != nullptr) {
                m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(Entry::DefaultIconNumber));
            } else {
                m_ui->defaultIconsView->setCurrentIndex(m_defaultIconModel->index(Group::DefaultIconNumber));
            }

            emit widgetUpdated();
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

void EditWidgetIcons::confirmApplyIconTo(QAction* action)
{
    m_applyIconTo = action->data().value<ApplyIconToOptions>();
    m_ui->applyIconToPushButton->setText(action->text());
}
