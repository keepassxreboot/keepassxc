/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PasskeyImporter.h"
#include "PasskeyImportDialog.h"
#include "browser/BrowserMessageBuilder.h"
#include "browser/BrowserPasskeys.h"
#include "browser/BrowserService.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "gui/FileDialog.h"
#include "gui/MessageBox.h"
#include <QFileInfo>
#include <QUuid>

static const QString IMPORTED_PASSKEYS_GROUP = QStringLiteral("Imported Passkeys");

void PasskeyImporter::importPasskey(QSharedPointer<Database>& database)
{
    auto filter = QString("%1 (*.passkey);;%2 (*)").arg(tr("Passkey file"), tr("All files"));
    auto fileName =
        fileDialog()->getOpenFileName(nullptr, tr("Open Passkey file"), FileDialog::getLastDir("passkey"), filter);
    if (fileName.isEmpty()) {
        return;
    }

    FileDialog::saveLastDir("passkey", fileName, true);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        MessageBox::information(
            nullptr, tr("Cannot open file"), tr("Cannot open file \"%1\" for reading.").arg(fileName));
        return;
    }

    importSelectedFile(file, database);
}

void PasskeyImporter::importSelectedFile(QFile& file, QSharedPointer<Database>& database)
{
    const auto fileData = file.readAll();
    const auto passkeyObject = browserMessageBuilder()->getJsonObject(fileData);
    if (passkeyObject.isEmpty()) {
        MessageBox::information(nullptr,
                                tr("Cannot import Passkey"),
                                tr("Cannot import Passkey file \"%1\". Data is missing.").arg(file.fileName()));
        return;
    }

    const auto relyingParty = passkeyObject["relyingParty"].toString();
    const auto url = passkeyObject["url"].toString();
    const auto username = passkeyObject["username"].toString();
    const auto credentialId = passkeyObject["credentialId"].toString();
    const auto userHandle = passkeyObject["userHandle"].toString();
    const auto privateKey = passkeyObject["privateKey"].toString();

    if (relyingParty.isEmpty() || username.isEmpty() || credentialId.isEmpty() || userHandle.isEmpty()
        || privateKey.isEmpty()) {
        MessageBox::information(nullptr,
                                tr("Cannot import Passkey"),
                                tr("Cannot import Passkey file \"%1\". Data is missing.").arg(file.fileName()));
    } else if (!privateKey.startsWith("-----BEGIN PRIVATE KEY-----")
               || !privateKey.trimmed().endsWith("-----END PRIVATE KEY-----")) {
        MessageBox::information(
            nullptr,
            tr("Cannot import Passkey"),
            tr("Cannot import Passkey file \"%1\". Private key is missing or malformed.").arg(file.fileName()));
    } else {
        showImportDialog(database, url, relyingParty, username, credentialId, userHandle, privateKey);
    }
}

void PasskeyImporter::showImportDialog(QSharedPointer<Database>& database,
                                       const QString& url,
                                       const QString& relyingParty,
                                       const QString& username,
                                       const QString& credentialId,
                                       const QString& userHandle,
                                       const QString& privateKey)
{
    PasskeyImportDialog passkeyImportDialog;
    passkeyImportDialog.setInfo(relyingParty, username, database);

    auto ret = passkeyImportDialog.exec();
    if (ret != QDialog::Accepted) {
        return;
    }

    auto db = passkeyImportDialog.getSelectedDatabase();
    if (!db) {
        db = database;
    }

    // Group settings. Use default group "Imported Passkeys" if user did not select a specific one.
    Group* group = nullptr;

    // Attempt to use the selected group
    if (!passkeyImportDialog.useDefaultGroup()) {
        auto groupUuid = passkeyImportDialog.getSelectedGroupUuid();
        group = db->rootGroup()->findGroupByUuid(groupUuid);
    }

    // Use default group if requested or if the selected group does not exist
    if (!group) {
        group = getDefaultGroup(db);
    }

    browserService()->addPasskeyToGroup(
        group, url, relyingParty, relyingParty, username, credentialId, userHandle, privateKey);
}

Group* PasskeyImporter::getDefaultGroup(QSharedPointer<Database>& database)
{
    auto defaultGroup = database->rootGroup()->findGroupByPath(IMPORTED_PASSKEYS_GROUP);

    // Create the default group if it does not exist
    if (!defaultGroup) {
        defaultGroup = new Group();
        defaultGroup->setName(IMPORTED_PASSKEYS_GROUP);
        defaultGroup->setUuid(QUuid::createUuid());
        defaultGroup->setParent(database->rootGroup());
    }

    return defaultGroup;
}
