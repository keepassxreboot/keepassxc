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
#include "core/Tools.h"
#include "gui/DatabaseWidget.h"
#include "gui/MainWindow.h"

#include <QLoggingCategory>
#include <QPointer>
#include <QSet>
#include <QSharedPointer>
#include <QStringList>
#include <QTimer>
#include <QVector>
#include <QtGlobal>

#import <AuthenticationServices/AuthenticationServices.h>
#import <Foundation/Foundation.h>
#import <Security/Security.h>
#import <dispatch/dispatch.h>
#include <libproc.h>
#include <unistd.h>

#import "AutoFillProtocol.h"

Q_LOGGING_CATEGORY(lcAutoFill, "keepassxc.autofill")

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
        bool hasPassword{false};

        bool isValid() const
        {
            return !recordIdentifier.isEmpty() && !domain.isEmpty() && hasPassword;
        }
    };

    explicit AutoFillPrivate(AutoFill* parent);
    ~AutoFillPrivate() override;

    void start();
    void stop();

    NSArray<NSDictionary<NSString*, id>*>* credentialsForDomain(const QString& domain) const;
    NSDictionary<NSString*, id>* credentialForRecordIdentifier(const QString& recordId) const;

private:
    static CFDataRef handleMessagePortCall(CFMessagePortRef local, SInt32 msgid, CFDataRef data, void* info);
    static bool isAuthorizedCaller(pid_t callerPid);

    void connectSignals();
    void watchExistingDatabases();
    void watchDatabase(DatabaseWidget* widget);
    void scheduleIdentityRefresh();
    void refreshIdentityStore();
    void ensureMessagePort();
    void closeMessagePort();

    QVector<CredentialRecord> collectCredentials(const QString& domain = {}) const;
    CredentialRecord buildRecord(const QSharedPointer<Database>& database,
                                 Entry* entry,
                                 const QString& domain,
                                 bool includePassword) const;
    QStringList entryDomains(Entry* entry) const;
    Entry* entryForRecordIdentifier(const QString& recordId, QSharedPointer<Database>& database) const;
    NSDictionary<NSString*, id>* serializeCredential(const CredentialRecord& record, bool includePassword) const;

    bool m_running{false};
    bool m_signalsConnected{false};
    QSet<DatabaseWidget*> m_watchedDatabases;
    QTimer* m_identityRefreshTimer{nullptr};
    CFMessagePortRef m_messagePort{nullptr};
    CFRunLoopSourceRef m_runLoopSource{nullptr};
};

AutoFillPrivate::AutoFillPrivate(AutoFill* parent)
    : QObject(parent)
{
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
    if (!AutoFill::isAvailable() || m_running) {
        return;
    }

    qCDebug(lcAutoFill) << "Starting AutoFill service.";
    m_running = true;
    connectSignals();
    watchExistingDatabases();
    ensureMessagePort();
    scheduleIdentityRefresh();
}

void AutoFillPrivate::stop()
{
    if (!m_running) {
        return;
    }

    qCDebug(lcAutoFill) << "Stopping AutoFill service.";
    m_running = false;

    if (m_identityRefreshTimer) {
        m_identityRefreshTimer->stop();
    }

    closeMessagePort();
    if (@available(macOS 12.0, *)) {
        [[ASCredentialIdentityStore sharedStore] removeAllCredentialIdentitiesWithCompletion:nil];
    }
}

NSArray<NSDictionary<NSString*, id>*>* AutoFillPrivate::credentialsForDomain(const QString& domain) const
{
    if (!m_running) {
        return @[];
    }
    const auto records = collectCredentials(domain);
    auto* list = [NSMutableArray arrayWithCapacity:records.size()];
    for (const auto& record : records) {
        if (auto* dictionary = serializeCredential(record, false)) {
            [list addObject:dictionary];
        }
    }
    return list;
}

NSDictionary<NSString*, id>* AutoFillPrivate::credentialForRecordIdentifier(const QString& recordId) const
{
    if (!m_running) {
        return @{};
    }

    QSharedPointer<Database> database;
    auto* entry = entryForRecordIdentifier(recordId, database);
    if (!entry || database.isNull()) {
        return @{};
    }

    auto domains = entryDomains(entry);
    if (domains.isEmpty()) {
        return @{};
    }

    auto record = buildRecord(database, entry, domains.first(), true);
    return serializeCredential(record, true) ?: @{};
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
        connect(window, &MainWindow::databaseLocked, this, [this](DatabaseWidget*) {
            scheduleIdentityRefresh();
        });
        connect(window, &MainWindow::activeDatabaseChanged, this, [this](DatabaseWidget* widget) {
            watchDatabase(widget);
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
    auto refresh = [this]() { scheduleIdentityRefresh(); };
    connect(widget, &DatabaseWidget::databaseModified, this, refresh);
    connect(widget, &DatabaseWidget::databaseSaved, this, refresh);
    connect(widget, &DatabaseWidget::databaseReplaced, this, [refresh](const auto&, const auto&) { refresh(); });
    connect(widget, &DatabaseWidget::databaseLocked, this, refresh);
    connect(widget, &QObject::destroyed, this, [this, widget]() {
        m_watchedDatabases.remove(widget);
        scheduleIdentityRefresh();
    });
}

void AutoFillPrivate::scheduleIdentityRefresh()
{
    if (m_running && m_identityRefreshTimer && !m_identityRefreshTimer->isActive()) {
        m_identityRefreshTimer->start();
    }
}

void AutoFillPrivate::refreshIdentityStore()
{
    if (!m_running) {
        return;
    }

    if (@available(macOS 12.0, *)) {
        auto records = collectCredentials();
        ASCredentialIdentityStore* store = [ASCredentialIdentityStore sharedStore];
        if (records.isEmpty()) {
            [store removeAllCredentialIdentitiesWithCompletion:nil];
            return;
        }

        auto* identities = [NSMutableArray arrayWithCapacity:records.size()];
        for (const auto& record : records) {
            auto* serviceIdentifier = [[ASCredentialServiceIdentifier alloc]
                initWithIdentifier:record.domain.toNSString()
                              type:ASCredentialServiceIdentifierTypeDomain];
            auto* identity = [[ASPasswordCredentialIdentity alloc]
                initWithServiceIdentifier:serviceIdentifier
                                     user:record.username.toNSString()
                         recordIdentifier:record.recordIdentifier.toNSString()];
            [identities addObject:identity];
        }

        [store replaceCredentialIdentitiesWithIdentities:identities
                                              completion:^(BOOL, NSError* error) {
                                                  if (error) {
                                                      qCWarning(lcAutoFill)
                                                          << "Unable to refresh AutoFill identities:"
                                                          << QString::fromNSString(error.localizedDescription);
                                                  }
                                              }];
    }
}

// Verify that the IPC caller is our own embedded keepassxc-autofill.appex process:
// 1. Owned by the current user, spawned by launchd (ppid == 1), and not being debugged/traced.
// 2. Valid in-memory code signature whose CDHash (kSecCodeInfoUnique) matches the embedded .appex bundle.
bool AutoFillPrivate::isAuthorizedCaller(pid_t callerPid)
{
    if (callerPid <= 1) {
        return false;
    }

    struct proc_bsdinfo bsdInfo = {};
    if (proc_pidinfo(callerPid, PROC_PIDTBSDINFO, 0, &bsdInfo, sizeof(bsdInfo)) != sizeof(bsdInfo)) {
        return false;
    }
    if (bsdInfo.pbi_uid != getuid() || bsdInfo.pbi_ppid != 1 || (bsdInfo.pbi_flags & PROC_FLAG_TRACED) != 0) {
        return false;
    }

    NSURL* extensionUrl =
        [NSBundle.mainBundle.builtInPlugInsURL URLByAppendingPathComponent:@"keepassxc-autofill.appex"];
    if (!extensionUrl) {
        return false;
    }

    SecStaticCodeRef expectedCode = nullptr;
    if (SecStaticCodeCreateWithPath((__bridge CFURLRef)extensionUrl, kSecCSDefaultFlags, &expectedCode) != errSecSuccess
        || !expectedCode) {
        return false;
    }
    CFDictionaryRef expectedInfoRef = nullptr;
    OSStatus expectedStatus = SecCodeCopySigningInformation(expectedCode, kSecCSDefaultFlags, &expectedInfoRef);
    CFRelease(expectedCode);
    if (expectedStatus != errSecSuccess || !expectedInfoRef) {
        return false;
    }
    NSDictionary* expectedInfo = CFBridgingRelease(expectedInfoRef);
    NSData* expectedCdHash = expectedInfo[(__bridge NSString*)kSecCodeInfoUnique];
    if (![expectedCdHash isKindOfClass:NSData.class] || expectedCdHash.length == 0) {
        return false;
    }

    SecCodeRef guestCode = nullptr;
    NSDictionary* guestAttrs = @{(__bridge NSString*)kSecGuestAttributePid : @(callerPid)};
    if (SecCodeCopyGuestWithAttributes(nullptr, (__bridge CFDictionaryRef)guestAttrs, kSecCSDefaultFlags, &guestCode)
            != errSecSuccess
        || !guestCode) {
        return false;
    }
    if (SecCodeCheckValidity(guestCode, kSecCSDefaultFlags, nullptr) != errSecSuccess) {
        CFRelease(guestCode);
        return false;
    }
    CFDictionaryRef guestInfoRef = nullptr;
    OSStatus guestStatus = SecCodeCopySigningInformation(guestCode, kSecCSDynamicInformation, &guestInfoRef);
    CFRelease(guestCode);
    if (guestStatus != errSecSuccess || !guestInfoRef) {
        return false;
    }
    NSDictionary* guestInfo = CFBridgingRelease(guestInfoRef);
    NSData* callerCdHash = guestInfo[(__bridge NSString*)kSecCodeInfoUnique];
    return [callerCdHash isKindOfClass:NSData.class] && [callerCdHash isEqualToData:expectedCdHash];
}

CFDataRef AutoFillPrivate::handleMessagePortCall(CFMessagePortRef, SInt32 msgid, CFDataRef data, void* info)
{
    auto* self = static_cast<AutoFillPrivate*>(info);
    if (!self || !self->m_running || !data) {
        return nullptr;
    }

    NSData* inputData = (__bridge NSData*)data;
    id plist = [NSPropertyListSerialization propertyListWithData:inputData
                                                         options:NSPropertyListImmutable
                                                          format:nullptr
                                                           error:nullptr];
    if (![plist isKindOfClass:NSDictionary.class]) {
        return nullptr;
    }

    NSDictionary* request = plist;
    NSNumber* pidNumber = request[AutoFillRequestPidKey];
    NSString* query = request[AutoFillRequestQueryKey];
    if (![pidNumber isKindOfClass:NSNumber.class] || ![query isKindOfClass:NSString.class]) {
        return nullptr;
    }

    if (!isAuthorizedCaller(static_cast<pid_t>(pidNumber.intValue))) {
        qCWarning(lcAutoFill) << "Rejected AutoFill IPC request from unauthorized PID" << pidNumber.intValue;
        return nullptr;
    }

    id responseObj = nil;
    if (msgid == AutoFillMessageFetchCredentialsForDomain) {
        responseObj = self->credentialsForDomain(QString::fromNSString(query));
    } else if (msgid == AutoFillMessageFetchCredentialByRecordId) {
        responseObj = self->credentialForRecordIdentifier(QString::fromNSString(query));
    }
    if (!responseObj) {
        return nullptr;
    }

    NSData* responseData = [NSPropertyListSerialization dataWithPropertyList:responseObj
                                                                      format:NSPropertyListBinaryFormat_v1_0
                                                                     options:0
                                                                       error:nil];
    return responseData ? static_cast<CFDataRef>(CFBridgingRetain(responseData)) : nullptr;
}

void AutoFillPrivate::ensureMessagePort()
{
    if (m_messagePort) {
        return;
    }

    CFMessagePortContext context = {0, this, nullptr, nullptr, nullptr};
    m_messagePort = CFMessagePortCreateLocal(
        kCFAllocatorDefault, (__bridge CFStringRef)AutoFillMachPortName, handleMessagePortCall, &context, nullptr);
    if (!m_messagePort) {
        qCWarning(lcAutoFill) << "Failed to create AutoFill local Mach message port.";
        return;
    }

    m_runLoopSource = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, m_messagePort, 0);
    if (m_runLoopSource) {
        CFRunLoopAddSource(CFRunLoopGetMain(), m_runLoopSource, kCFRunLoopCommonModes);
    }
}

void AutoFillPrivate::closeMessagePort()
{
    if (m_runLoopSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), m_runLoopSource, kCFRunLoopCommonModes);
        CFRelease(m_runLoopSource);
        m_runLoopSource = nullptr;
    }
    if (m_messagePort) {
        CFMessagePortInvalidate(m_messagePort);
        CFRelease(m_messagePort);
        m_messagePort = nullptr;
    }
}

QVector<AutoFillPrivate::CredentialRecord> AutoFillPrivate::collectCredentials(const QString& domain) const
{
    QVector<CredentialRecord> records;
    const auto normalizedDomain = domain.isEmpty() ? QString() : AutoFillUtils::hostFromUrl(domain);
    if (!domain.isEmpty() && normalizedDomain.isEmpty()) {
        return records;
    }

    auto* window = getMainWindow();
    if (!window) {
        return records;
    }

    QSet<QString> dedup;
    for (auto* widget : window->getOpenDatabases()) {
        if (!widget || widget->isLocked()) {
            continue;
        }
        auto database = widget->database();
        if (database.isNull() || !database->rootGroup()) {
            continue;
        }

        for (auto* entry : database->rootGroup()->entriesRecursive(false)) {
            if (!entry || entry->isRecycled() || entry->isExpired()) {
                continue;
            }

            for (const auto& candidateDomain : entryDomains(entry)) {
                if (!normalizedDomain.isEmpty() && !AutoFillUtils::hostsMatch(normalizedDomain, candidateDomain)) {
                    continue;
                }
                auto record = buildRecord(database, entry, candidateDomain, false);
                if (!record.isValid()) {
                    continue;
                }
                const auto key = record.recordIdentifier + QLatin1Char('|') + record.domain;
                if (!dedup.contains(key)) {
                    dedup.insert(key);
                    records.append(record);
                }
                if (!normalizedDomain.isEmpty()) {
                    break;
                }
            }
        }
    }
    return records;
}

AutoFillPrivate::CredentialRecord AutoFillPrivate::buildRecord(const QSharedPointer<Database>& database,
                                                               Entry* entry,
                                                               const QString& domain,
                                                               bool includePassword) const
{
    CredentialRecord record;
    if (database.isNull() || !entry) {
        return record;
    }

    record.recordIdentifier = database->uuid().toString(QUuid::WithoutBraces) + QLatin1Char(':') + entry->uuidToHex();
    record.domain = domain;
    record.username = entry->resolveMultiplePlaceholders(entry->username());
    record.title = entry->resolveMultiplePlaceholders(entry->title());
    record.hasPassword = !entry->password().isEmpty();
    if (includePassword && record.hasPassword) {
        record.password = entry->resolveMultiplePlaceholders(entry->password());
        record.hasPassword = !record.password.isEmpty();
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

    if (auto* window = getMainWindow()) {
        for (auto* widget : window->getOpenDatabases()) {
            if (widget && !widget->isLocked() && widget->database() && widget->database()->uuid() == databaseUuid) {
                auto db = widget->database();
                if (db->rootGroup()) {
                    auto* entry = db->rootGroup()->findEntryByUuid(entryUuid);
                    if (entry && !entry->isRecycled() && !entry->isExpired()) {
                        database = db;
                        return entry;
                    }
                }
                return nullptr;
            }
        }
    }

    return nullptr;
}

NSDictionary<NSString*, id>* AutoFillPrivate::serializeCredential(const CredentialRecord& record,
                                                                  bool includePassword) const
{
    if (!record.isValid() || (includePassword && record.password.isEmpty())) {
        return nil;
    }

    NSMutableDictionary* dict = [NSMutableDictionary dictionary];
    dict[AutoFillCredentialRecordIdentifierKey] = record.recordIdentifier.toNSString();
    dict[AutoFillCredentialDomainKey] = record.domain.toNSString();
    dict[AutoFillCredentialUsernameKey] = record.username.toNSString();
    dict[AutoFillCredentialTitleKey] = record.title.toNSString();
    if (includePassword) {
        dict[AutoFillCredentialPasswordKey] = record.password.toNSString();
    }
    return dict;
}

AutoFill::AutoFill(QObject* parent)
    : QObject(parent)
    , d_ptr(new AutoFillPrivate(this))
{
}

AutoFill::~AutoFill()
{
    delete d_ptr;
}

bool AutoFill::isAvailable()
{
    if (@available(macOS 12.0, *)) {
        return true;
    }
    return false;
}

void AutoFill::start()
{
    d_ptr->start();
}

void AutoFill::stop()
{
    d_ptr->stop();
}
