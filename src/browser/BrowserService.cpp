/*
 *  Copyright (C) 2013 Francois Ferrand
 *  Copyright (C) 2017 Sami Vänttinen <sami.vanttinen@protonmail.com>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include <QInputDialog>
#include <QJsonArray>
#include <QMessageBox>
#include <QProgressDialog>
#include <QUuid>

#include "BrowserAccessControlDialog.h"
#include "BrowserEntryConfig.h"
#include "BrowserEntrySaveDialog.h"
#include "BrowserService.h"
#include "BrowserSettings.h"
#include "core/Database.h"
#include "core/EntrySearcher.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/PasswordGenerator.h"
#include "core/Tools.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#ifdef Q_OS_MACOS
#include "gui/macutils/MacUtils.h"
#endif

const char BrowserService::KEEPASSXCBROWSER_NAME[] = "KeePassXC-Browser Settings";
const char BrowserService::KEEPASSXCBROWSER_OLD_NAME[] = "keepassxc-browser Settings";
const char BrowserService::ASSOCIATE_KEY_PREFIX[] = "KPXC_BROWSER_";
static const char KEEPASSXCBROWSER_GROUP_NAME[] = "KeePassXC-Browser Passwords";
static int KEEPASSXCBROWSER_DEFAULT_ICON = 1;
// These are for the settings and password conversion
const char BrowserService::LEGACY_ASSOCIATE_KEY_PREFIX[] = "Public Key: ";
static const char KEEPASSHTTP_NAME[] = "KeePassHttp Settings";
static const char KEEPASSHTTP_GROUP_NAME[] = "KeePassHttp Passwords";

BrowserService::BrowserService(DatabaseTabWidget* parent)
    : m_dbTabWidget(parent)
    , m_dialogActive(false)
    , m_bringToFrontRequested(false)
    , m_wasMinimized(false)
    , m_wasHidden(false)
    , m_keepassBrowserUUID(QUuid::fromRfc4122(QByteArray::fromHex("de887cc3036343b8974b5911b8816224")))
{
    // Don't connect the signals when used from DatabaseSettingsWidgetBrowser (parent is nullptr)
    if (m_dbTabWidget) {
        connect(m_dbTabWidget, SIGNAL(databaseLocked(DatabaseWidget*)), this, SLOT(databaseLocked(DatabaseWidget*)));
        connect(
            m_dbTabWidget, SIGNAL(databaseUnlocked(DatabaseWidget*)), this, SLOT(databaseUnlocked(DatabaseWidget*)));
        connect(m_dbTabWidget,
                SIGNAL(activateDatabaseChanged(DatabaseWidget*)),
                this,
                SLOT(activateDatabaseChanged(DatabaseWidget*)));
    }
}

bool BrowserService::isDatabaseOpened() const
{
    DatabaseWidget* dbWidget = m_dbTabWidget->currentDatabaseWidget();
    if (!dbWidget) {
        return false;
    }

    return dbWidget->currentMode() == DatabaseWidget::Mode::ViewMode
           || dbWidget->currentMode() == DatabaseWidget::Mode::EditMode;
}

bool BrowserService::openDatabase(bool triggerUnlock)
{
    if (!browserSettings()->unlockDatabase()) {
        return false;
    }

    DatabaseWidget* dbWidget = m_dbTabWidget->currentDatabaseWidget();
    if (!dbWidget) {
        return false;
    }

    if (dbWidget->currentMode() == DatabaseWidget::Mode::ViewMode
        || dbWidget->currentMode() == DatabaseWidget::Mode::EditMode) {
        return true;
    }

    if (triggerUnlock) {
        m_bringToFrontRequested = true;
        raiseWindow(true);
    }

    return false;
}

void BrowserService::lockDatabase()
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "lockDatabase", Qt::BlockingQueuedConnection);
    }

    DatabaseWidget* dbWidget = m_dbTabWidget->currentDatabaseWidget();
    if (!dbWidget) {
        return;
    }

    if (dbWidget->currentMode() == DatabaseWidget::Mode::ViewMode
        || dbWidget->currentMode() == DatabaseWidget::Mode::EditMode) {
        dbWidget->lock();
    }
}

QString BrowserService::getDatabaseRootUuid()
{
    auto db = getDatabase();
    if (!db) {
        return {};
    }

    Group* rootGroup = db->rootGroup();
    if (!rootGroup) {
        return {};
    }

    return rootGroup->uuidToHex();
}

QString BrowserService::getDatabaseRecycleBinUuid()
{
    auto db = getDatabase();
    if (!db) {
        return {};
    }

    Group* recycleBin = db->metadata()->recycleBin();
    if (!recycleBin) {
        return {};
    }
    return recycleBin->uuidToHex();
}

QString BrowserService::storeKey(const QString& key)
{
    QString id;

    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(
            this, "storeKey", Qt::BlockingQueuedConnection, Q_RETURN_ARG(QString, id), Q_ARG(QString, key));
        return id;
    }

    auto db = getDatabase();
    if (!db) {
        return {};
    }

    bool contains;
    MessageBox::Button dialogResult = MessageBox::Cancel;

    do {
        QInputDialog keyDialog;
        keyDialog.setWindowTitle(tr("KeePassXC: New key association request"));
        keyDialog.setLabelText(tr("You have received an association request for the above key.\n\n"
                                  "If you would like to allow it access to your KeePassXC database,\n"
                                  "give it a unique name to identify and accept it."));
        keyDialog.setOkButtonText(tr("Save and allow access"));
        keyDialog.setWindowFlags(keyDialog.windowFlags() | Qt::WindowStaysOnTopHint);
        raiseWindow();
        keyDialog.show();
        keyDialog.activateWindow();
        keyDialog.raise();
        auto ok = keyDialog.exec();

        id = keyDialog.textValue();

        if (ok != QDialog::Accepted || id.isEmpty()) {
            hideWindow();
            return {};
        }

        contains = db->metadata()->customData()->contains(QLatin1String(ASSOCIATE_KEY_PREFIX) + id);
        if (contains) {
            dialogResult = MessageBox::warning(nullptr,
                                               tr("KeePassXC: Overwrite existing key?"),
                                               tr("A shared encryption key with the name \"%1\" "
                                                  "already exists.\nDo you want to overwrite it?")
                                                   .arg(id),
                                               MessageBox::Overwrite | MessageBox::Cancel,
                                               MessageBox::Cancel);
        }
    } while (contains && dialogResult == MessageBox::Cancel);

    hideWindow();
    db->metadata()->customData()->set(QLatin1String(ASSOCIATE_KEY_PREFIX) + id, key);
    return id;
}

QString BrowserService::getKey(const QString& id)
{
    auto db = getDatabase();
    if (!db) {
        return {};
    }

    return db->metadata()->customData()->value(QLatin1String(ASSOCIATE_KEY_PREFIX) + id);
}

QJsonArray BrowserService::findMatchingEntries(const QString& id,
                                               const QString& url,
                                               const QString& submitUrl,
                                               const QString& realm,
                                               const StringPairList& keyList,
                                               const bool httpAuth)
{
    QJsonArray result;
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this,
                                  "findMatchingEntries",
                                  Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(QJsonArray, result),
                                  Q_ARG(QString, id),
                                  Q_ARG(QString, url),
                                  Q_ARG(QString, submitUrl),
                                  Q_ARG(QString, realm),
                                  Q_ARG(StringPairList, keyList),
                                  Q_ARG(bool, httpAuth));
        return result;
    }

    const bool alwaysAllowAccess = browserSettings()->alwaysAllowAccess();
    const bool ignoreHttpAuth = browserSettings()->httpAuthPermission();
    const QString host = QUrl(url).host();
    const QString submitHost = QUrl(submitUrl).host();

    // Check entries for authorization
    QList<Entry*> pwEntriesToConfirm;
    QList<Entry*> pwEntries;
    for (Entry* entry : searchEntries(url, keyList)) {
        // HTTP Basic Auth always needs a confirmation
        if (!ignoreHttpAuth && httpAuth) {
            pwEntriesToConfirm.append(entry);
            continue;
        }

        switch (checkAccess(entry, host, submitHost, realm)) {
        case Denied:
            continue;

        case Unknown:
            if (alwaysAllowAccess) {
                pwEntries.append(entry);
            } else {
                pwEntriesToConfirm.append(entry);
            }
            break;

        case Allowed:
            pwEntries.append(entry);
            break;
        }
    }

    // Confirm entries
    if (confirmEntries(pwEntriesToConfirm, url, host, submitHost, realm)) {
        pwEntries.append(pwEntriesToConfirm);
    }

    if (pwEntries.isEmpty()) {
        return QJsonArray();
    }

    // Sort results
    pwEntries = sortEntries(pwEntries, host, submitUrl);

    // Fill the list
    for (Entry* entry : pwEntries) {
        result << prepareEntry(entry);
    }

    return result;
}

void BrowserService::addEntry(const QString& id,
                              const QString& login,
                              const QString& password,
                              const QString& url,
                              const QString& submitUrl,
                              const QString& realm,
                              QSharedPointer<Database> selectedDb)
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this,
                                  "addEntry",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, id),
                                  Q_ARG(QString, login),
                                  Q_ARG(QString, password),
                                  Q_ARG(QString, url),
                                  Q_ARG(QString, submitUrl),
                                  Q_ARG(QString, realm),
                                  Q_ARG(QSharedPointer<Database>, selectedDb));
    }

    auto db = selectedDb ? selectedDb : selectedDatabase();
    if (!db) {
        return;
    }

    Group* group = findCreateAddEntryGroup(db);
    if (!group) {
        return;
    }

    auto* entry = new Entry();
    entry->setUuid(QUuid::createUuid());
    entry->setTitle(QUrl(url).host());
    entry->setUrl(url);
    entry->setIcon(KEEPASSXCBROWSER_DEFAULT_ICON);
    entry->setUsername(login);
    entry->setPassword(password);
    entry->setGroup(group);

    const QString host = QUrl(url).host();
    const QString submitHost = QUrl(submitUrl).host();
    BrowserEntryConfig config;
    config.allow(host);

    if (!submitHost.isEmpty()) {
        config.allow(submitHost);
    }
    if (!realm.isEmpty()) {
        config.setRealm(realm);
    }
    config.save(entry);
}

void BrowserService::updateEntry(const QString& id,
                                 const QString& uuid,
                                 const QString& login,
                                 const QString& password,
                                 const QString& url,
                                 const QString& submitUrl)
{
    if (thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this,
                                  "updateEntry",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QString, id),
                                  Q_ARG(QString, uuid),
                                  Q_ARG(QString, login),
                                  Q_ARG(QString, password),
                                  Q_ARG(QString, url),
                                  Q_ARG(QString, submitUrl));
    }

    auto db = selectedDatabase();
    if (!db) {
        return;
    }

    Entry* entry = db->rootGroup()->findEntryByUuid(QUuid::fromRfc4122(QByteArray::fromHex(uuid.toLatin1())));
    if (!entry) {
        // If entry is not found for update, add a new one to the selected database
        addEntry(id, login, password, url, submitUrl, "", db);
        return;
    }

    // Check if the entry password is a reference. If so, update the original entry instead
    while (entry->attributes()->isReference(EntryAttributes::PasswordKey)) {
        const QUuid referenceUuid = entry->attributes()->referenceUuid(EntryAttributes::PasswordKey);
        if (!referenceUuid.isNull()) {
            entry = db->rootGroup()->findEntryByUuid(referenceUuid);
            if (!entry) {
                return;
            }
        }
    }

    QString username = entry->username();
    if (username.isEmpty()) {
        return;
    }

    if (username.compare(login, Qt::CaseSensitive) != 0
        || entry->password().compare(password, Qt::CaseSensitive) != 0) {
        MessageBox::Button dialogResult = MessageBox::No;
        if (!browserSettings()->alwaysAllowUpdate()) {
            raiseWindow();
            dialogResult = MessageBox::question(
                nullptr,
                tr("KeePassXC: Update Entry"),
                tr("Do you want to update the information in %1 - %2?").arg(QUrl(url).host(), username),
                MessageBox::Save | MessageBox::Cancel,
                MessageBox::Cancel,
                MessageBox::Raise);
        }

        if (browserSettings()->alwaysAllowUpdate() || dialogResult == MessageBox::Save) {
            entry->beginUpdate();
            if (!entry->attributes()->isReference(EntryAttributes::UserNameKey)) {
                entry->setUsername(login);
            }
            entry->setPassword(password);
            entry->endUpdate();
        }

        hideWindow();
    }
}

QList<Entry*> BrowserService::searchEntries(QSharedPointer<Database> db, const QString& hostname, const QString& url)
{
    QList<Entry*> entries;
    auto* rootGroup = db->rootGroup();
    if (!rootGroup) {
        return entries;
    }

    for (Entry* entry : EntrySearcher().search(baseDomain(hostname), rootGroup)) {
        QString entryUrl = entry->url();
        QUrl entryQUrl(entryUrl);
        QString entryScheme = entryQUrl.scheme();
        QUrl qUrl(url);

        // Ignore entry if port or scheme defined in the URL doesn't match
        if ((entryQUrl.port() > 0 && entryQUrl.port() != qUrl.port())
            || (browserSettings()->matchUrlScheme() && !entryScheme.isEmpty()
                && entryScheme.compare(qUrl.scheme()) != 0)) {
            continue;
        }

        // Filter to match hostname in URL field
        if ((!entryUrl.isEmpty() && hostname.contains(entryUrl))
            || (matchUrlScheme(entryUrl) && hostname.endsWith(entryQUrl.host()))) {
            entries.append(entry);
        }
    }

    return entries;
}

QList<Entry*> BrowserService::searchEntries(const QString& url, const StringPairList& keyList)
{
    // Get the list of databases to search
    QList<QSharedPointer<Database>> databases;
    if (browserSettings()->searchInAllDatabases()) {
        const int count = m_dbTabWidget->count();
        for (int i = 0; i < count; ++i) {
            if (auto* dbWidget = qobject_cast<DatabaseWidget*>(m_dbTabWidget->widget(i))) {
                if (const auto& db = dbWidget->database()) {
                    // Check if database is connected with KeePassXC-Browser
                    for (const StringPair& keyPair : keyList) {
                        QString key =
                            db->metadata()->customData()->value(QLatin1String(ASSOCIATE_KEY_PREFIX) + keyPair.first);
                        if (!key.isEmpty() && keyPair.second == key) {
                            databases << db;
                        }
                    }
                }
            }
        }
    } else if (const auto& db = getDatabase()) {
        databases << db;
    }

    // Search entries matching the hostname
    QString hostname = QUrl(url).host();
    QList<Entry*> entries;
    do {
        for (const auto& db : databases) {
            entries << searchEntries(db, hostname, url);
        }
    } while (entries.isEmpty() && removeFirstDomain(hostname));

    return entries;
}

void BrowserService::convertAttributesToCustomData(QSharedPointer<Database> currentDb)
{
    auto db = currentDb ? currentDb : getDatabase();
    if (!db) {
        return;
    }

    QList<Entry*> entries = db->rootGroup()->entriesRecursive();
    QProgressDialog progress(tr("Converting attributes to custom data…"), tr("Abort"), 0, entries.count());
    progress.setWindowModality(Qt::WindowModal);

    int counter = 0;
    int keyCounter = 0;
    for (Entry* entry : entries) {
        if (progress.wasCanceled()) {
            return;
        }

        if (moveSettingsToCustomData(entry, KEEPASSHTTP_NAME)) {
            ++counter;
        }

        if (moveSettingsToCustomData(entry, KEEPASSXCBROWSER_OLD_NAME)) {
            ++counter;
        }

        if (moveSettingsToCustomData(entry, KEEPASSXCBROWSER_NAME)) {
            ++counter;
        }

        if (entry->title() == KEEPASSHTTP_NAME || entry->title().contains(KEEPASSXCBROWSER_NAME, Qt::CaseInsensitive)) {
            keyCounter += moveKeysToCustomData(entry, db);
            delete entry;
        }

        progress.setValue(progress.value() + 1);
    }
    progress.reset();

    if (counter > 0) {
        MessageBox::information(nullptr,
                                tr("KeePassXC: Converted KeePassHTTP attributes"),
                                tr("Successfully converted attributes from %1 entry(s).\n"
                                   "Moved %2 keys to custom data.",
                                   "")
                                    .arg(counter)
                                    .arg(keyCounter),
                                MessageBox::Ok);
    } else if (counter == 0 && keyCounter > 0) {
        MessageBox::information(nullptr,
                                tr("KeePassXC: Converted KeePassHTTP attributes"),
                                tr("Successfully moved %n keys to custom data.", "", keyCounter),
                                MessageBox::Ok);
    } else {
        MessageBox::information(nullptr,
                                tr("KeePassXC: No entry with KeePassHTTP attributes found!"),
                                tr("The active database does not contain an entry with KeePassHTTP attributes."),
                                MessageBox::Ok);
    }

    // Rename password groupName
    Group* rootGroup = db->rootGroup();
    if (!rootGroup) {
        return;
    }

    const QString keePassBrowserGroupName = QLatin1String(KEEPASSXCBROWSER_GROUP_NAME);
    const QString keePassHttpGroupName = QLatin1String(KEEPASSHTTP_GROUP_NAME);

    for (Group* g : rootGroup->groupsRecursive(true)) {
        if (g->name() == keePassHttpGroupName) {
            g->setName(keePassBrowserGroupName);
            break;
        }
    }
}

QList<Entry*> BrowserService::sortEntries(QList<Entry*>& pwEntries, const QString& host, const QString& entryUrl)
{
    QUrl url(entryUrl);
    if (url.scheme().isEmpty()) {
        url.setScheme("http");
    }

    const QString submitUrl = url.toString(QUrl::StripTrailingSlash);
    const QString baseSubmitUrl =
        url.toString(QUrl::StripTrailingSlash | QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);

    // Build map of prioritized entries
    QMultiMap<int, Entry*> priorities;
    for (Entry* entry : pwEntries) {
        priorities.insert(sortPriority(entry, host, submitUrl, baseSubmitUrl), entry);
    }

    QList<Entry*> results;
    QString field = browserSettings()->sortByTitle() ? "Title" : "UserName";
    for (int i = 100; i >= 0; i -= 5) {
        if (priorities.count(i) > 0) {
            // Sort same priority entries by Title or UserName
            auto entries = priorities.values(i);
            std::sort(entries.begin(), entries.end(), [&field](Entry* left, Entry* right) {
                return (QString::localeAwareCompare(left->attributes()->value(field), right->attributes()->value(field))
                        < 0)
                       || ((QString::localeAwareCompare(left->attributes()->value(field),
                                                        right->attributes()->value(field))
                            == 0)
                           && (QString::localeAwareCompare(left->attributes()->value("UserName"),
                                                           right->attributes()->value("UserName"))
                               < 0));
            });
            results << entries;
            if (browserSettings()->bestMatchOnly() && !pwEntries.isEmpty()) {
                // Early out once we find the highest batch of matches
                break;
            }
        }
    }

    return results;
}

bool BrowserService::confirmEntries(QList<Entry*>& pwEntriesToConfirm,
                                    const QString& url,
                                    const QString& host,
                                    const QString& submitHost,
                                    const QString& realm)
{
    if (pwEntriesToConfirm.isEmpty() || m_dialogActive) {
        return false;
    }

    m_dialogActive = true;
    BrowserAccessControlDialog accessControlDialog;
    accessControlDialog.setUrl(url);
    accessControlDialog.setItems(pwEntriesToConfirm);

    raiseWindow();
    accessControlDialog.show();
    accessControlDialog.activateWindow();
    accessControlDialog.raise();

    int res = accessControlDialog.exec();
    if (accessControlDialog.remember()) {
        for (Entry* entry : pwEntriesToConfirm) {
            BrowserEntryConfig config;
            config.load(entry);
            if (res == QDialog::Accepted) {
                config.allow(host);
                if (!submitHost.isEmpty() && host != submitHost)
                    config.allow(submitHost);
            } else if (res == QDialog::Rejected) {
                config.deny(host);
                if (!submitHost.isEmpty() && host != submitHost) {
                    config.deny(submitHost);
                }
            }
            if (!realm.isEmpty()) {
                config.setRealm(realm);
            }
            config.save(entry);
        }
    }

    m_dialogActive = false;
    hideWindow();
    if (res == QDialog::Accepted) {
        return true;
    }

    return false;
}

QJsonObject BrowserService::prepareEntry(const Entry* entry)
{
    QJsonObject res;
    res["login"] = entry->resolveMultiplePlaceholders(entry->username());
    res["password"] = entry->resolveMultiplePlaceholders(entry->password());
    res["name"] = entry->resolveMultiplePlaceholders(entry->title());
    res["uuid"] = entry->resolveMultiplePlaceholders(entry->uuidToHex());

    if (entry->hasTotp()) {
        res["totp"] = entry->totp();
    }

    if (browserSettings()->supportKphFields()) {
        const EntryAttributes* attr = entry->attributes();
        QJsonArray stringFields;
        for (const QString& key : attr->keys()) {
            if (key.startsWith(QLatin1String("KPH: "))) {
                QJsonObject sField;
                sField[key] = entry->resolveMultiplePlaceholders(attr->value(key));
                stringFields << sField;
            }
        }
        res["stringFields"] = stringFields;
    }
    return res;
}

BrowserService::Access
BrowserService::checkAccess(const Entry* entry, const QString& host, const QString& submitHost, const QString& realm)
{
    BrowserEntryConfig config;
    if (!config.load(entry)) {
        return Unknown;
    }
    if (entry->isExpired()) {
        return Denied;
    }
    if ((config.isAllowed(host)) && (submitHost.isEmpty() || config.isAllowed(submitHost))) {
        return Allowed;
    }
    if ((config.isDenied(host)) || (!submitHost.isEmpty() && config.isDenied(submitHost))) {
        return Denied;
    }
    if (!realm.isEmpty() && config.realm() != realm) {
        return Denied;
    }
    return Unknown;
}

Group* BrowserService::findCreateAddEntryGroup(QSharedPointer<Database> selectedDb)
{
    auto db = selectedDb ? selectedDb : getDatabase();
    if (!db) {
        return nullptr;
    }

    Group* rootGroup = db->rootGroup();
    if (!rootGroup) {
        return nullptr;
    }

    const QString groupName =
        QLatin1String(KEEPASSXCBROWSER_GROUP_NAME); // TODO: setting to decide where new keys are created

    for (const Group* g : rootGroup->groupsRecursive(true)) {
        if (g->name() == groupName) {
            return db->rootGroup()->findGroupByUuid(g->uuid());
        }
    }

    auto* group = new Group();
    group->setUuid(QUuid::createUuid());
    group->setName(groupName);
    group->setIcon(KEEPASSXCBROWSER_DEFAULT_ICON);
    group->setParent(rootGroup);
    return group;
}

int BrowserService::sortPriority(const Entry* entry,
                                 const QString& host,
                                 const QString& submitUrl,
                                 const QString& baseSubmitUrl) const
{
    QUrl url(entry->url());
    if (url.scheme().isEmpty()) {
        url.setScheme("http");
    }
    const QString entryURL = url.toString(QUrl::StripTrailingSlash);
    const QString baseEntryURL =
        url.toString(QUrl::StripTrailingSlash | QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);

    if (submitUrl == entryURL) {
        return 100;
    }
    if (submitUrl.startsWith(entryURL) && entryURL != host && baseSubmitUrl != entryURL) {
        return 90;
    }
    if (submitUrl.startsWith(baseEntryURL) && entryURL != host && baseSubmitUrl != baseEntryURL) {
        return 80;
    }
    if (entryURL == host) {
        return 70;
    }
    if (entryURL == baseSubmitUrl) {
        return 60;
    }
    if (entryURL.startsWith(submitUrl)) {
        return 50;
    }
    if (entryURL.startsWith(baseSubmitUrl) && baseSubmitUrl != host) {
        return 40;
    }
    if (submitUrl.startsWith(entryURL)) {
        return 30;
    }
    if (submitUrl.startsWith(baseEntryURL)) {
        return 20;
    }
    if (entryURL.startsWith(host)) {
        return 10;
    }
    if (host.startsWith(entryURL)) {
        return 5;
    }
    return 0;
}

bool BrowserService::matchUrlScheme(const QString& url)
{
    QUrl address(url);
    return !address.scheme().isEmpty();
}

bool BrowserService::removeFirstDomain(QString& hostname)
{
    int pos = hostname.indexOf(".");
    if (pos < 0) {
        return false;
    }

    // Don't remove the second-level domain if it's the only one
    if (hostname.count(".") > 1) {
        hostname = hostname.mid(pos + 1);
        return !hostname.isEmpty();
    }

    // Nothing removed
    return false;
}

/**
 * Gets the base domain of URL.
 *
 * Returns the base domain, e.g. https://another.example.co.uk -> example.co.uk
 */
QString BrowserService::baseDomain(const QString& url) const
{
    QUrl qurl = QUrl::fromUserInput(url);
    QString hostname = qurl.host();

    if (hostname.isEmpty() || !hostname.contains(qurl.topLevelDomain())) {
        return {};
    }

    // Remove the top level domain part from the hostname, e.g. https://another.example.co.uk -> https://another.example
    hostname.chop(qurl.topLevelDomain().length());
    // Split the URL and select the last part, e.g. https://another.example -> example
    QString baseDomain = hostname.split('.').last();
    // Append the top level domain back to the URL, e.g. example -> example.co.uk
    baseDomain.append(qurl.topLevelDomain());
    return baseDomain;
}

QSharedPointer<Database> BrowserService::getDatabase()
{
    if (DatabaseWidget* dbWidget = m_dbTabWidget->currentDatabaseWidget()) {
        if (const auto& db = dbWidget->database()) {
            return db;
        }
    }
    return {};
}

QSharedPointer<Database> BrowserService::selectedDatabase()
{
    QList<DatabaseWidget*> databaseWidgets;
    for (int i = 0;; ++i) {
        auto* dbWidget = m_dbTabWidget->databaseWidgetFromIndex(i);
        // Add only open databases
        if (dbWidget && dbWidget->database()->hasKey()
            && (dbWidget->currentMode() == DatabaseWidget::Mode::ViewMode
                || dbWidget->currentMode() == DatabaseWidget::Mode::EditMode)) {
            databaseWidgets.push_back(dbWidget);
            continue;
        }

        // Break out if dbStruct.dbWidget is nullptr
        break;
    }

    BrowserEntrySaveDialog browserEntrySaveDialog;
    int openDatabaseCount = browserEntrySaveDialog.setItems(databaseWidgets, m_dbTabWidget->currentDatabaseWidget());
    if (openDatabaseCount > 1) {
        int res = browserEntrySaveDialog.exec();
        if (res == QDialog::Accepted) {
            const auto selectedDatabase = browserEntrySaveDialog.getSelected();
            if (selectedDatabase.length() > 0) {
                int index = selectedDatabase[0]->data(Qt::UserRole).toUInt();
                return databaseWidgets[index]->database();
            }
        } else {
            return {};
        }
    }

    // Return current database
    return getDatabase();
}

bool BrowserService::moveSettingsToCustomData(Entry* entry, const QString& name) const
{
    if (entry->attributes()->contains(name)) {
        QString attr = entry->attributes()->value(name);
        entry->beginUpdate();
        if (!attr.isEmpty()) {
            entry->customData()->set(KEEPASSXCBROWSER_NAME, attr);
        }
        entry->attributes()->remove(name);
        entry->endUpdate();
        return true;
    }
    return false;
}

int BrowserService::moveKeysToCustomData(Entry* entry, QSharedPointer<Database> db) const
{
    int keyCounter = 0;
    for (const auto& key : entry->attributes()->keys()) {
        if (key.contains(LEGACY_ASSOCIATE_KEY_PREFIX)) {
            QString publicKey = key;
            publicKey.remove(LEGACY_ASSOCIATE_KEY_PREFIX);

            // Add key to database custom data
            if (db && !db->metadata()->customData()->contains(QLatin1String(ASSOCIATE_KEY_PREFIX) + publicKey)) {
                db->metadata()->customData()->set(QLatin1String(ASSOCIATE_KEY_PREFIX) + publicKey,
                                                  entry->attributes()->value(key));
                ++keyCounter;
            }
        }
    }

    return keyCounter;
}

bool BrowserService::checkLegacySettings()
{
    auto db = getDatabase();
    if (!db) {
        return false;
    }

    bool legacySettingsFound = false;
    QList<Entry*> entries = db->rootGroup()->entriesRecursive();
    for (const auto& e : entries) {
        if ((e->attributes()->contains(KEEPASSHTTP_NAME) || e->attributes()->contains(KEEPASSXCBROWSER_NAME))
            || (e->title() == KEEPASSHTTP_NAME || e->title().contains(KEEPASSXCBROWSER_NAME, Qt::CaseInsensitive))) {
            legacySettingsFound = true;
            break;
        }
    }

    if (!legacySettingsFound) {
        return false;
    }

    auto dialogResult = MessageBox::warning(nullptr,
                                            tr("KeePassXC: Legacy browser integration settings detected"),
                                            tr("Legacy browser integration settings have been detected.\n"
                                               "Do you want to upgrade the settings to the latest standard?\n"
                                               "This is necessary to maintain compatibility with the browser plugin."),
                                            MessageBox::Yes | MessageBox::No);

    return dialogResult == MessageBox::Yes;
}

void BrowserService::hideWindow() const
{
    if (m_wasMinimized) {
        getMainWindow()->showMinimized();
    } else {
#ifdef Q_OS_MACOS
        if (m_wasHidden) {
            macUtils()->hideOwnWindow();
        } else {
            macUtils()->raiseLastActiveWindow();
        }
#else
        getMainWindow()->lower();
#endif
    }
}

void BrowserService::raiseWindow(const bool force)
{
    m_wasMinimized = getMainWindow()->isMinimized();
#ifdef Q_OS_MACOS
    m_wasHidden = macUtils()->isHidden();
    macUtils()->raiseOwnWindow();
    Tools::wait(500);
#else
    if (force) {
        getMainWindow()->bringToFront();
    }
#endif
}

void BrowserService::databaseLocked(DatabaseWidget* dbWidget)
{
    if (dbWidget) {
        emit databaseLocked();
    }
}

void BrowserService::databaseUnlocked(DatabaseWidget* dbWidget)
{
    if (dbWidget) {
        if (m_bringToFrontRequested) {
            hideWindow();
            m_bringToFrontRequested = false;
        }
        emit databaseUnlocked();

        if (checkLegacySettings()) {
            convertAttributesToCustomData();
        }
    }
}

void BrowserService::activateDatabaseChanged(DatabaseWidget* dbWidget)
{
    if (dbWidget) {
        auto currentMode = dbWidget->currentMode();
        if (currentMode == DatabaseWidget::Mode::ViewMode || currentMode == DatabaseWidget::Mode::EditMode) {
            emit databaseUnlocked();
        } else {
            emit databaseLocked();
        }
    }
}
