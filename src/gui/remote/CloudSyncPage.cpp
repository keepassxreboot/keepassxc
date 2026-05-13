/*
 *  Copyright (C) 2026 Thongvan Alexis <thongvan.alexis@proton.me>
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

#include "CloudSyncPage.h"

#include "dropbox/DropboxCloudSyncPage.h"
#include "nextcloud/NextcloudCloudSyncPage.h"

#include <algorithm>

CloudSyncPage::CloudSyncPage(QWidget* parent)
    : QWidget(parent)
{
}

void CloudSyncPage::setRemoteSettings(RemoteSettings* /*settings*/)
{
    // Default: no-op. Subclasses that persist tokens override.
}

void CloudSyncPage::setMutualExclusivityWarning(bool /*active*/)
{
    // Default: no-op. Subclasses that need to disable their fields override.
}

QList<CloudSyncPage*> CloudSyncPage::createBuiltinPages(QWidget* parent)
{
    QList<CloudSyncPage*> pages;

    // The parent widget never has to learn about either concrete type --
    // it drives both pages through the abstract CloudSyncPage contract.
    auto* dropbox = new DropboxCloudSyncPage(parent);
    // Stable objectName so QObject::findChild lookups in tests / UI
    // introspection work regardless of insertion order.
    dropbox->setObjectName(QStringLiteral("dropboxPage"));
    pages.append(dropbox);

    auto* nextcloud = new NextcloudCloudSyncPage(parent);
    nextcloud->setObjectName(QStringLiteral("nextcloudPage"));
    pages.append(nextcloud);

    // Sort alphabetically by providerDisplayName() so the dropdown order is
    // independent of registration order. Provider names are ASCII, so
    // QString::compare with Qt::CaseSensitive suffices (locale-independent).
    // The sort lives ONLY in the factory; registerPage keeps m_pages and
    // providerComboBox parallel via insertion order.
    std::sort(pages.begin(), pages.end(), [](CloudSyncPage* a, CloudSyncPage* b) {
        return QString::compare(a->providerDisplayName(), b->providerDisplayName(), Qt::CaseSensitive) < 0;
    });

    return pages;
}
