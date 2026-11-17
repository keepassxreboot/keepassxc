/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "MoveDir.h"

#include "Utils.h"
#include "core/CustomData.h"
#include "core/Global.h"
#include "core/Group.h"
#include "core/Metadata.h"

#include <QCommandLineParser>

namespace
{
    QString normalizedPath(const QString& path)
    {
        return path.split('/', Qt::SkipEmptyParts).join('/');
    }

    QList<Group*> groupsForPath(Group* rootGroup, const QString& path)
    {
        if (path.isEmpty() || path.count('/') == path.size()) {
            return {rootGroup};
        }

        QString exactQueryPath = path;
        if (!exactQueryPath.startsWith('/')) {
            exactQueryPath.prepend('/');
        }
        if (!exactQueryPath.endsWith('/')) {
            exactQueryPath.append('/');
        }

        const QString normalizedQueryPath = "/" + normalizedPath(path) + "/";
        QList<Group*> exactMatches;
        QList<Group*> normalizedMatches;

        for (Group* group : rootGroup->groupsRecursive(true)) {
            QStringList pathParts;
            for (Group* current = group; current && current != rootGroup; current = current->parentGroup()) {
                pathParts.prepend(current->name());
            }

            QString storedPath = "/";
            for (const QString& pathPart : pathParts) {
                storedPath += pathPart + "/";
            }

            if (storedPath == exactQueryPath) {
                exactMatches.append(group);
            } else if (storedPath == normalizedQueryPath) {
                normalizedMatches.append(group);
            }
        }

        // Match Group::findGroupByPath() spelling first. Collapsed query aliases
        // are accepted only when no exact stored path has that spelling.
        return exactMatches.isEmpty() ? normalizedMatches : exactMatches;
    }
} // namespace

MoveDir::MoveDir()
{
    name = QString("mvdir");
    description = QObject::tr("Moves and renames a group.");
    positionalArguments.append({QString("group"), QObject::tr("Path of the group to move."), QString("")});
    positionalArguments.append({QString("destination"), QObject::tr("New path for the group."), QString("")});
}

MoveDir::~MoveDir() = default;

int MoveDir::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = parser->isSet(Command::QuietOption) ? Utils::DEVNULL : Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    const QString& sourcePath = args.at(1);
    const QString& destinationPath = args.at(2);
    Group* rootGroup = database->rootGroup();

    const QList<Group*> sourceGroups = groupsForPath(rootGroup, sourcePath);
    if (sourceGroups.isEmpty()) {
        err << QObject::tr("Could not find group with path %1.").arg(sourcePath) << Qt::endl;
        return EXIT_FAILURE;
    }
    if (sourceGroups.size() > 1) {
        err << QObject::tr("Group path %1 is ambiguous.").arg(sourcePath) << Qt::endl;
        return EXIT_FAILURE;
    }
    Group* sourceGroup = sourceGroups.first();
    if (sourceGroup == rootGroup) {
        err << QObject::tr("Cannot move root group from database.") << Qt::endl;
        return EXIT_FAILURE;
    }

    const QStringList destinationParts = destinationPath.split('/', Qt::SkipEmptyParts);
    if (destinationParts.isEmpty()) {
        err << QObject::tr("Destination path %1 is invalid.").arg(destinationPath) << Qt::endl;
        return EXIT_FAILURE;
    }

    const QString groupName = destinationParts.last();
    QString destinationWithoutTrailingSeparators = destinationPath;
    while (destinationWithoutTrailingSeparators.endsWith('/')) {
        destinationWithoutTrailingSeparators.chop(1);
    }
    const int finalSeparator = destinationWithoutTrailingSeparators.lastIndexOf('/');
    const QString parentPath =
        finalSeparator < 0 ? QString() : destinationWithoutTrailingSeparators.left(finalSeparator);
    const QString displayedParentPath = normalizedPath(parentPath);

    if (!groupsForPath(rootGroup, destinationPath).isEmpty()) {
        err << QObject::tr("Destination group %1 already exists.").arg(destinationPath) << Qt::endl;
        return EXIT_FAILURE;
    }

    const QList<Group*> destinationParents = groupsForPath(rootGroup, parentPath);
    if (destinationParents.isEmpty()) {
        err << QObject::tr("Destination parent group %1 not found.").arg(displayedParentPath) << Qt::endl;
        return EXIT_FAILURE;
    }
    if (destinationParents.size() > 1) {
        err << QObject::tr("Destination parent group %1 is ambiguous.").arg(displayedParentPath) << Qt::endl;
        return EXIT_FAILURE;
    }
    Group* destinationParent = destinationParents.first();

    const QStringList sourceNameParts = sourceGroup->name().split('/', Qt::SkipEmptyParts);
    bool destinationLooksLikePreservedSourceName = sourceGroup->name().contains('/') && !sourceNameParts.isEmpty()
                                                   && destinationParts.size() >= sourceNameParts.size();
    for (qsizetype i = 0; destinationLooksLikePreservedSourceName && i < sourceNameParts.size(); ++i) {
        destinationLooksLikePreservedSourceName =
            destinationParts.at(destinationParts.size() - sourceNameParts.size() + i) == sourceNameParts.at(i);
    }
    if (destinationLooksLikePreservedSourceName) {
        err << QObject::tr(
            "Destination path cannot preserve a group name containing '/'; choose a different final name.")
            << Qt::endl;
        return EXIT_FAILURE;
    }

    for (const Group* child : destinationParent->children()) {
        if (child->name() == groupName) {
            err << QObject::tr("Destination group %1 already exists.").arg(destinationPath) << Qt::endl;
            return EXIT_FAILURE;
        }
    }

    for (Group* parent = destinationParent; parent; parent = parent->parentGroup()) {
        if (parent == sourceGroup) {
            err << QObject::tr("Cannot move a group into itself or one of its descendants.") << Qt::endl;
            return EXIT_FAILURE;
        }
    }

    Group* originalParent = sourceGroup->parentGroup();
    const int originalIndex = originalParent->children().indexOf(sourceGroup);
    const QString originalName = sourceGroup->name();
    const TimeInfo originalTimeInfo = sourceGroup->timeInfo();
    const QUuid originalPreviousParentUuid = sourceGroup->previousParentGroupUuid();
    const bool databaseWasModified = database->isModified();
    CustomData originalMetadataCustomData;
    originalMetadataCustomData.copyDataFrom(database->metadata()->customData());

    sourceGroup->setName(groupName);
    if (originalParent != destinationParent) {
        sourceGroup->setParent(destinationParent);
    }

    QString errorMessage;
    if (!database->save(Database::Atomic, {}, &errorMessage)) {
        sourceGroup->setName(originalName);
        if (originalParent != destinationParent) {
            sourceGroup->setParent(originalParent, originalIndex, false);
        }
        sourceGroup->setPreviousParentGroupUuid(originalPreviousParentUuid);
        sourceGroup->setTimeInfo(originalTimeInfo);
        database->metadata()->customData()->copyDataFrom(&originalMetadataCustomData);
        if (databaseWasModified) {
            database->markAsModified();
        } else {
            database->markAsClean();
        }
        err << QObject::tr("Writing the database failed %1.").arg(errorMessage) << Qt::endl;
        return EXIT_FAILURE;
    }

    out << QObject::tr("Successfully moved group %1 to %2.").arg(sourcePath, destinationPath) << Qt::endl;
    return EXIT_SUCCESS;
}
