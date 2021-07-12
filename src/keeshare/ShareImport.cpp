/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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
#include "ShareImport.h"
#include "config-keepassx.h"
#include "core/Merger.h"
#include "format/KeePass2Reader.h"
#include "keeshare/KeeShare.h"
#include "keeshare/Signature.h"
#include "keys/PasswordKey.h"

#include <QBuffer>
#include <QMessageBox>
#include <QPushButton>
#include <QTextStream>

#if defined(WITH_XC_KEESHARE_SECURE)
#include <botan/pk_keys.h>
#include <quazipfile.h>
#endif

namespace
{
    enum Trust
    {
        Invalid,
        Own,
        UntrustedForever,
        UntrustedOnce,
        TrustedOnce,
        TrustedForever,
    };

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
            if (!Signature::verify(data, sign.certificate.key, sign.signature)) {
                qCritical("Invalid signature for shared container %s.", qPrintable(reference.path));
                return {Invalid, KeeShareSettings::Certificate()};
            }

            // Automatically trust your own certificate
            if (ownCertificate == sign.certificate) {
                return {Own, ownCertificate};
            }
        }

        for (const auto& scopedCertificate : knownCertificates) {
            if (scopedCertificate.certificate == certificate && scopedCertificate.path == reference.path) {
                if (scopedCertificate.trust == KeeShareSettings::Trust::Trusted) {
                    return {TrustedForever, certificate};
                } else if (scopedCertificate.trust == KeeShareSettings::Trust::Untrusted) {
                    return {UntrustedForever, certificate};
                }
                // Default to ask
                break;
            }
        }

        // Ask the user if they want to trust the certificate
        QMessageBox warning;
        warning.setWindowTitle(ShareImport::tr("KeeShare Import"));
        if (sign.signature.isEmpty()) {
            warning.setIcon(QMessageBox::Warning);
            warning.setText(ShareImport::tr("The source of the shared container cannot be verified because it is not "
                                            "signed. Do you really want to import from %1?")
                                .arg(reference.path));
        } else {
            warning.setIcon(QMessageBox::Question);
            warning.setText(ShareImport::tr("Do you want to trust %1 with certificate fingerprint:\n%2\n%3")
                                .arg(reference.path)
                                .arg(certificate.signer)
                                .arg(certificate.fingerprint()));
        }
        auto untrustedOnce = warning.addButton(ShareImport::tr("Not this time"), QMessageBox::ButtonRole::NoRole);
        auto untrustedForever = warning.addButton(ShareImport::tr("Never"), QMessageBox::ButtonRole::NoRole);
        auto trustedForever = warning.addButton(ShareImport::tr("Always"), QMessageBox::ButtonRole::YesRole);
        auto trustedOnce = warning.addButton(ShareImport::tr("Just this time"), QMessageBox::ButtonRole::YesRole);
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

    ShareObserver::Result
    signedContainerInto(const QString& resolvedPath, const KeeShareSettings::Reference& reference, Group* targetGroup)
    {
#if !defined(WITH_XC_KEESHARE_SECURE)
        Q_UNUSED(targetGroup);
        Q_UNUSED(resolvedPath);
        return {reference.path,
                ShareObserver::Result::Warning,
                ShareImport::tr("Signed share container are not supported - import prevented")};
#else
        QuaZip zip(resolvedPath);
        if (!zip.open(QuaZip::mdUnzip)) {
            qCritical("Unable to open file %s.", qPrintable(reference.path));
            return {reference.path, ShareObserver::Result::Error, ShareImport::tr("File is not readable")};
        }
        const auto expected = QSet<QString>() << KeeShare::signatureFileName() << KeeShare::containerFileName();
        const auto files = zip.getFileInfoList();
        QSet<QString> actual;
        for (const auto& file : files) {
            actual << file.name;
        }
        if (expected != actual) {
            qCritical("Invalid sharing container %s.", qPrintable(reference.path));
            return {reference.path, ShareObserver::Result::Error, ShareImport::tr("Invalid sharing container")};
        }

        zip.setCurrentFile(KeeShare::signatureFileName());
        QuaZipFile signatureFile(&zip);
        signatureFile.open(QuaZipFile::ReadOnly);
        QTextStream stream(&signatureFile);

        const auto sign = KeeShareSettings::Sign::deserialize(stream.readAll());
        signatureFile.close();

        zip.setCurrentFile(KeeShare::containerFileName());
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
            return {reference.path, ShareObserver::Result::Error, reader.errorString()};
        }

        auto foreign = KeeShare::foreign();
        auto own = KeeShare::own();
        auto trust = check(payload, reference, own.certificate, foreign.certificates, sign);
        switch (trust.first) {
        case Invalid:
            qWarning("Prevent untrusted import");
            return {reference.path, ShareObserver::Result::Error, ShareImport::tr("Untrusted import prevented")};

        case UntrustedForever:
        case TrustedForever: {
            bool found = false;
            const auto trusted =
                trust.first == TrustedForever ? KeeShareSettings::Trust::Trusted : KeeShareSettings::Trust::Untrusted;
            for (KeeShareSettings::ScopedCertificate& scopedCertificate : foreign.certificates) {
                if (scopedCertificate.certificate == trust.second && scopedCertificate.path == reference.path) {
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
                auto changelist = merger.merge();
                if (!changelist.isEmpty()) {
                    return {
                        reference.path, ShareObserver::Result::Success, ShareImport::tr("Successful signed import")};
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
            auto changelist = merger.merge();
            if (!changelist.isEmpty()) {
                return {reference.path, ShareObserver::Result::Success, ShareImport::tr("Successful signed import")};
            }
            return {};
        }
        default:
            qWarning("Prevented untrusted import of signed KeeShare database %s", qPrintable(reference.path));
            return {reference.path, ShareObserver::Result::Warning, ShareImport::tr("Untrusted import prevented")};
        }
#endif
    }

    ShareObserver::Result
    unsignedContainerInto(const QString& resolvedPath, const KeeShareSettings::Reference& reference, Group* targetGroup)
    {
#if !defined(WITH_XC_KEESHARE_INSECURE)
        Q_UNUSED(targetGroup);
        Q_UNUSED(resolvedPath);
        return {reference.path,
                ShareObserver::Result::Warning,
                ShareImport::tr("Unsigned share container are not supported - import prevented")};
#else
        QFile file(resolvedPath);
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical("Unable to open file %s.", qPrintable(reference.path));
            return {reference.path, ShareObserver::Result::Error, ShareImport::tr("File is not readable")};
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
            return {reference.path, ShareObserver::Result::Error, reader.errorString()};
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
                if (scopedCertificate.certificate == trust.second && scopedCertificate.path == reference.path) {
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
                auto changelist = merger.merge();
                if (!changelist.isEmpty()) {
                    return {
                        reference.path, ShareObserver::Result::Success, ShareImport::tr("Successful signed import")};
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
            auto changelist = merger.merge();
            if (!changelist.isEmpty()) {
                return {reference.path, ShareObserver::Result::Success, ShareImport::tr("Successful unsigned import")};
            }
            return {};
        }
        default:
            qWarning("Prevented untrusted import of unsigned KeeShare database %s", qPrintable(reference.path));
            return {reference.path, ShareObserver::Result::Warning, ShareImport::tr("Untrusted import prevented")};
        }
#endif
    }

} // namespace

ShareObserver::Result ShareImport::containerInto(const QString& resolvedPath,
                                                 const KeeShareSettings::Reference& reference,
                                                 Group* targetGroup)
{
    const QFileInfo info(resolvedPath);
    if (!info.exists()) {
        qCritical("File %s does not exist.", qPrintable(info.absoluteFilePath()));
        return {reference.path, ShareObserver::Result::Warning, tr("File does not exist")};
    }

    if (KeeShare::isContainerType(info, KeeShare::signedContainerFileType())) {
        return signedContainerInto(resolvedPath, reference, targetGroup);
    }
    return unsignedContainerInto(resolvedPath, reference, targetGroup);
}
