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

#include <quazip5/quazip.h>
#include <quazip5/quazipfile.h>

namespace
{
    static const QString KeeShare_Signature("container.share.signature");
    static const QString KeeShare_Container("container.share.kdbx");

    enum Trust
    {
        None,
        Invalid,
        Single,
        Lasting,
        Known,
        Own
    };

    QPair<Trust, KeeShareSettings::Certificate> check(QByteArray& data,
                                                      const KeeShareSettings::Reference& reference,
                                                      const KeeShareSettings::Certificate& ownCertificate,
                                                      const QList<KeeShareSettings::Certificate>& knownCertificates,
                                                      const KeeShareSettings::Sign& sign)
    {
        if (sign.signature.isEmpty()) {
            QMessageBox warning;
            warning.setIcon(QMessageBox::Warning);
            warning.setWindowTitle(ShareObserver::tr("Untrustworthy container without signature"));
            warning.setText(ShareObserver::tr("Do you want to import from unsigned container %1").arg(reference.path));
            auto yes = warning.addButton(ShareObserver::tr("Import once"), QMessageBox::ButtonRole::YesRole);
            auto no = warning.addButton(ShareObserver::tr("No"), QMessageBox::ButtonRole::NoRole);
            warning.setDefaultButton(no);
            warning.exec();
            const auto trust = warning.clickedButton() == yes ? Single : None;
            return qMakePair(trust, KeeShareSettings::Certificate());
        }
        auto key = sign.certificate.sshKey();
        key.openKey(QString());
        const Signature signer;
        if (!signer.verify(data, sign.signature, key)) {
            const QFileInfo info(reference.path);
            qCritical("Invalid signature for sharing container %s.", qPrintable(info.absoluteFilePath()));
            return qMakePair(Invalid, KeeShareSettings::Certificate());
        }
        if (ownCertificate.key == sign.certificate.key) {
            return qMakePair(Own, ownCertificate);
        }
        for (const auto& certificate : knownCertificates) {
            if (certificate.key == certificate.key && certificate.trusted) {
                return qMakePair(Known, certificate);
            }
        }

        QMessageBox warning;
        warning.setIcon(QMessageBox::Question);
        warning.setWindowTitle(ShareObserver::tr("Import from untrustworthy certificate for sharing container"));
        warning.setText(ShareObserver::tr("Do you want to trust %1 with the fingerprint of %2")
                            .arg(sign.certificate.signer)
                            .arg(sign.certificate.fingerprint()));
        auto yes = warning.addButton(ShareObserver::tr("Import and trust"), QMessageBox::ButtonRole::YesRole);
        auto no = warning.addButton(ShareObserver::tr("No"), QMessageBox::ButtonRole::NoRole);
        warning.setDefaultButton(no);
        warning.exec();
        if (warning.clickedButton() != yes) {
            qWarning("Prevented import due to untrusted certificate of %s", qPrintable(sign.certificate.signer));
            return qMakePair(None, sign.certificate);
        }
        return qMakePair(Lasting, sign.certificate);
    }
}

ShareObserver::ShareObserver(Database* db, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_fileWatcher(new BulkFileWatcher(this))
{
    connect(KeeShare::instance(), SIGNAL(activeChanged()), this, SLOT(handleDatabaseChanged()));

    connect(m_db, SIGNAL(modified()), this, SLOT(handleDatabaseChanged()));

    connect(m_fileWatcher, SIGNAL(fileCreated(QString)), this, SLOT(handleFileCreated(QString)));
    connect(m_fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(handleFileChanged(QString)));
    connect(m_fileWatcher, SIGNAL(fileRemoved(QString)), this, SLOT(handleFileRemoved(QString)));
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
        if (couple.newReference.isValid() && ((active.in && couple.newReference.isImporting())
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
    for (Update update : updated) {
        if (!update.oldReference.path.isEmpty()) {
            m_fileWatcher->removePath(update.oldReference.path);
        }
        if (!update.newReference.path.isEmpty() && update.newReference.type != KeeShareSettings::Inactive) {
            m_fileWatcher->addPath(update.newReference.path);
        }

        if (update.newReference.isImporting()) {
            const Result result = this->importFromReferenceContainer(update.newReference.path);
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

void ShareObserver::handleFileUpdated(const QString& path, Change change)
{
    switch (change) {
    case Creation:
        qDebug("File created %s", qPrintable(path));
        break;
    case Update:
        qDebug("File changed %s", qPrintable(path));
        break;
    case Deletion:
        qDebug("File deleted %s", qPrintable(path));
        break;
    }

    const Result result = this->importFromReferenceContainer(path);
    if (!result.isValid()) {
        return;
    }
    QStringList success;
    QStringList warning;
    QStringList error;
    if (result.isError()) {
        error << tr("Import from %1 failed (%2)").arg(result.path).arg(result.message);
    } else if (result.isWarning()) {
        warning << tr("Import from %1 failed (%2)").arg(result.path).arg(result.message);
    } else if (result.isInfo()) {
        success << tr("Import from %1 successful (%2)").arg(result.path).arg(result.message);
    } else {
        success << tr("Imported from %1").arg(result.path);
    }
    notifyAbout(success, warning, error);
}

void ShareObserver::handleFileCreated(const QString& path)
{
    handleFileUpdated(path, Creation);
}

void ShareObserver::handleFileChanged(const QString& path)
{
    handleFileUpdated(path, Update);
}

void ShareObserver::handleFileRemoved(const QString& path)
{
    handleFileUpdated(path, Deletion);
}

ShareObserver::Result ShareObserver::importContainerInto(const KeeShareSettings::Reference& reference, Group* targetGroup)
{
    const QFileInfo info(reference.path);
    if (!info.exists()) {
        qCritical("File %s does not exist.", qPrintable(info.absoluteFilePath()));
        return {reference.path, Result::Warning, tr("File does not exist")};
    }
    QuaZip zip(info.absoluteFilePath());
    if (!zip.open(QuaZip::mdUnzip)) {
        qCritical("Unable to open file %s.", qPrintable(info.absoluteFilePath()));
        return {reference.path, Result::Error, tr("File is not readable")};
    }
    const auto expected = QSet<QString>() << KeeShare_Signature << KeeShare_Container;
    const auto files = zip.getFileInfoList();
    QSet<QString> actual;
    for (const auto& file : files) {
        actual << file.name;
    }
    if (expected != actual) {
        qCritical("Invalid sharing container %s.", qPrintable(info.absoluteFilePath()));
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
    auto* sourceDb = reader.readDatabase(&buffer, key);
    if (reader.hasError()) {
        qCritical("Error while parsing the database: %s", qPrintable(reader.errorString()));
        return {reference.path, Result::Error, reader.errorString()};
    }
    auto foreign = KeeShare::foreign();
    auto own = KeeShare::own();
    auto trusted = check(payload, reference, own.certificate, foreign.certificates, sign);
    switch (trusted.first) {
    case None:
        qWarning("Prevent untrusted import");
        return {reference.path, Result::Warning, tr("Untrusted import prevented")};

    case Invalid:
        qCritical("Prevent untrusted import");
        return {reference.path, Result::Error, tr("Untrusted import prevented")};

    case Known:
    case Lasting: {
        bool found = false;
        for (KeeShareSettings::Certificate& knownCertificate : foreign.certificates) {
            if (knownCertificate.key == trusted.second.key) {
                knownCertificate.signer = trusted.second.signer;
                knownCertificate.trusted = true;
                found = true;
            }
        }
        if (!found) {
            foreign.certificates << trusted.second;
            // we need to update with the new signer
            KeeShare::setForeign(foreign);
        }
    }
    [[gnu::fallthrough]];
    case Single:
    case Own: {
        qDebug("Synchronize %s %s with %s",
               qPrintable(reference.path),
               qPrintable(targetGroup->name()),
               qPrintable(sourceDb->rootGroup()->name()));
        Merger merger(sourceDb->rootGroup(), targetGroup);
        merger.setForcedMergeMode(Group::Synchronize);
        const bool changed = merger.merge();
        if (changed) {
            return {reference.path, Result::Success, tr("Successful import")};
        }
        return {};
    }
    default:
        Q_ASSERT(false);
        return {};
    }
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
        qDebug("Ignore change of inactive reference %s", qPrintable(reference.path));
        return {};
    }
    if (reference.type == KeeShareSettings::ExportTo) {
        qDebug("Ignore change of export reference %s", qPrintable(reference.path));
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
        const auto* sourceReference = sourceDb->resolveEntry(targetEntry->uuid());
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

const Database* ShareObserver::database() const
{
    return m_db;
}

Database* ShareObserver::database()
{
    return m_db;
}

void ShareObserver::handleDatabaseOpened()
{
    if (!m_db) {
        Q_ASSERT(m_db);
        return;
    }
    const auto active = KeeShare::active();
    if (!active.in && !active.out) {
        deinitialize();
    } else {
        reinitialize();
    }
}

QList<ShareObserver::Result> ShareObserver::exportIntoReferenceContainers()
{
    QList<Result> results;
    const auto own = KeeShare::own();
    const auto groups = m_db->rootGroup()->groupsRecursive(true);
    for (const auto* group : groups) {
        const auto reference = KeeShare::referenceOf(group);
        if (!reference.isExporting()) {
            continue;
        }

        m_fileWatcher->ignoreFileChanges(reference.path);
        QScopedPointer<Database> targetDb(exportIntoContainer(reference, group));
        QByteArray bytes;
        {
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            KeePass2Writer writer;
            writer.writeDatabase(&buffer, targetDb.data());
            if (writer.hasError()) {
                qWarning("Serializing export dabase failed: %s.", writer.errorString().toLatin1().data());
                results << Result{reference.path, Result::Error, writer.errorString()};
                m_fileWatcher->observeFileChanges(true);
                continue;
            }
        }
        QuaZip zip(reference.path);
        zip.setFileNameCodec("UTF-8");
        const bool zipOpened = zip.open(QuaZip::mdCreate);
        if (!zipOpened) {
            ::qWarning("Opening export file failed: %d", zip.getZipError());
            results << Result{reference.path, Result::Error, tr("Could not write export container (%1)").arg(zip.getZipError())};
            m_fileWatcher->observeFileChanges(true);
            continue;
        }
        {
            QuaZipFile file(&zip);
            const auto signatureOpened = file.open(QIODevice::WriteOnly, QuaZipNewInfo(KeeShare_Signature));
            if (!signatureOpened) {
                ::qWarning("Embedding signature failed: %d", zip.getZipError());
                results << Result{reference.path, Result::Error, tr("Could not embed signature (%1)").arg(file.getZipError())};
                m_fileWatcher->observeFileChanges(true);
                continue;
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
                results << Result{reference.path, Result::Error, tr("Could not embed signature (%1)").arg(file.getZipError())};
                m_fileWatcher->observeFileChanges(true);
                continue;
            }
            file.close();
        }
        {
            QuaZipFile file(&zip);
            const auto dbOpened = file.open(QIODevice::WriteOnly, QuaZipNewInfo(KeeShare_Container));
            if (!dbOpened) {
                ::qWarning("Embedding database failed: %d", zip.getZipError());
                results << Result{reference.path, Result::Error, tr("Could not embed database (%1)").arg(file.getZipError())};
                m_fileWatcher->observeFileChanges(true);
                continue;
            }
            if (file.getZipError() != ZIP_OK) {
                ::qWarning("Embedding database failed: %d", zip.getZipError());
                results << Result{reference.path, Result::Error, tr("Could not embed database (%1)").arg(file.getZipError())};
                m_fileWatcher->observeFileChanges(true);
                continue;
            }
            file.write(bytes);
            file.close();
        }
        zip.close();

        m_fileWatcher->observeFileChanges(true);
        results << Result{reference.path};
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
