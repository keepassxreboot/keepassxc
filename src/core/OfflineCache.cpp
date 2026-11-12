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

#include "OfflineCache.h"

#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "format/KdbxXmlWriter.h"
#include "format/KeePass2.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSemaphore>
#include <QSet>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QThreadPool>
#include <QtEndian>

#include <algorithm>
#include <atomic>

namespace
{
    constexpr auto kCacheFileName = "cache.kdbx";
    constexpr auto kStateFileName = "state.json";
    constexpr auto kVersionsDirName = "versions";

    QString hashHex(const QByteArray& data)
    {
        return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    }

    QString directoryNameFor(const QString& originalFilePath)
    {
        const auto path = OfflineCache::normalizeFilePath(originalFilePath);
        return QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha256).toHex().left(24));
    }

    QJsonObject fingerprintsToJson(const OfflineCache::Fingerprints& fp)
    {
        return {{"content", fp.content}, {"structure", fp.structure}};
    }

    OfflineCache::Fingerprints fingerprintsFromJson(const QJsonObject& obj)
    {
        return {obj["content"].toString(), obj["structure"].toString()};
    }
} // namespace

OfflineCache::OfflineCache(const QString& originalFilePath, const QString& baseDir)
    : m_originalFilePath(normalizeFilePath(originalFilePath))
    , m_directory(QDir(baseDir.isEmpty() ? configuredBaseDir() : baseDir).filePath(directoryNameFor(originalFilePath)))
    , m_maxVersions(configuredMaxVersions())
{
}

OfflineCache OfflineCache::forDatabase(const QString& originalFilePath)
{
    OfflineCache cache(originalFilePath, configuredBaseDir());
    cache.setMaxVersions(configuredMaxVersions());
    return cache;
}

QString OfflineCache::defaultBaseDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/offline");
}

QString OfflineCache::configuredBaseDir()
{
    const auto custom = config()->get(Config::OfflineCacheDir).toString().trimmed();
    return custom.isEmpty() ? defaultBaseDir() : custom;
}

int OfflineCache::configuredMaxVersions()
{
    const int val = config()->get(Config::OfflineCacheMaxVersions).toInt();
    return val > 0 ? val : DefaultMaxVersions;
}

QString OfflineCache::normalizeFilePath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return {};
    }
    QString path = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
#ifdef Q_OS_MACOS
    if (path.startsWith(QLatin1String("/var/"))) {
        path.prepend(QLatin1String("/private"));
    } else if (path.startsWith(QLatin1String("/tmp/"))) {
        path.prepend(QLatin1String("/private"));
    }
#endif
    return path;
}

OfflineCache::Override OfflineCache::localOverride(const QString& filePath)
{
    const auto key = normalizeFilePath(filePath);
    if (key.isEmpty()) {
        return Override::Unset;
    }
    const auto overrides = config()->get(Config::OfflineCacheDatabaseOverrides).toHash();
    if (!overrides.contains(key)) {
        return Override::Unset;
    }
    return overrides.value(key).toBool() ? Override::Enabled : Override::Disabled;
}

void OfflineCache::setLocalOverride(const QString& filePath, Override override)
{
    const auto key = normalizeFilePath(filePath);
    if (key.isEmpty()) {
        return;
    }
    auto overrides = config()->get(Config::OfflineCacheDatabaseOverrides).toHash();
    if (override == Override::Unset) {
        overrides.remove(key);
    } else {
        overrides.insert(key, override == Override::Enabled);
    }
    config()->set(Config::OfflineCacheDatabaseOverrides, overrides);
}

bool OfflineCache::isCloudDatabase(const Database* db)
{
    if (!db || db->filePath().isEmpty()) {
        return false;
    }
    const auto ov = localOverride(db->filePath());
    if (ov == Override::Enabled) {
        return true;
    }
    if (ov == Override::Disabled) {
        return false;
    }
    if (db->metadata() && db->metadata()->customData()) {
        const auto val = db->metadata()->customData()->value(CustomData::CloudFile);
        if (val == QStringLiteral("true") || val == QStringLiteral("1")) {
            return true;
        }
    }
    return forDatabase(db->filePath()).isSuggestedByDatabase();
}

bool OfflineCache::isCloudFilePath(const QString& filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }
    const auto ov = localOverride(filePath);
    if (ov == Override::Enabled) {
        return true;
    }
    if (ov == Override::Disabled) {
        return false;
    }
    const auto cache = forDatabase(filePath);
    return cache.isSuggestedByDatabase() || cache.hasCacheFile();
}

bool OfflineCache::canFallbackToCache(const QString& filePath)
{
    if (!isCloudFilePath(filePath)) {
        return false;
    }
    return forDatabase(filePath).hasCacheFile();
}

bool OfflineCache::isOriginalFileReachable(const QString& filePath, int timeoutMs)
{
    if (filePath.isEmpty()) {
        return false;
    }

    auto checkFn = [filePath]() -> bool {
        QFileInfo info(filePath);
        if (!info.exists() || !info.isFile() || info.size() < 64) {
            return false;
        }
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        const QByteArray header = file.read(8);
        if (header.size() < 8) {
            return false;
        }
        const auto sig1 = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(header.constData()));
        const auto sig2 = qFromLittleEndian<quint32>(reinterpret_cast<const uchar*>(header.constData() + 4));
        return sig1 == KeePass2::SIGNATURE_1 && sig2 == KeePass2::SIGNATURE_2;
    };

    if (timeoutMs <= 0) {
        return checkFn();
    }

    // Use a dedicated pool and deduplicate in-flight probes so a hung cloud/network mount
    // cannot exhaust QThreadPool::globalInstance() (which AsyncTask uses for database saves).
    static QThreadPool* reachabilityPool = []() {
        auto* pool = new QThreadPool();
        pool->setMaxThreadCount(2);
        return pool;
    }();
    static QMutex inFlightMutex;
    static QSet<QString> inFlightPaths;

    const QString key = normalizeFilePath(filePath);
    {
        QMutexLocker locker(&inFlightMutex);
        if (inFlightPaths.contains(key)) {
            return false;
        }
        inFlightPaths.insert(key);
    }

    auto done = QSharedPointer<QSemaphore>::create(0);
    auto ok = QSharedPointer<std::atomic<bool>>::create(false);
    reachabilityPool->start([checkFn, done, ok, key]() {
        ok->store(checkFn(), std::memory_order_relaxed);
        {
            QMutexLocker locker(&inFlightMutex);
            inFlightPaths.remove(key);
        }
        done->release(1);
    });

    if (!done->tryAcquire(1, timeoutMs)) {
        return false;
    }
    return ok->load(std::memory_order_relaxed);
}

/**
 * Compute the fingerprints of a database. The serialized XML only lives in memory and is never written to disk.
 */
OfflineCache::Fingerprints OfflineCache::fingerprint(const Database* db)
{
    Fingerprints result;
    if (!db || !db->rootGroup()) {
        return result;
    }

    // Without a random stream the XML writer emits protected values in plain text, which makes password
    // changes visible in the fingerprint.
    QByteArray xml;
    {
        QBuffer buffer(&xml);
        buffer.open(QIODevice::WriteOnly);
        if (db->formatVersion() >= KeePass2::FILE_VERSION_4) {
            KdbxXmlWriter writer(db->formatVersion(), KdbxXmlWriter::BinaryIdxMap{});
            writer.writeDatabase(&buffer, db);
        } else {
            KdbxXmlWriter writer(db->formatVersion());
            writer.writeDatabase(&buffer, db);
        }
        buffer.close();
    }

    // Remove everything that changes without the user changing any data. Besides the access statistics this is the
    // random slug written on every save and the custom data "last modified" bookkeeping that goes with it.
    static const QRegularExpression volatileFields(
        QStringLiteral("\\s*<LastAccessTime>[^<]*</LastAccessTime>|\\s*<UsageCount>[^<]*</UsageCount>"
                       "|\\s*<Item>\\s*<Key>(?:%1|%2)</Key>.*?</Item>")
            .arg(QRegularExpression::escape(CustomData::RandomSlug),
                 QRegularExpression::escape(CustomData::LastModified)),
        QRegularExpression::DotMatchesEverythingOption);
    auto text = QString::fromUtf8(xml);
    text.remove(volatileFields);
    // A custom data block that only held volatile items is empty now, so it must look like a missing block
    static const QRegularExpression emptyCustomData(QStringLiteral("\\s*<CustomData>\\s*</CustomData>"));
    text.remove(emptyCustomData);
    result.content = hashHex(text.toUtf8());

    QStringList ids;
    for (const auto* group : db->rootGroup()->groupsRecursive(true)) {
        ids << QStringLiteral("g:") + group->uuid().toString(QUuid::WithoutBraces);
    }
    for (const auto* entry : db->rootGroup()->entriesRecursive(false)) {
        ids << QStringLiteral("e:") + entry->uuid().toString(QUuid::WithoutBraces);
    }
    std::sort(ids.begin(), ids.end());
    ids << QStringLiteral("k:") + db->metadata()->databaseKeyChanged().toString(Qt::ISODateWithMs);
    result.structure = hashHex(ids.join('\n').toUtf8());

    return result;
}

QString OfflineCache::originalFilePath() const
{
    return m_originalFilePath;
}

QString OfflineCache::directory() const
{
    return m_directory;
}

QString OfflineCache::cacheFilePath() const
{
    return QDir(m_directory).filePath(kCacheFileName);
}

QString OfflineCache::versionsDirectory() const
{
    return QDir(m_directory).filePath(kVersionsDirName);
}

QString OfflineCache::stateFilePath() const
{
    return QDir(m_directory).filePath(kStateFileName);
}

bool OfflineCache::hasCacheFile() const
{
    return QFileInfo(cacheFilePath()).size() > 0;
}

QDateTime OfflineCache::cacheLastModified() const
{
    return QFileInfo(cacheFilePath()).lastModified();
}

int OfflineCache::maxVersions() const
{
    return m_maxVersions;
}

void OfflineCache::setMaxVersions(int maxVersions)
{
    m_maxVersions = std::max(1, maxVersions);
}

bool OfflineCache::isDirty() const
{
    return loadState().dirty;
}

bool OfflineCache::setDirty(bool dirty)
{
    auto state = loadState();
    if (state.dirty == dirty && QFile::exists(stateFilePath())) {
        return true;
    }
    state.dirty = dirty;
    return saveState(state);
}

QDateTime OfflineCache::lastSyncTime() const
{
    const auto state = loadState();
    return state.lastSyncTime.isValid() ? state.lastSyncTime : cacheLastModified();
}

bool OfflineCache::setLastSyncTime(const QDateTime& time)
{
    auto state = loadState();
    state.lastSyncTime = time;
    return saveState(state);
}

bool OfflineCache::isSuggestedByDatabase() const
{
    return loadState().suggestedByDatabase;
}

bool OfflineCache::setSuggestedByDatabase(bool suggested)
{
    auto state = loadState();
    if (state.suggestedByDatabase == suggested && QFile::exists(stateFilePath())) {
        return true;
    }
    state.suggestedByDatabase = suggested;
    return saveState(state);
}

bool OfflineCache::copyAtomically(const QString& source, const QString& destination, QString* error)
{
    QFile in(source);
    if (!in.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = in.errorString();
        }
        return false;
    }

    if (!QDir().mkpath(QFileInfo(destination).absolutePath())) {
        if (error) {
            *error = QStringLiteral("Cannot create directory %1").arg(QFileInfo(destination).absolutePath());
        }
        return false;
    }

    QSaveFile out(destination);
    if (!out.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = out.errorString();
        }
        return false;
    }

    constexpr qint64 chunk = 1 << 20;
    while (!in.atEnd()) {
        const auto data = in.read(chunk);
        if (data.isEmpty() && !in.atEnd()) {
            out.cancelWriting();
            if (error) {
                *error = in.errorString();
            }
            return false;
        }
        if (out.write(data) != data.size()) {
            out.cancelWriting();
            if (error) {
                *error = out.errorString();
            }
            return false;
        }
    }

    if (!out.commit()) {
        if (error) {
            *error = out.errorString();
        }
        return false;
    }
    // The file contains secrets, keep it private
    QFile::setPermissions(destination, QFile::ReadUser | QFile::WriteUser);
    return true;
}

bool OfflineCache::storeCopyOf(const QString& sourceFile, QString* error)
{
    if (!copyAtomically(sourceFile, cacheFilePath(), error)) {
        return false;
    }
    auto state = loadState();
    state.lastSyncTime = QDateTime::currentDateTime();
    saveState(state);
    return true;
}

OfflineCache::State OfflineCache::loadState() const
{
    State state;
    QFile file(stateFilePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return state;
    }
    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return state;
    }
    const auto root = doc.object();
    state.dirty = root["dirty"].toBool();
    state.suggestedByDatabase = root["suggestedByDatabase"].toBool();
    state.lastSyncTime = QDateTime::fromString(root["lastSyncTime"].toString(), Qt::ISODateWithMs);
    for (const auto& value : root["versions"].toArray()) {
        const auto obj = value.toObject();
        Version v;
        v.fileName = obj["file"].toString();
        v.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODateWithMs);
        v.windowStart = QDateTime::fromString(obj["windowStart"].toString(), Qt::ISODateWithMs);
        v.fingerprints = fingerprintsFromJson(obj["fingerprints"].toObject());
        v.structural = obj["structural"].toBool();
        state.versions << v;
    }
    return state;
}

bool OfflineCache::saveState(const State& state) const
{
    QJsonArray versions;
    for (const auto& v : state.versions) {
        versions.append(QJsonObject{{"file", v.fileName},
                                    {"timestamp", v.timestamp.toString(Qt::ISODateWithMs)},
                                    {"windowStart", v.windowStart.toString(Qt::ISODateWithMs)},
                                    {"fingerprints", fingerprintsToJson(v.fingerprints)},
                                    {"structural", v.structural}});
    }
    const QJsonObject root{{"originalPath", m_originalFilePath},
                           {"dirty", state.dirty},
                           {"suggestedByDatabase", state.suggestedByDatabase},
                           {"lastSyncTime", state.lastSyncTime.toString(Qt::ISODateWithMs)},
                           {"versions", versions}};

    if (!QDir().mkpath(m_directory)) {
        return false;
    }
    QSaveFile file(stateFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        return false;
    }
    QFile::setPermissions(stateFilePath(), QFile::ReadUser | QFile::WriteUser);
    return true;
}

QList<OfflineCache::Version> OfflineCache::versions() const
{
    return loadState().versions;
}

bool OfflineCache::addVersion(const QString& sourceFile,
                              const Fingerprints& fingerprints,
                              bool forceNewVersion,
                              const QDateTime& now,
                              QString* error)
{
    auto state = loadState();
    const QDir versionsDir(versionsDirectory());

    // versions are stored oldest first, the newest is last
    const bool hasNewest = !state.versions.isEmpty();
    const Version newest = hasNewest ? state.versions.last() : Version();

    if (hasNewest && !forceNewVersion && newest.fingerprints.content == fingerprints.content) {
        return true;
    }

    const bool structuralChange = !hasNewest || newest.fingerprints.structure != fingerprints.structure;
    const bool coalesce = hasNewest && !forceNewVersion && !structuralChange && !newest.structural
                          && newest.windowStart.isValid()
                          && newest.windowStart.secsTo(now) < DefaultCoalesceMinutes * 60;

    if (coalesce) {
        // Same slot, newer content
        if (!copyAtomically(sourceFile, versionsDir.filePath(newest.fileName), error)) {
            return false;
        }
        auto& last = state.versions.last();
        last.timestamp = now;
        last.fingerprints = fingerprints;
        return saveState(state);
    }

    // Start a new slot
    auto baseName = now.toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    QString fileName = baseName + QStringLiteral(".kdbx");
    for (int i = 1; versionsDir.exists(fileName); ++i) {
        fileName = QStringLiteral("%1-%2.kdbx").arg(baseName).arg(i);
    }
    if (!copyAtomically(sourceFile, versionsDir.filePath(fileName), error)) {
        return false;
    }

    Version version;
    version.fileName = fileName;
    version.timestamp = now;
    version.windowStart = now;
    version.fingerprints = fingerprints;
    version.structural = forceNewVersion || (hasNewest && structuralChange);
    state.versions << version;

    // Rotate
    while (state.versions.size() > m_maxVersions) {
        const auto oldest = state.versions.takeFirst();
        QFile::remove(versionsDir.filePath(oldest.fileName));
    }

    return saveState(state);
}

bool OfflineCache::removeAll()
{
    return QDir(m_directory).removeRecursively();
}
