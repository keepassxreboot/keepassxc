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
#include "config-keepassx.h"
#include "core/Clock.h"
#include "core/Config.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Entry.h"
#include "core/FilePath.h"
#include "core/FileWatcher.h"
#include "core/Group.h"
#include "core/Merger.h"
#include "core/Metadata.h"
#include "format/KeePass2Reader.h"
#include "format/KeePass2Writer.h"
#include "keeshare/KeeShare.h"
#include "keeshare/KeeShareSettings.h"
#include "keeshare/Signature.h"
#include "keys/PasswordKey.h"

#include <QBuffer>
#include <QDebug>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStringBuilder>

#if defined(WITH_XC_KEESHARE_SECURE)
#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>
#endif

namespace
{
    static const QString KeeShare_Signature("container.share.signature");
    static const QString KeeShare_Container("container.share.kdbx");

    enum Trust
    {
        Invalid,
        Own,
        UntrustedForever,
        UntrustedOnce,
        TrustedOnce,
        TrustedForever,
    };

    bool isOfExportType(const QFileInfo& fileInfo, const QString type)
    {
        return fileInfo.fileName().endsWith(type, Qt::CaseInsensitive);
    }

    QPair<Trust, KeeShareSettings::Certificate>
    check(QByteArray& data,
          const KeeShareSettings::Reference& reference,
          const KeeShareSettings::Certificate& ownCertificate,
          const QList<KeeShareSettings::ScopedCertificate>& knownCertificates,
          const KeeShareSettings::Sign& sign)
    {
        KeeShareSettings::Certificate certificate;
        if (!sign.signature.isEmpty()) {
            certificate = sign.certificate;
            auto key = sign.certificate.sshKey();
            key.openKey(QString());
            const auto signer = Signature();
            if (!signer.verify(data, sign.signature, key)) {
                qCritical("Invalid signature for sharing container %s.", qPrintable(reference.path));
                return {Invalid, KeeShareSettings::Certificate()};
            }

            if (ownCertificate.key == sign.certificate.key) {
                return {Own, ownCertificate};
            }
        }
        enum Scope
        {
            Invalid,
            Global,
            Local
        };
        Scope scope = Invalid;
        KeeShareSettings::Trust trusted = KeeShareSettings::Trust::Ask;
        for (const auto& scopedCertificate : knownCertificates) {
            if (scopedCertificate.certificate.key == certificate.key && scopedCertificate.path == reference.path) {
                // Global scope is overwritten by local scope
                scope = Global;
                trusted = scopedCertificate.trust;
            }
            if (scopedCertificate.certificate.key == certificate.key && scopedCertificate.path == reference.path) {
                scope = Local;
                trusted = scopedCertificate.trust;
                break;
            }
        }
        if (scope != Invalid && trusted != KeeShareSettings::Trust::Ask) {
            // we introduce now scopes if there is a global
            return {trusted == KeeShareSettings::Trust::Trusted ? TrustedForever : UntrustedForever, certificate};
        }

        QMessageBox warning;
        if (sign.signature.isEmpty()) {
            warning.setIcon(QMessageBox::Warning);
            warning.setWindowTitle(ShareObserver::tr("Import from container without signature"));
            warning.setText(ShareObserver::tr("We cannot verify the source of the shared container because it is not "
                                              "signed. Do you really want to import from %1?")
                                .arg(reference.path));
        } else {
            warning.setIcon(QMessageBox::Question);
            warning.setWindowTitle(ShareObserver::tr("Import from container with certificate"));
            warning.setText(ShareObserver::tr("Do you want to trust %1 with the fingerprint of %2 from %3")
                                .arg(certificate.signer, certificate.fingerprint(), reference.path));
        }
        auto untrustedOnce = warning.addButton(ShareObserver::tr("Not this time"), QMessageBox::ButtonRole::NoRole);
        auto untrustedForever = warning.addButton(ShareObserver::tr("Never"), QMessageBox::ButtonRole::NoRole);
        auto trustedForever = warning.addButton(ShareObserver::tr("Always"), QMessageBox::ButtonRole::YesRole);
        auto trustedOnce = warning.addButton(ShareObserver::tr("Just this time"), QMessageBox::ButtonRole::YesRole);
        warning.setDefaultButton(untrustedOnce);
        warning.exec();
        if (warning.clickedButton() == trustedForever) {
            return {TrustedForever, certificate};
        }
        if (warning.clickedButton() == trustedOnce) {
            return {TrustedOnce, certificate};
        }
        if (warning.clickedButton() == untrustedOnce) {
            return {UntrustedOnce, certificate};
        }
        if (warning.clickedButton() == untrustedForever) {
            return {UntrustedForever, certificate};
        }
        return {UntrustedOnce, certificate};
    }

} // End Namespace

ShareObserver::ShareObserver(QSharedPointer<Database> db, QObject* parent)
    : QObject(parent)
    , m_db(std::move(db))
    , m_fileWatcher(new BulkFileWatcher(this))
{
    connect(KeeShare::instance(), SIGNAL(activeChanged()), SLOT(handleDatabaseChanged()));

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

    const auto active = KeeShare::active();
    QList<Update> updated;
    QList<Group*> groups = m_db->rootGroup()->groupsRecursive(true);
    for (Group* group : groups) {
        Update couple{group, m_groupToReference.value(group), KeeShare::referenceOf(group)};
        if (couple.oldReference == couple.newReference) {
            continue;
        }

        m_groupToReference.remove(couple.group);
        m_referenceToGroup.remove(couple.oldReference);
        m_shareToGroup.remove(couple.oldReference.path);
        if (couple.newReference.isValid()
            && ((active.in && couple.newReference.isImporting())
                || (active.out && couple.newReference.isExporting()))) {
            m_groupToReference[couple.group] = couple.newReference;
            m_referenceToGroup[couple.newReference] = couple.group;
            m_shareToGroup[couple.newReference.path] = couple.group;
        }
        updated << couple;
    }

    QStringList success;
    QStringList warning;
    QStringList error;
    for (const auto& update : updated) {
        if (!update.oldReference.path.isEmpty()) {
            m_fileWatcher->removePath(update.oldReference.path);
        }

        if (!update.newReference.path.isEmpty() && update.newReference.type != KeeShareSettings::Inactive) {
            m_fileWatcher->addPath(update.newReference.path);
        }

        if (update.newReference.isImporting()) {
            const auto result = this->importFromReferenceContainer(update.newReference.path);
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

    notifyAbout(success, warning, error);
}

void ShareObserver::notifyAbout(const QStringList& success, const QStringList& warning, const QStringList& error)
{
    if (error.isEmpty() && warning.isEmpty() && success.isEmpty()) {
        return;
    }

    MessageWidget::MessageType type = MessageWidget::Positive;
    if (!warning.isEmpty()) {
        type = MessageWidget::Warning;
    }
    if (!error.isEmpty()) {
        type = MessageWidget::Error;
    }
    emit sharingMessage((success + warning + error).join("\n"), type);
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
    const Result result = this->importFromReferenceContainer(path);
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

ShareObserver::Result ShareObserver::importSingedContainerInto(const KeeShareSettings::Reference& reference,
                                                               Group* targetGroup)
{
#if !defined(WITH_XC_KEESHARE_SECURE)
    Q_UNUSED(targetGroup);
    return {reference.path, Result::Warning, tr("Signed share container are not supported - import prevented")};
#else
    QuaZip zip(reference.path);
    if (!zip.open(QuaZip::mdUnzip)) {
        qCritical("Unable to open file %s.", qPrintable(reference.path));
        return {reference.path, Result::Error, tr("File is not readable")};
    }
    const auto expected = QSet<QString>() << KeeShare_Signature << KeeShare_Container;
    const auto files = zip.getFileInfoList();
    QSet<QString> actual;
    for (const auto& file : files) {
        actual << file.name;
    }
    if (expected != actual) {
        qCritical("Invalid sharing container %s.", qPrintable(reference.path));
        return {reference.path, Result::Error, tr("Invalid sharing container")};
    }

    zip.setCurrentFile(KeeShare_Signature);
    QuaZipFile signatureFile(&zip);
    signatureFile.open(QuaZipFile::ReadOnly);
    QTextStream stream(&signatureFile);

    const auto sign = KeeShareSettings::Sign::deserialize(stream.readAll());
    signatureFile.close();

    zip.setCurrentFile(KeeShare_Container);
    QuaZipFile databaseFile(&zip);
    databaseFile.open(QuaZipFile::ReadOnly);
    auto payload = databaseFile.readAll();
    databaseFile.close();
    QBuffer buffer(&payload);
    buffer.open(QIODevice::ReadOnly);

    KeePass2Reader reader;
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(reference.password));
    auto sourceDb = QSharedPointer<Database>::create();
    if (!reader.readDatabase(&buffer, key, sourceDb.data())) {
        qCritical("Error while parsing the database: %s", qPrintable(reader.errorString()));
        return {reference.path, Result::Error, reader.errorString()};
    }

    auto foreign = KeeShare::foreign();
    auto own = KeeShare::own();
    auto trust = check(payload, reference, own.certificate, foreign.certificates, sign);
    switch (trust.first) {
    case Invalid:
        qWarning("Prevent untrusted import");
        return {reference.path, Result::Error, tr("Untrusted import prevented")};

    case UntrustedForever:
    case TrustedForever: {
        bool found = false;
        const auto trusted =
            trust.first == TrustedForever ? KeeShareSettings::Trust::Trusted : KeeShareSettings::Trust::Untrusted;
        for (KeeShareSettings::ScopedCertificate& scopedCertificate : foreign.certificates) {
            if (scopedCertificate.certificate.key == trust.second.key && scopedCertificate.path == reference.path) {
                scopedCertificate.certificate.signer = trust.second.signer;
                scopedCertificate.path = reference.path;
                scopedCertificate.trust = trusted;
                found = true;
                break;
            }
        }
        if (!found) {
            foreign.certificates << KeeShareSettings::ScopedCertificate{reference.path, trust.second, trusted};
        }
        // update foreign certificates with new settings
        KeeShare::setForeign(foreign);

        if (trust.first == TrustedForever) {
            qDebug("Synchronize %s %s with %s",
                   qPrintable(reference.path),
                   qPrintable(targetGroup->name()),
                   qPrintable(sourceDb->rootGroup()->name()));
            Merger merger(sourceDb->rootGroup(), targetGroup);
            merger.setForcedMergeMode(Group::Synchronize);
            const bool changed = merger.merge();
            if (changed) {
                return {reference.path, Result::Success, tr("Successful signed import")};
            }
        }
        // Silent ignore of untrusted import or unchanging import
        return {};
    }
    case TrustedOnce:
    case Own: {
        qDebug("Synchronize %s %s with %s",
               qPrintable(reference.path),
               qPrintable(targetGroup->name()),
               qPrintable(sourceDb->rootGroup()->name()));
        Merger merger(sourceDb->rootGroup(), targetGroup);
        merger.setForcedMergeMode(Group::Synchronize);
        const bool changed = merger.merge();
        if (changed) {
            return {reference.path, Result::Success, tr("Successful signed import")};
        }
        return {};
    }
    default:
        Q_ASSERT(false);
        return {reference.path, Result::Error, tr("Unexpected error")};
    }
#endif
}

ShareObserver::Result ShareObserver::importUnsignedContainerInto(const KeeShareSettings::Reference& reference,
                                                                 Group* targetGroup)
{
#if !defined(WITH_XC_KEESHARE_INSECURE)
    Q_UNUSED(targetGroup);
    return {reference.path, Result::Warning, tr("Unsigned share container are not supported - import prevented")};
#else
    QFile file(reference.path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Unable to open file %s.", qPrintable(reference.path));
        return {reference.path, Result::Error, tr("File is not readable")};
    }
    auto payload = file.readAll();
    file.close();
    QBuffer buffer(&payload);
    buffer.open(QIODevice::ReadOnly);

    KeePass2Reader reader;
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(reference.password));
    auto sourceDb = QSharedPointer<Database>::create();
    if (!reader.readDatabase(&buffer, key, sourceDb.data())) {
        qCritical("Error while parsing the database: %s", qPrintable(reader.errorString()));
        return {reference.path, Result::Error, reader.errorString()};
    }

    auto foreign = KeeShare::foreign();
    const auto own = KeeShare::own();
    const auto sign = KeeShareSettings::Sign(); // invalid sign
    auto trust = check(payload, reference, own.certificate, foreign.certificates, sign);
    switch (trust.first) {
    case UntrustedForever:
    case TrustedForever: {
        bool found = false;
        const auto trusted =
            trust.first == TrustedForever ? KeeShareSettings::Trust::Trusted : KeeShareSettings::Trust::Untrusted;
        for (KeeShareSettings::ScopedCertificate& scopedCertificate : foreign.certificates) {
            if (scopedCertificate.certificate.key == trust.second.key && scopedCertificate.path == reference.path) {
                scopedCertificate.certificate.signer = trust.second.signer;
                scopedCertificate.path = reference.path;
                scopedCertificate.trust = trusted;
                found = true;
                break;
            }
        }
        if (!found) {
            foreign.certificates << KeeShareSettings::ScopedCertificate{reference.path, trust.second, trusted};
        }
        // update foreign certificates with new settings
        KeeShare::setForeign(foreign);

        if (trust.first == TrustedForever) {
            qDebug("Synchronize %s %s with %s",
                   qPrintable(reference.path),
                   qPrintable(targetGroup->name()),
                   qPrintable(sourceDb->rootGroup()->name()));
            Merger merger(sourceDb->rootGroup(), targetGroup);
            merger.setForcedMergeMode(Group::Synchronize);
            const bool changed = merger.merge();
            if (changed) {
                return {reference.path, Result::Success, tr("Successful signed import")};
            }
        }
        return {};
    }

    case TrustedOnce: {
        qDebug("Synchronize %s %s with %s",
               qPrintable(reference.path),
               qPrintable(targetGroup->name()),
               qPrintable(sourceDb->rootGroup()->name()));
        Merger merger(sourceDb->rootGroup(), targetGroup);
        merger.setForcedMergeMode(Group::Synchronize);
        const bool changed = merger.merge();
        if (changed) {
            return {reference.path, Result::Success, tr("Successful unsigned import")};
        }
        return {};
    }
    default:
        qWarning("Prevent untrusted import");
        return {reference.path, Result::Warning, tr("Untrusted import prevented")};
    }
#endif
}

ShareObserver::Result ShareObserver::importContainerInto(const KeeShareSettings::Reference& reference,
                                                         Group* targetGroup)
{
    const QFileInfo info(reference.path);
    if (!info.exists()) {
        qCritical("File %s does not exist.", qPrintable(info.absoluteFilePath()));
        return {reference.path, Result::Warning, tr("File does not exist")};
    }

    if (isOfExportType(info, KeeShare::signedContainerFileType())) {
        return importSingedContainerInto(reference, targetGroup);
    }
    if (isOfExportType(info, KeeShare::unsignedContainerFileType())) {
        return importUnsignedContainerInto(reference, targetGroup);
    }
    return {reference.path, Result::Error, tr("Unknown share container type")};
}

ShareObserver::Result ShareObserver::importFromReferenceContainer(const QString& path)
{
    if (!KeeShare::active().in) {
        return {};
    }
    auto shareGroup = m_shareToGroup.value(path);
    if (!shareGroup) {
        qWarning("Source for %s does not exist", qPrintable(path));
        Q_ASSERT(shareGroup);
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
    return importContainerInto(reference, shareGroup);
}

void ShareObserver::resolveReferenceAttributes(Entry* targetEntry, const Database* sourceDb)
{
    for (const auto& attribute : EntryAttributes::DefaultAttributes) {
        const auto standardValue = targetEntry->attributes()->value(attribute);
        const auto type = targetEntry->placeholderType(standardValue);
        if (type != Entry::PlaceholderType::Reference) {
            // No reference to resolve
            continue;
        }
        const auto* referencedTargetEntry = targetEntry->resolveReference(standardValue);
        if (referencedTargetEntry) {
            // References is within scope, no resolving needed
            continue;
        }
        // We could do more sophisticated **** trying to point the reference to the next in-scope reference
        // but those cases with high propability constructed examples and very rare in real usage
        const auto* sourceReference = sourceDb->rootGroup()->findEntryByUuid(targetEntry->uuid());
        const auto resolvedValue = sourceReference->resolveMultiplePlaceholders(standardValue);
        targetEntry->setUpdateTimeinfo(false);
        targetEntry->attributes()->set(attribute, resolvedValue, targetEntry->attributes()->isProtected(attribute));
        targetEntry->setUpdateTimeinfo(true);
    }
}

Database* ShareObserver::exportIntoContainer(const KeeShareSettings::Reference& reference, const Group* sourceRoot)
{
    const auto* sourceDb = sourceRoot->database();
    auto* targetDb = new Database();
    targetDb->metadata()->setRecycleBinEnabled(false);
    auto key = QSharedPointer<CompositeKey>::create();
    key->addKey(QSharedPointer<PasswordKey>::create(reference.password));

    // Copy the source root as the root of the export database, memory manage the old root node
    auto* targetRoot = sourceRoot->clone(Entry::CloneNoFlags, Group::CloneNoFlags);
    const bool updateTimeinfo = targetRoot->canUpdateTimeinfo();
    targetRoot->setUpdateTimeinfo(false);
    KeeShare::setReferenceTo(targetRoot, KeeShareSettings::Reference());
    targetRoot->setUpdateTimeinfo(updateTimeinfo);
    const auto sourceEntries = sourceRoot->entriesRecursive(false);
    for (const Entry* sourceEntry : sourceEntries) {
        auto* targetEntry = sourceEntry->clone(Entry::CloneIncludeHistory);
        const bool updateTimeinfo = targetEntry->canUpdateTimeinfo();
        targetEntry->setUpdateTimeinfo(false);
        targetEntry->setGroup(targetRoot);
        targetEntry->setUpdateTimeinfo(updateTimeinfo);
        const auto iconUuid = targetEntry->iconUuid();
        if (!iconUuid.isNull()) {
            targetDb->metadata()->addCustomIcon(iconUuid, sourceEntry->icon());
        }
    }

    targetDb->setKey(key);
    auto* obsoleteRoot = targetDb->rootGroup();
    targetDb->setRootGroup(targetRoot);
    delete obsoleteRoot;

    targetDb->metadata()->setName(sourceRoot->name());

    // Push all deletions of the source database to the target
    // simple moving out of a share group will not trigger a deletion in the
    // target - a more elaborate mechanism may need the use of another custom
    // attribute to share unshared entries from the target db
    for (const auto& object : sourceDb->deletedObjects()) {
        targetDb->addDeletedObject(object);
    }
    for (auto* targetEntry : targetRoot->entriesRecursive(false)) {
        if (targetEntry->hasReferences()) {
            resolveReferenceAttributes(targetEntry, sourceDb);
        }
    }
    return targetDb;
}

QSharedPointer<Database> ShareObserver::database()
{
    return m_db;
}

ShareObserver::Result ShareObserver::exportIntoReferenceSignedContainer(const KeeShareSettings::Reference& reference,
                                                                        Database* targetDb)
{
#if !defined(WITH_XC_KEESHARE_SECURE)
    Q_UNUSED(targetDb);
    return {
        reference.path, Result::Warning, tr("Overwriting signed share container is not supported - export prevented")};
#else
    QByteArray bytes;
    {
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        KeePass2Writer writer;
        writer.writeDatabase(&buffer, targetDb);
        if (writer.hasError()) {
            qWarning("Serializing export dabase failed: %s.", writer.errorString().toLatin1().data());
            return {reference.path, Result::Error, writer.errorString()};
        }
    }
    const auto own = KeeShare::own();
    QuaZip zip(reference.path);
    zip.setFileNameCodec("UTF-8");
    const bool zipOpened = zip.open(QuaZip::mdCreate);
    if (!zipOpened) {
        ::qWarning("Opening export file failed: %d", zip.getZipError());
        return {reference.path, Result::Error, tr("Could not write export container (%1)").arg(zip.getZipError())};
    }
    {
        QuaZipFile file(&zip);
        const auto signatureOpened = file.open(QIODevice::WriteOnly, QuaZipNewInfo(KeeShare_Signature));
        if (!signatureOpened) {
            ::qWarning("Embedding signature failed: %d", zip.getZipError());
            return {reference.path, Result::Error, tr("Could not embed signature (%1)").arg(file.getZipError())};
        }
        QTextStream stream(&file);
        KeeShareSettings::Sign sign;
        auto sshKey = own.key.sshKey();
        sshKey.openKey(QString());
        const Signature signer;
        sign.signature = signer.create(bytes, sshKey);
        sign.certificate = own.certificate;
        stream << KeeShareSettings::Sign::serialize(sign);
        stream.flush();
        if (file.getZipError() != ZIP_OK) {
            ::qWarning("Embedding signature failed: %d", zip.getZipError());
            return {reference.path, Result::Error, tr("Could not embed signature (%1)").arg(file.getZipError())};
        }
        file.close();
    }
    {
        QuaZipFile file(&zip);
        const auto dbOpened = file.open(QIODevice::WriteOnly, QuaZipNewInfo(KeeShare_Container));
        if (!dbOpened) {
            ::qWarning("Embedding database failed: %d", zip.getZipError());
            return {reference.path, Result::Error, tr("Could not embed database (%1)").arg(file.getZipError())};
        }
        if (file.getZipError() != ZIP_OK) {
            ::qWarning("Embedding database failed: %d", zip.getZipError());
            return {reference.path, Result::Error, tr("Could not embed database (%1)").arg(file.getZipError())};
        }
        file.write(bytes);
        file.close();
    }
    zip.close();
    return {reference.path};
#endif
}

ShareObserver::Result ShareObserver::exportIntoReferenceUnsignedContainer(const KeeShareSettings::Reference& reference,
                                                                          Database* targetDb)
{
#if !defined(WITH_XC_KEESHARE_INSECURE)
    Q_UNUSED(targetDb);
    return {reference.path,
            Result::Warning,
            tr("Overwriting unsigned share container is not supported - export prevented")};
#else
    QFile file(reference.path);
    const bool fileOpened = file.open(QIODevice::WriteOnly);
    if (!fileOpened) {
        ::qWarning("Opening export file failed");
        return {reference.path, Result::Error, tr("Could not write export container")};
    }
    KeePass2Writer writer;
    writer.writeDatabase(&file, targetDb);
    if (writer.hasError()) {
        qWarning("Exporting dabase failed: %s.", writer.errorString().toLatin1().data());
        return {reference.path, Result::Error, writer.errorString()};
    }
    file.close();
#endif
    return {reference.path};
}

QList<ShareObserver::Result> ShareObserver::exportIntoReferenceContainers()
{
    QList<Result> results;
    const auto groups = m_db->rootGroup()->groupsRecursive(true);
    for (const auto* group : groups) {
        const auto reference = KeeShare::referenceOf(group);
        if (!reference.isExporting()) {
            continue;
        }

        m_fileWatcher->ignoreFileChanges(reference.path);
        QScopedPointer<Database> targetDb(exportIntoContainer(reference, group));
        QFileInfo info(reference.path);
        if (isOfExportType(info, KeeShare::signedContainerFileType())) {
            results << exportIntoReferenceSignedContainer(reference, targetDb.data());
            m_fileWatcher->observeFileChanges(true);
            continue;
        }
        if (isOfExportType(info, KeeShare::unsignedContainerFileType())) {
            results << exportIntoReferenceUnsignedContainer(reference, targetDb.data());
            m_fileWatcher->observeFileChanges(true);
            continue;
        }
        Q_ASSERT(false);
        results << Result{reference.path, Result::Error, tr("Unexpected export error occurred")};
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
    const auto results = exportIntoReferenceContainers();
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
    return !path.isEmpty() || !message.isEmpty() || !message.isEmpty() || !message.isEmpty();
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
