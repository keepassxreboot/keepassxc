/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include "SSHAdd.h"

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"
#include "sshagent/KeeAgentSettings.h"
#include "sshagent/SSHAgent.h"

#include <QCommandLineParser>
#include <QLocale>

const QCommandLineOption SSHAdd::ListOption =
    QCommandLineOption(QStringList() << "l", QObject::tr("List the SSH keys in the database."));
const QCommandLineOption SSHAdd::DeleteOption =
    QCommandLineOption(QStringList() << "d", QObject::tr("Remove the SSH keys from the agent instead of adding them."));

SSHAdd::SSHAdd()
{
    name = QString("ssh-add");
    description = QObject::tr("Add ssh keys from the database to the running SSH agent.");
    options.append(SSHAdd::ListOption);
    optionalArguments.append(
        {QString("entry"), QObject::tr("Name of the entry with an SSH key to unlock."), QString("")});
}

int SSHAdd::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();
    bool listOption = parser->isSet(SSHAdd::ListOption);

    // If no entries are specified as arguments, we assume that we should operate on all
    // the available SSH keys.
    if (args.size() == 1) {
        for (Entry* e : database->rootGroup()->entriesRecursive()) {
            QSharedPointer<OpenSSHKey> key = SSHAgent::instance()->getKeyFromEntry(database, e);
            if (key.isNull()) {
                continue;
            }

            KeeAgentSettings settings;
            if (!settings.fromEntry(e)) {
                err << QObject::tr("Error while getting the SSH settings for entry %1.").arg(e->title()) << endl;
                continue;
            }

            if (listOption) {
                out << QObject::tr("%1 %2.").arg(e->title(), key->fingerprint()) << endl;
                continue;
            }

            // TODO check delete option.

            if (SSHAgent::instance()->addIdentity(*key, settings, database->uuid())) {
                out << QObject::tr("Successfully added SSH key from entry %1 to the SSH agent.").arg(e->title())
                    << endl;
            } else {
                err << QObject::tr("Could not add SSH key from entry %1 to the SSH agent.").arg(e->title()) << endl;
            }
        }

        return EXIT_SUCCESS;
    }

    const QString& entryPath = args.at(1);

    Entry* entry = database->rootGroup()->findEntryByPath(entryPath);
    if (!entry) {
        err << QObject::tr("Could not find entry with path %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    QSharedPointer<OpenSSHKey> key = SSHAgent::instance()->getKeyFromEntry(database, entry);
    if (key.isNull()) {
        err << QObject::tr("Could not get SSH key from entry %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }

    KeeAgentSettings settings;
    if (!settings.fromEntry(entry)) {
        err << QObject::tr("Error while getting the SSH settings for entry %1.").arg(entryPath) << endl;
        return EXIT_FAILURE;
    }
    // @hifi: Should I check for the KeeAgent settings from the CLI?
    // if (!KeeAgentSettings::inEntryAttachments(currentEntry->attachments())) {
    // return EXIT_FAILURE;
    // }

    if (SSHAgent::instance()->addIdentity(*key, settings, database->uuid())) {
        out << QObject::tr("Successfully added SSH key from entry %1 to the SSH agent.").arg(entryPath) << endl;
    } else {
        err << QObject::tr("Could not add SSH key from entry %1 to the SSH agent.").arg(entryPath) << endl;
    }
    return EXIT_SUCCESS;
}
