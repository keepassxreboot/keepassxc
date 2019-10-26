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

#include "ShareObserver.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/FileWatcher.h"
#include "core/Global.h"
#include "core/Group.h"
#include "keeshare/KeeShare.h"
#include "keeshare/ShareExport.h"
#include "keeshare/ShareImport.h"

namespace
{
    QString resolvePath(const QString& path, QSharedPointer<Database> database)
    {
        const QFileInfo info(database->filePath());
        return info.absoluteDir().absoluteFilePath(path);
    }
} // End Namespace

ShareObserver::ShareObserver(QSharedPointer<Database> db, QObject* parent)
    : QObject(parent)
    , m_db(std::move(db))
    , m_fileWatcher(new BulkFileWatcher(this))
{
    connect(KeeShare::instance(), SIGNAL(activeChanged()), SLOT(handleDatabaseChanged()));

    connect(m_db.data(), SIGNAL(groupDataChanged(Group*)), SLOT(handleDatabaseChanged()));
    connect(m_db.data(), SIGNAL(groupAdded()), SLOT(handleDatabaseChanged()));
    connect(m_db.data(), SIGNAL(groupRemoved()), SLOT(handleDatabaseChanged()));

    connect(m_db.data(), SIGNAL(databaseModified()), SLOT(handleDatabaseChanged()));
    connect(m_db.data(), SIGNAL(databaseSaved()), SLOT(handleDatabaseSaved()));

    connect(m_fileWatcher, SIGNAL(fileCreated(QString)), SLOT(handleFileCreated(QString)));
    connect(m_fileWatcher, SIGNAL(fileChanged(QString)), SLOT(handleFileUpdated(QString)));
    connect(m_fileWatcher, SIGNAL(fileRemoved(QString)), SLOT(handleFileDeleted(QString)));

    handleDatabaseChanged();
}

ShareObserver::~ShareObserver()
{
}

void ShareObserver::deinitialize()
{
    m_fileWatcher->clear();
    m_groupToReference.clear();
    m_referenceToGroup.clear();
}

void ShareObserver::reinitialize()
{
    struct Update
    {
        Group* group;
        KeeShareSettings::Reference oldReference;
        KeeShareSettings::Reference newReference;
    };

    QList<Update> updated;
    const QList<Group*> groups = m_db->rootGroup()->groupsRecursive(true);
    for (Group* group : groups) {
        const Update couple{group, m_groupToReference.value(group), KeeShare::referenceOf(group)};
        if (couple.oldReference == couple.newReference) {
            continue;
        }

        m_groupToReference.remove(couple.group);
        m_referenceToGroup.remove(couple.oldReference);
        const auto oldResolvedPath = resolvePath(couple.oldReference.path, m_db);
        m_shareToGroup.remove(oldResolvedPath);
        if (couple.newReference.isValid()) {
            m_groupToReference[couple.group] = couple.newReference;
            m_referenceToGroup[couple.newReference] = couple.group;
            const auto newResolvedPath = resolvePath(couple.newReference.path, m_db);
            m_shareToGroup[newResolvedPath] = couple.group;
        }
        updated << couple;
    }

    QStringList success;
    QStringList warning;
    QStringList error;
    QMap<QString, QStringList> imported;
    QMap<QString, QStringList> exported;
    for (const auto& update : asConst(updated)) {
        if (!update.oldReference.path.isEmpty()) {
            const auto oldResolvedPath = resolvePath(update.oldReference.path, m_db);
            m_fileWatcher->removePath(oldResolvedPath);
        }

        if (!update.newReference.path.isEmpty() && update.newReference.type != KeeShareSettings::Inactive) {
            const auto newResolvedPath = resolvePath(update.newReference.path, m_db);
            m_fileWatcher->addPath(newResolvedPath);
        }
        if (update.newReference.isExporting()) {
            exported[update.newReference.path] << update.group->name();
            // export is only on save
        }

        if (update.newReference.isImporting()) {
            imported[update.newReference.path] << update.group->name();
            // import has to occur immediately
            const auto result = this->importShare(update.newReference.path);
            if (!result.isValid()) {
                // tolerable result - blocked import or missing source
                continue;
            }

            if (result.isError()) {
                error << tr("Import from %1 failed (%2)").arg(result.path).arg(result.message);
            } else if (result.isWarning()) {
                warning << tr("Import from %1 failed (%2)").arg(result.path).arg(result.message);
            } else if (result.isInfo()) {
                success << tr("Import from %1 successful (%2)").arg(result.path).arg(result.message);
            } else {
                success << tr("Imported from %1").arg(result.path);
            }
        }
    }
    for (auto it = imported.cbegin(); it != imported.cend(); ++it) {
        if (it.value().count() > 1) {
            warning << tr("Multiple import source path to %1 in %2").arg(it.key(), it.value().join(", "));
        }
    }
    for (auto it = exported.cbegin(); it != exported.cend(); ++it) {
        if (it.value().count() > 1) {
            error << tr("Conflicting export target path %1 in %2").arg(it.key(), it.value().join(", "));
        }
    }

    notifyAbout(success, warning, error);
}

void ShareObserver::notifyAbout(const QStringList& success, const QStringList& warning, const QStringList& error)
{
    QStringList messages;
    MessageWidget::MessageType type = MessageWidget::Positive;
    if (!(success.isEmpty() || config()->get("KeeShare/QuietSuccess", true).toBool())) {
        messages += success;
    }
    if (!warning.isEmpty()) {
        type = MessageWidget::Warning;
        messages += warning;
    }
    if (!error.isEmpty()) {
        type = MessageWidget::Error;
        messages += error;
    }
    if (!messages.isEmpty()) {
        emit sharingMessage(messages.join("\n"), type);
    }
}

void ShareObserver::handleDatabaseChanged()
{
    if (!m_db) {
        Q_ASSERT(m_db);
        return;
    }
    const auto active = KeeShare::active();
    if (!active.out && !active.in) {
        deinitialize();
    } else {
        reinitialize();
    }
}

void ShareObserver::handleFileCreated(const QString& path)
{
    // there is currently no difference in handling an added share or updating from one
    this->handleFileUpdated(path);
}

void ShareObserver::handleFileDeleted(const QString& path)
{
    Q_UNUSED(path);
    // There is nothing we can or should do for now, ignore deletion
}

void ShareObserver::handleFileUpdated(const QString& path)
{
    const Result result = this->importShare(path);
    if (!result.isValid()) {
        return;
    }
    QStringList success;
    QStringList warning;
    QStringList error;
    if (result.isError()) {
        error << tr("Import from %1 failed (%2)").arg(result.path, result.message);
    } else if (result.isWarning()) {
        warning << tr("Import from %1 failed (%2)").arg(result.path, result.message);
    } else if (result.isInfo()) {
        success << tr("Import from %1 successful (%2)").arg(result.path, result.message);
    } else {
        success << tr("Imported from %1").arg(result.path);
    }
    notifyAbout(success, warning, error);
}

ShareObserver::Result ShareObserver::importShare(const QString& path)
{
    if (!KeeShare::active().in) {
        return {};
    }
    const auto changePath = resolvePath(path, m_db);
    auto shareGroup = m_shareToGroup.value(changePath);
    if (!shareGroup) {
        qWarning("Group for %s does not exist", qPrintable(path));
        return {};
    }
    const auto reference = KeeShare::referenceOf(shareGroup);
    if (reference.type == KeeShareSettings::Inactive) {
        // changes of inactive references are ignored
        return {};
    }
    if (reference.type == KeeShareSettings::ExportTo) {
        // changes of export only references are ignored
        return {};
    }

    Q_ASSERT(shareGroup->database() == m_db);
    Q_ASSERT(shareGroup == m_db->rootGroup()->findGroupByUuid(shareGroup->uuid()));
    const auto resolvedPath = resolvePath(reference.path, m_db);
    return ShareImport::containerInto(resolvedPath, reference, shareGroup);
}

QSharedPointer<Database> ShareObserver::database()
{
    return m_db;
}

QList<ShareObserver::Result> ShareObserver::exportShares()
{
    QList<Result> results;
    struct Reference
    {
        KeeShareSettings::Reference config;
        const Group* group;
    };

    QMap<QString, QList<Reference>> references;
    const auto groups = m_db->rootGroup()->groupsRecursive(true);
    for (const auto* group : groups) {
        const auto reference = KeeShare::referenceOf(group);
        if (!reference.isExporting()) {
            continue;
        }
        references[reference.path] << Reference{reference, group};
    }

    for (auto it = references.cbegin(); it != references.cend(); ++it) {
        if (it.value().count() != 1) {
            const auto path = it.value().first().config.path;
            QStringList groupnames;
            for (const auto& reference : it.value()) {
                groupnames << reference.group->name();
            }
            results << Result{
                path, Result::Error, tr("Conflicting export target path %1 in %2").arg(path, groupnames.join(", "))};
        }
    }
    if (!results.isEmpty()) {
        // We need to block export due to config
        return results;
    }

    for (auto it = references.cbegin(); it != references.cend(); ++it) {
        const auto& reference = it.value().first();
        const QString resolvedPath = resolvePath(reference.config.path, m_db);
        m_fileWatcher->ignoreFileChanges(resolvedPath);
        results << ShareExport::intoContainer(resolvedPath, reference.config, reference.group);
        m_fileWatcher->observeFileChanges(true);
    }
    return results;
}

void ShareObserver::handleDatabaseSaved()
{
    if (!KeeShare::active().out) {
        return;
    }
    QStringList error;
    QStringList warning;
    QStringList success;

    const auto results = exportShares();
    for (const Result& result : results) {
        if (!result.isValid()) {
            Q_ASSERT(result.isValid());
            continue;
        }
        if (result.isError()) {
            error << tr("Export to %1 failed (%2)").arg(result.path).arg(result.message);
        } else if (result.isWarning()) {
            warning << tr("Export to %1 failed (%2)").arg(result.path).arg(result.message);
        } else if (result.isInfo()) {
            success << tr("Export to %1 successful (%2)").arg(result.path).arg(result.message);
        } else {
            success << tr("Export to %1").arg(result.path);
        }
    }
    notifyAbout(success, warning, error);
}

ShareObserver::Result::Result(const QString& path, ShareObserver::Result::Type type, const QString& message)
    : path(path)
    , type(type)
    , message(message)
{
}

bool ShareObserver::Result::isValid() const
{
    return !path.isEmpty() || !message.isEmpty();
}

bool ShareObserver::Result::isError() const
{
    return !message.isEmpty() && type == Error;
}

bool ShareObserver::Result::isInfo() const
{
    return !message.isEmpty() && type == Info;
}

bool ShareObserver::Result::isWarning() const
{
    return !message.isEmpty() && type == Warning;
}
