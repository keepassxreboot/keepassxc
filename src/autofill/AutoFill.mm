/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "AutoFill.h"
#include "AutoFillUtils.h"

#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "core/Tools.h"
#include "gui/DatabaseWidget.h"
#include "gui/MainWindow.h"

#include <QCoreApplication>
#include <QtGlobal>
#include <QLoggingCategory>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <algorithm>

#ifdef Q_OS_MACOS

#import <AuthenticationServices/AuthenticationServices.h>
#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>

#import "AutoFillProviderProtocol.h"
#import "AutoFillXPCProtocol.h"

Q_LOGGING_CATEGORY(lcAutoFill, "keepassxc.autofill")

namespace
{
NSString* toNSString(const QString& value)
{
    if (value.isEmpty()) {
        return @"";
    }
    return [NSString stringWithCharacters:reinterpret_cast<const unichar*>(value.constData()) length:value.size()];
}

QString fromNSString(NSString* value)
{
    if (!value) {
        return {};
    }

    const char* buffer = [value UTF8String];
    if (!buffer) {
        return {};
    }

    return QString::fromUtf8(buffer);
}
}

@class AutoFillHostAdapter;

class AutoFillPrivate : public QObject
{
public:
    struct CredentialRecord
    {
        QString recordIdentifier;
        QString domain;
        QString username;
        QString password;
        QString title;
        QString url;
        QString otp;

        bool isValid() const
        {
            return !recordIdentifier.isEmpty() && !domain.isEmpty() && !password.isEmpty();
        }
    };

    explicit AutoFillPrivate(AutoFill* parent);
    ~AutoFillPrivate() override;

    bool isAvailable() const
    {
        return m_available;
    }

    void start();
    void stop();

    void fetchCredentialsMatchingDomain(const QString& domain, void (^reply)(NSArray<NSDictionary<NSString*, id>*>*));
    void fetchCredentialWithRecordIdentifier(const QString& recordId, void (^reply)(NSDictionary<NSString*, id>*));

private:
    void connectSignals();
    void watchExistingDatabases();
    void watchDatabase(DatabaseWidget* widget);
    void scheduleIdentityRefresh();
    void refreshIdentityStore();
    void clearIdentityStore();
    void ensureListener();
    void connectToServiceIfNeeded();
    void handleServiceInvalidation();

    QVector<CredentialRecord> collectCredentialsForDomain(const QString& domain) const;
    QVector<CredentialRecord> collectAllCredentialRecords() const;
    CredentialRecord buildRecord(const QSharedPointer<Database>& database,
                                 Entry* entry,
                                 const QString& domain,
                                 const QString& sourceUrl) const;
    QStringList entryDomains(Entry* entry) const;
    QString recordIdentifierFor(const QSharedPointer<Database>& database, Entry* entry) const;
    Entry* entryForRecordIdentifier(const QString& recordId, QSharedPointer<Database>& database) const;
    DatabaseWidget* databaseWidgetForUuid(const QUuid& uuid) const;
    NSArray<NSDictionary<NSString*, id>*>* serializeCredentialList(const QVector<CredentialRecord>& records) const;
    NSDictionary<NSString*, id>* serializeCredential(const CredentialRecord& record) const;

    bool m_available{false};
    bool m_running{false};
    bool m_signalsConnected{false};
    bool m_serviceRegistered{false};
    QSet<DatabaseWidget*> m_watchedDatabases;
    QTimer* m_identityRefreshTimer{nullptr};
    NSXPCConnection* m_serviceConnection{nil};
    NSXPCListener* m_listener{nil};
    AutoFillHostAdapter* m_hostAdapter{nil};
};

@interface AutoFillHostAdapter : NSObject <NSXPCListenerDelegate, AutoFillProviderProtocol>
@property(nonatomic, assign) AutoFillPrivate* owner;
@end

@implementation AutoFillHostAdapter

- (BOOL)listener:(NSXPCListener*)listener shouldAcceptNewConnection:(NSXPCConnection*)connection
{
    Q_UNUSED(listener);
    connection.exportedInterface = [NSXPCInterface interfaceWithProtocol:@protocol(AutoFillProviderProtocol)];
    connection.exportedObject = self;
    [connection resume];
    return YES;
}

- (void)fetchCredentialsMatchingDomain:(NSString*)domain
                              withReply:(void (^)(NSArray<NSDictionary<NSString*, id>*>*))reply
{
    if (!reply) {
        return;
    }
    void (^replyCopy)(NSArray<NSDictionary<NSString*, id>*>*) = [reply copy];
    QString host = fromNSString(domain);
    AutoFillPrivate* owner = self.owner;
    if (!owner) {
        replyCopy(@[]);
        return;
    }
    owner->fetchCredentialsMatchingDomain(host, replyCopy);
}

- (void)fetchCredentialWithRecordIdentifier:(NSString*)recordIdentifier
                                   withReply:(void (^)(NSDictionary<NSString*, id>*))reply
{
    if (!reply) {
        return;
    }
    void (^replyCopy)(NSDictionary<NSString*, id>*) = [reply copy];
    QString identifier = fromNSString(recordIdentifier);
    AutoFillPrivate* owner = self.owner;
    if (!owner) {
        replyCopy(@{});
        return;
    }
    owner->fetchCredentialWithRecordIdentifier(identifier, replyCopy);
}

@end

AutoFillPrivate::AutoFillPrivate(AutoFill* parent)
    : QObject(parent)
{
    if (@available(macOS 12.0, *)) {
        m_available = true;
    }

    m_identityRefreshTimer = new QTimer(this);
    m_identityRefreshTimer->setSingleShot(true);
    m_identityRefreshTimer->setInterval(300);
    connect(m_identityRefreshTimer, &QTimer::timeout, this, [this]() { refreshIdentityStore(); });
}

AutoFillPrivate::~AutoFillPrivate()
{
    stop();
}

void AutoFillPrivate::start()
{
    qCDebug(lcAutoFill) << "Starting AutoFill service.";
    if (!m_available || m_running) {
        return;
    }

    m_running = true;
    connectSignals();
    watchExistingDatabases();
    ensureListener();
    connectToServiceIfNeeded();
    scheduleIdentityRefresh();
}

void AutoFillPrivate::stop()
{
    qCDebug(lcAutoFill) << "Stopping AutoFill service.";
    if (!m_running) {
        return;
    }

    m_running = false;
    m_watchedDatabases.clear();
    m_serviceRegistered = false;

    if (m_identityRefreshTimer) {
        m_identityRefreshTimer->stop();
    }

    if (m_serviceConnection) {
        [m_serviceConnection invalidate];
        m_serviceConnection.invalidationHandler = nil;
        m_serviceConnection = nil;
    }

    if (m_listener) {
        [m_listener invalidate];
        m_listener = nil;
    }

    if (m_hostAdapter) {
        m_hostAdapter.owner = nullptr;
    }
    m_hostAdapter = nil;
    clearIdentityStore();
}

void AutoFillPrivate::fetchCredentialsMatchingDomain(const QString& domain,
                                                       void (^reply)(NSArray<NSDictionary<NSString*, id>*>*))
{
    if (!reply) {
        return;
    }

    void (^replyCopy)(NSArray<NSDictionary<NSString*, id>*>*) = [reply copy];
    QPointer<AutoFillPrivate> guard(this);
    QMetaObject::invokeMethod(this,
                              [guard, replyCopy, domain]() {
                                  if (!guard) {
                                      replyCopy(@[]);
                                      return;
                                  }
                                  auto matches = guard->collectCredentialsForDomain(domain);
                                  replyCopy(guard->serializeCredentialList(matches));
                              },
                              Qt::QueuedConnection);
}

void AutoFillPrivate::fetchCredentialWithRecordIdentifier(const QString& recordId,
                                                            void (^reply)(NSDictionary<NSString*, id>*))
{
    if (!reply) {
        return;
    }

    void (^replyCopy)(NSDictionary<NSString*, id>*) = [reply copy];
    QPointer<AutoFillPrivate> guard(this);
    QMetaObject::invokeMethod(this,
                              [guard, replyCopy, recordId]() {
                                  if (!guard) {
                                      replyCopy(@{});
                                      return;
                                  }
                                  QSharedPointer<Database> database;
                                  auto* entry = guard->entryForRecordIdentifier(recordId, database);
                                  if (!entry || database.isNull()) {
                                      replyCopy(@{});
                                      return;
                                  }
                                  auto domains = guard->entryDomains(entry);
                                  if (domains.isEmpty()) {
                                      replyCopy(@{});
                                      return;
                                  }
                                  auto record = guard->buildRecord(database, entry, domains.first(), entry->displayUrl());
                                  replyCopy(guard->serializeCredential(record));
                              },
                              Qt::QueuedConnection);
}

void AutoFillPrivate::connectSignals()
{
    if (m_signalsConnected) {
        return;
    }

    if (auto* window = getMainWindow()) {
        connect(window, &MainWindow::databaseUnlocked, this, [this](DatabaseWidget* widget) {
            watchDatabase(widget);
            scheduleIdentityRefresh();
        });
        connect(window, &MainWindow::databaseLocked, this, [this](DatabaseWidget* widget) {
            m_watchedDatabases.remove(widget);
            scheduleIdentityRefresh();
        });
        connect(window, &MainWindow::activeDatabaseChanged, this, [this](DatabaseWidget*) {
            scheduleIdentityRefresh();
        });
    }

    m_signalsConnected = true;
}

void AutoFillPrivate::watchExistingDatabases()
{
    if (auto* window = getMainWindow()) {
        for (auto* widget : window->getOpenDatabases()) {
            watchDatabase(widget);
        }
    }
}

void AutoFillPrivate::watchDatabase(DatabaseWidget* widget)
{
    if (!widget || m_watchedDatabases.contains(widget)) {
        return;
    }

    m_watchedDatabases.insert(widget);

    connect(widget, &DatabaseWidget::databaseModified, this, [this]() { scheduleIdentityRefresh(); });
    connect(widget, &DatabaseWidget::databaseSaved, this, [this]() { scheduleIdentityRefresh(); });
    connect(widget, &DatabaseWidget::databaseReplaced, this, [this](const QSharedPointer<Database>&, const QSharedPointer<Database>&) {
        scheduleIdentityRefresh();
    });
    connect(widget, &DatabaseWidget::databaseLocked, this, [this, widget]() {
        m_watchedDatabases.remove(widget);
        scheduleIdentityRefresh();
    });
    connect(widget, &QObject::destroyed, this, [this, widget]() {
        m_watchedDatabases.remove(widget);
        scheduleIdentityRefresh();
    });
}

void AutoFillPrivate::scheduleIdentityRefresh()
{
    if (!m_available || !m_running || !m_identityRefreshTimer) {
        return;
    }

    if (!m_identityRefreshTimer->isActive()) {
        m_identityRefreshTimer->start();
    }
}

void AutoFillPrivate::refreshIdentityStore()
{
    if (!m_available) {
        return;
    }

    qCDebug(lcAutoFill) << "Refreshing identity store.";
    auto records = collectAllCredentialRecords();
    qCDebug(lcAutoFill) << "Found" << records.size() << "records to refresh.";

    if (@available(macOS 12.0, *)) {
        auto identities = [NSMutableArray arrayWithCapacity:records.size()];
        for (const auto& record : records) {
            auto* serviceIdentifier = [[ASCredentialServiceIdentifier alloc]
                initWithIdentifier:toNSString(record.domain)
                               type:ASCredentialServiceIdentifierTypeDomain];
            auto* identity = [[ASPasswordCredentialIdentity alloc]
                initWithServiceIdentifier:serviceIdentifier
                                     user:toNSString(record.username)
                        recordIdentifier:toNSString(record.recordIdentifier)];
            [identities addObject:identity];
        }

        dispatch_async(dispatch_get_main_queue(), ^{
            qCDebug(lcAutoFill) << "Calling replaceCredentialIdentitiesWithIdentities...";
            ASCredentialIdentityStore* store = [ASCredentialIdentityStore sharedStore];
            if (records.isEmpty()) {
                [store removeAllCredentialIdentitiesWithCompletion:nil];
                qCDebug(lcAutoFill) << "Cleared all identities.";
                return;
            }

            [store replaceCredentialIdentitiesWithIdentities:identities
                                                  completion:^(BOOL success, NSError* error) {
                                                      if (success) {
                                                          qCDebug(lcAutoFill) << "Successfully refreshed identities.";
                                                      } else if (error) {
                                                          qCWarning(lcAutoFill)
                                                              << "Unable to refresh AutoFill identities:"
                                                              << fromNSString(error.localizedDescription);
                                                      }
                                                  }];
        });
    }
}

void AutoFillPrivate::clearIdentityStore()
{
    if (!m_available) {
        return;
    }

    if (@available(macOS 12.0, *)) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [[ASCredentialIdentityStore sharedStore] removeAllCredentialIdentitiesWithCompletion:nil];
        });
    }
}

void AutoFillPrivate::ensureListener()
{
    if (m_listener) {
        return;
    }

    m_hostAdapter = [[AutoFillHostAdapter alloc] init];
    m_hostAdapter.owner = this;
    m_listener = [NSXPCListener anonymousListener];
    m_listener.delegate = m_hostAdapter;
    [m_listener resume];
}

void AutoFillPrivate::connectToServiceIfNeeded()
{
    if (m_serviceRegistered || !m_listener || !m_running) {
        return;
    }

    qCDebug(lcAutoFill) << "Connecting to AutoFill XPC service.";
    m_serviceConnection = [[NSXPCConnection alloc] initWithServiceName:@"org.keepassxc.KeePassXC.AutoFill-XPC-Service"];
    m_serviceConnection.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(AutoFillXPCProtocol)];

    // Use QPointer to safely prevent use-after-free if AutoFillPrivate
    // is destroyed while an invalidation callback is in-flight
    QPointer<AutoFillPrivate> guard(this);
    m_serviceConnection.invalidationHandler = ^{
        dispatch_async(dispatch_get_main_queue(), ^{
            if (guard) {
                guard->handleServiceInvalidation();
            }
        });
    };

    [m_serviceConnection resume];

    id<AutoFillXPCProtocol> remote =
        [m_serviceConnection remoteObjectProxyWithErrorHandler:^(NSError* error) {
            qCWarning(lcAutoFill) << "AutoFill XPC service unavailable:" << fromNSString(error.localizedDescription);
        }];

    [remote registerProvider:m_listener.endpoint
                   withReply:^(NSError* error) {
                       if (error) {
                           qCWarning(lcAutoFill) << "Failed to register AutoFill provider:"
                                                 << fromNSString(error.localizedDescription);
                           return;
                       }
                       qCDebug(lcAutoFill) << "Successfully registered AutoFill provider.";
                       m_serviceRegistered = true;
                   }];
}

void AutoFillPrivate::handleServiceInvalidation()
{
    qCDebug(lcAutoFill) << "AutoFill service connection invalidated.";
    if (!m_running) {
        if (m_serviceConnection) {
            m_serviceConnection.invalidationHandler = nil;
        }
        m_serviceConnection = nil;
        m_serviceRegistered = false;
        return;
    }

    m_serviceRegistered = false;
    if (m_serviceConnection) {
        m_serviceConnection.invalidationHandler = nil;
    }
    m_serviceConnection = nil;
    connectToServiceIfNeeded();
}

QVector<AutoFillPrivate::CredentialRecord> AutoFillPrivate::collectCredentialsForDomain(const QString& domain) const
{
    QVector<CredentialRecord> matches;
    if (domain.isEmpty()) {
        return collectAllCredentialRecords();
    }

    const auto normalizedDomain = AutoFillUtils::normalizeHost(domain);
    if (normalizedDomain.isEmpty()) {
        return matches;
    }

    if (auto* window = getMainWindow()) {
        for (auto* widget : window->getOpenDatabases()) {
            if (!widget || widget->isLocked()) {
                continue;
            }

            auto database = widget->database();
            if (database.isNull()) {
                continue;
            }

            auto* root = database->rootGroup();
            if (!root) {
                continue;
            }

            for (auto* entry : root->entriesRecursive(false)) {
                if (!entry || entry->isRecycled()) {
                    continue;
                }

                const auto domains = entryDomains(entry);
                auto matchIt = std::find_if(domains.begin(), domains.end(), [&](const QString& candidate) {
                    return AutoFillUtils::hostsMatch(normalizedDomain, candidate);
                });
                if (matchIt == domains.end()) {
                    continue;
                }

                auto record = buildRecord(database, entry, *matchIt, entry->displayUrl());
                if (record.isValid()) {
                    matches.append(record);
                }
            }
        }
    }
    return matches;
}

QVector<AutoFillPrivate::CredentialRecord> AutoFillPrivate::collectAllCredentialRecords() const
{
    QVector<CredentialRecord> records;
    QSet<QString> dedup;

    if (auto* window = getMainWindow()) {
        for (auto* widget : window->getOpenDatabases()) {
            if (!widget || widget->isLocked()) {
                continue;
            }
            auto database = widget->database();
            if (database.isNull()) {
                continue;
            }
            auto* root = database->rootGroup();
            if (!root) {
                continue;
            }

            for (auto* entry : root->entriesRecursive(false)) {
                if (!entry || entry->isRecycled()) {
                    continue;
                }

                for (const auto& domain : entryDomains(entry)) {
                    auto record = buildRecord(database, entry, domain, entry->displayUrl());
                    if (!record.isValid()) {
                        continue;
                    }

                    const auto key = record.recordIdentifier + QLatin1Char('|') + record.domain;
                    if (dedup.contains(key)) {
                        continue;
                    }
                    dedup.insert(key);
                    records.append(record);
                }
            }
        }
    }

    return records;
}

AutoFillPrivate::CredentialRecord AutoFillPrivate::buildRecord(const QSharedPointer<Database>& database,
                                                                   Entry* entry,
                                                                   const QString& domain,
                                                                   const QString& sourceUrl) const
{
    CredentialRecord record;
    if (database.isNull() || !entry) {
        return record;
    }

    record.recordIdentifier = recordIdentifierFor(database, entry);
    record.domain = domain;
    record.username = entry->resolveMultiplePlaceholders(entry->username());
    record.password = entry->resolveMultiplePlaceholders(entry->password());
    record.title = entry->resolveMultiplePlaceholders(entry->title());
    record.url = sourceUrl;
    if (entry->hasTotp()) {
        bool validTotp = false;
        const auto totpValue = entry->totp(&validTotp);
        if (validTotp) {
            record.otp = totpValue;
        }
    }
    return record;
}

QStringList AutoFillPrivate::entryDomains(Entry* entry) const
{
    QStringList domains;
    if (!entry) {
        return domains;
    }

    QSet<QString> uniqueHosts;
    for (const auto& url : entry->getAllUrls()) {
        const auto host = AutoFillUtils::hostFromUrl(url);
        if (host.isEmpty() || uniqueHosts.contains(host)) {
            continue;
        }
        uniqueHosts.insert(host);
        domains.append(host);
    }

    return domains;
}

QString AutoFillPrivate::recordIdentifierFor(const QSharedPointer<Database>& database, Entry* entry) const
{
    if (database.isNull() || !entry) {
        return {};
    }

    return database->uuid().toString(QUuid::WithoutBraces) + QLatin1Char(':') + entry->uuidToHex();
}

Entry* AutoFillPrivate::entryForRecordIdentifier(const QString& recordId, QSharedPointer<Database>& database) const
{
    database.clear();

    const auto parts = recordId.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    if (parts.size() != 2) {
        return nullptr;
    }

    QUuid databaseUuid(QStringLiteral("{%1}").arg(parts.at(0)));
    auto entryUuid = Tools::hexToUuid(parts.at(1));

    if (databaseUuid.isNull() || entryUuid.isNull()) {
        return nullptr;
    }

    if (auto* widget = databaseWidgetForUuid(databaseUuid)) {
        auto db = widget->database();
        if (!db.isNull() && db->rootGroup()) {
            database = db;
            return db->rootGroup()->findEntryByUuid(entryUuid);
        }
    }

    return nullptr;
}

DatabaseWidget* AutoFillPrivate::databaseWidgetForUuid(const QUuid& uuid) const
{
    if (uuid.isNull()) {
        return nullptr;
    }

    if (auto* window = getMainWindow()) {
        for (auto* widget : window->getOpenDatabases()) {
            if (!widget || widget->isLocked()) {
                continue;
            }
            auto database = widget->database();
            if (!database.isNull() && database->uuid() == uuid) {
                return widget;
            }
        }
    }

    return nullptr;
}

NSArray<NSDictionary<NSString*, id>*>* AutoFillPrivate::serializeCredentialList(
    const QVector<CredentialRecord>& records) const
{
    auto* list = [NSMutableArray arrayWithCapacity:records.size()];
    for (const auto& record : records) {
        auto* dictionary = serializeCredential(record);
        if (dictionary) {
            [list addObject:dictionary];
        }
    }
    return list;
}

NSDictionary<NSString*, id>* AutoFillPrivate::serializeCredential(const CredentialRecord& record) const
{
    if (!record.isValid()) {
        return nil;
    }

    NSMutableDictionary* dict = [NSMutableDictionary dictionary];
    dict[AutoFillCredentialRecordIdentifierKey] = toNSString(record.recordIdentifier);
    dict[AutoFillCredentialDomainKey] = toNSString(record.domain);
    dict[AutoFillCredentialUsernameKey] = toNSString(record.username);
    dict[AutoFillCredentialPasswordKey] = toNSString(record.password);
    dict[AutoFillCredentialTitleKey] = toNSString(record.title);
    dict[AutoFillCredentialUrlKey] = toNSString(record.url);
    if (!record.otp.isEmpty()) {
        dict[AutoFillCredentialOtpKey] = toNSString(record.otp);
    }
    return dict;
}

#else

class AutoFillPrivate
{
public:
    explicit AutoFillPrivate(AutoFill*) {}
    bool isAvailable() const
    {
        return false;
    }
    void start() {}
    void stop() {}
};

#endif // Q_OS_MACOS

AutoFill::AutoFill(QObject* parent)
    : QObject(parent)
    , d_ptr(new AutoFillPrivate(this))
{
}

AutoFill::~AutoFill()
{
    delete d_ptr;
}

bool AutoFill::isAvailable() const
{
    return d_ptr->isAvailable();
}

void AutoFill::start()
{
    d_ptr->start();
}

void AutoFill::stop()
{
    d_ptr->stop();
}
