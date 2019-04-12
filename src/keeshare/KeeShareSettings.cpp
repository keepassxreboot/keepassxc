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

#include "KeeShareSettings.h"
#include "core/CustomData.h"
#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "crypto/ssh/OpenSSHKey.h"
#include "keeshare/Signature.h"

#include <QMessageBox>
#include <QPainter>
#include <QPushButton>

#include <functional>

namespace KeeShareSettings
{
    namespace
    {
        Certificate packCertificate(const OpenSSHKey& key, const QString& signer)
        {
            KeeShareSettings::Certificate extracted;
            extracted.signer = signer;
            Q_ASSERT(key.type() == "ssh-rsa");
            extracted.key = OpenSSHKey::serializeToBinary(OpenSSHKey::Public, key);
            return extracted;
        }

        Key packKey(const OpenSSHKey& key)
        {
            KeeShareSettings::Key extracted;
            Q_ASSERT(key.type() == "ssh-rsa");
            extracted.key = OpenSSHKey::serializeToBinary(OpenSSHKey::Private, key);
            return extracted;
        }

        OpenSSHKey unpackKey(const Key& sign)
        {
            if (sign.key.isEmpty()) {
                return OpenSSHKey();
            }
            OpenSSHKey key = OpenSSHKey::restoreFromBinary(OpenSSHKey::Private, sign.key);
            Q_ASSERT(key.type() == "ssh-rsa");
            return key;
        }

        OpenSSHKey unpackCertificate(const Certificate& certificate)
        {
            if (certificate.key.isEmpty()) {
                return OpenSSHKey();
            }
            OpenSSHKey key = OpenSSHKey::restoreFromBinary(OpenSSHKey::Public, certificate.key);
            Q_ASSERT(key.type() == "ssh-rsa");
            return key;
        }

        QString xmlSerialize(std::function<void(QXmlStreamWriter& writer)> specific)
        {
            QString buffer;
            QXmlStreamWriter writer(&buffer);

            writer.setCodec(QTextCodec::codecForName("UTF-8"));
            writer.setAutoFormatting(true);
            writer.setAutoFormattingIndent(2);

            writer.writeStartDocument();
            writer.writeStartElement("KeeShare");
            writer.writeAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema");
            writer.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
            specific(writer);
            writer.writeEndElement();
            writer.writeEndElement();
            writer.writeEndDocument();
            return buffer;
        }

        void xmlDeserialize(const QString& raw, std::function<void(QXmlStreamReader& reader)> specific)
        {
            QXmlStreamReader reader(raw);
            if (!reader.readNextStartElement() || reader.qualifiedName() != "KeeShare") {
                return;
            }
            specific(reader);
        }
    } // namespace

    void Certificate::serialize(QXmlStreamWriter& writer, const Certificate& certificate)
    {
        if (certificate.isNull()) {
            return;
        }
        writer.writeStartElement("Signer");
        writer.writeCharacters(certificate.signer);
        writer.writeEndElement();
        writer.writeStartElement("Key");
        writer.writeCharacters(certificate.key.toBase64());
        writer.writeEndElement();
    }

    bool Certificate::operator==(const Certificate& other) const
    {
        return key == other.key && signer == other.signer;
    }

    bool Certificate::operator!=(const Certificate& other) const
    {
        return !operator==(other);
    }

    bool Certificate::isNull() const
    {
        return key.isEmpty() && signer.isEmpty();
    }

    QString Certificate::fingerprint() const
    {
        if (isNull()) {
            return {};
        }
        return sshKey().fingerprint();
    }

    OpenSSHKey Certificate::sshKey() const
    {
        return unpackCertificate(*this);
    }

    QString Certificate::publicKey() const
    {
        if (isNull()) {
            return {};
        }
        return sshKey().publicKey();
    }

    Certificate Certificate::deserialize(QXmlStreamReader& reader)
    {
        Certificate certificate;
        while (!reader.error() && reader.readNextStartElement()) {
            if (reader.name() == "Signer") {
                certificate.signer = reader.readElementText();
            } else if (reader.name() == "Key") {
                certificate.key = QByteArray::fromBase64(reader.readElementText().toLatin1());
            }
        }
        return certificate;
    }

    bool Key::operator==(const Key& other) const
    {
        return key == other.key;
    }

    bool Key::operator!=(const Key& other) const
    {
        return !operator==(other);
    }

    bool Key::isNull() const
    {
        return key.isEmpty();
    }

    QString Key::privateKey() const
    {
        if (isNull()) {
            return {};
        }
        return sshKey().privateKey();
    }

    OpenSSHKey Key::sshKey() const
    {
        return unpackKey(*this);
    }

    void Key::serialize(QXmlStreamWriter& writer, const Key& key)
    {
        if (key.isNull()) {
            return;
        }
        writer.writeCharacters(key.key.toBase64());
    }

    Key Key::deserialize(QXmlStreamReader& reader)
    {
        Key key;
        key.key = QByteArray::fromBase64(reader.readElementText().toLatin1());
        return key;
    }

    Own Own::generate()
    {
        OpenSSHKey key = OpenSSHKey::generate(false);
        key.openKey(QString());
        Own own;
        own.key = packKey(key);
        const QString name = qgetenv("USER"); // + "@" + QHostInfo::localHostName();
        own.certificate = packCertificate(key, name);
        return own;
    }

    QString Active::serialize(const Active& active)
    {
        return xmlSerialize([&](QXmlStreamWriter& writer) {
            writer.writeStartElement("Active");
            if (active.in) {
                writer.writeEmptyElement("Import");
            }
            if (active.out) {
                writer.writeEmptyElement("Export");
            }
            writer.writeEndElement();
        });
    }

    Active Active::deserialize(const QString& raw)
    {
        Active active;
        xmlDeserialize(raw, [&](QXmlStreamReader& reader) {
            while (!reader.error() && reader.readNextStartElement()) {
                if (reader.name() == "Active") {
                    while (reader.readNextStartElement()) {
                        if (reader.name() == "Import") {
                            active.in = true;
                            reader.skipCurrentElement();
                        } else if (reader.name() == "Export") {
                            active.out = true;
                            reader.skipCurrentElement();
                        } else {
                            break;
                        }
                    }
                } else {
                    ::qWarning() << "Unknown KeeShareSettings element" << reader.name();
                    reader.skipCurrentElement();
                }
            }
        });
        return active;
    }

    bool Own::operator==(const Own& other) const
    {
        return key == other.key && certificate == other.certificate;
    }

    bool Own::operator!=(const Own& other) const
    {
        return !operator==(other);
    }

    QString Own::serialize(const Own& own)
    {
        return xmlSerialize([&](QXmlStreamWriter& writer) {
            writer.writeStartElement("PrivateKey");
            Key::serialize(writer, own.key);
            writer.writeEndElement();
            writer.writeStartElement("PublicKey");
            Certificate::serialize(writer, own.certificate);
            writer.writeEndElement();
        });
    }

    Own Own::deserialize(const QString& raw)
    {
        Own own;
        xmlDeserialize(raw, [&](QXmlStreamReader& reader) {
            while (!reader.error() && reader.readNextStartElement()) {
                if (reader.name() == "PrivateKey") {
                    own.key = Key::deserialize(reader);
                } else if (reader.name() == "PublicKey") {
                    own.certificate = Certificate::deserialize(reader);
                } else {
                    ::qWarning() << "Unknown KeeShareSettings element" << reader.name();
                    reader.skipCurrentElement();
                }
            }
        });
        return own;
    }

    bool ScopedCertificate::operator==(const ScopedCertificate& other) const
    {
        return trust == other.trust && path == other.path && certificate == other.certificate;
    }

    bool ScopedCertificate::operator!=(const ScopedCertificate& other) const
    {
        return !operator==(other);
    }

    void ScopedCertificate::serialize(QXmlStreamWriter& writer, const ScopedCertificate& scopedCertificate)
    {
        writer.writeAttribute("Path", scopedCertificate.path);
        QString trust = "Ask";
        if (scopedCertificate.trust == KeeShareSettings::Trust::Trusted) {
            trust = "Trusted";
        }
        if (scopedCertificate.trust == KeeShareSettings::Trust::Untrusted) {
            trust = "Untrusted";
        }
        writer.writeAttribute("Trust", trust);
        Certificate::serialize(writer, scopedCertificate.certificate);
    }

    ScopedCertificate ScopedCertificate::deserialize(QXmlStreamReader& reader)
    {
        ScopedCertificate scopedCertificate;
        scopedCertificate.path = reader.attributes().value("Path").toString();
        scopedCertificate.trust = KeeShareSettings::Trust::Ask;
        auto trust = reader.attributes().value("Trust").toString();
        if (trust.compare("Trusted", Qt::CaseInsensitive) == 0) {
            scopedCertificate.trust = KeeShareSettings::Trust::Trusted;
        }
        if (trust.compare("Untrusted", Qt::CaseInsensitive) == 0) {
            scopedCertificate.trust = KeeShareSettings::Trust::Untrusted;
        }
        scopedCertificate.certificate = Certificate::deserialize(reader);
        return scopedCertificate;
    }

    QString Foreign::serialize(const Foreign& foreign)
    {
        return xmlSerialize([&](QXmlStreamWriter& writer) {
            writer.writeStartElement("Foreign");
            for (const ScopedCertificate& scopedCertificate : foreign.certificates) {
                writer.writeStartElement("Certificate");
                ScopedCertificate::serialize(writer, scopedCertificate);
                writer.writeEndElement();
            }
            writer.writeEndElement();
        });
    }

    Foreign Foreign::deserialize(const QString& raw)
    {
        Foreign foreign;
        xmlDeserialize(raw, [&](QXmlStreamReader& reader) {
            while (!reader.error() && reader.readNextStartElement()) {
                if (reader.name() == "Foreign") {
                    while (!reader.error() && reader.readNextStartElement()) {
                        if (reader.name() == "Certificate") {
                            foreign.certificates << ScopedCertificate::deserialize(reader);
                        } else {
                            ::qWarning() << "Unknown Cerificates element" << reader.name();
                            reader.skipCurrentElement();
                        }
                    }
                } else {
                    ::qWarning() << "Unknown KeeShareSettings element" << reader.name();
                    reader.skipCurrentElement();
                }
            }
        });
        return foreign;
    }

    Reference::Reference()
        : type(Inactive)
        , uuid(QUuid::createUuid())
    {
    }

    bool Reference::isNull() const
    {
        return type == Inactive && path.isEmpty() && password.isEmpty();
    }

    bool Reference::isValid() const
    {
        return type != Inactive && !path.isEmpty();
    }

    bool Reference::isExporting() const
    {
        return (type & ExportTo) != 0 && !path.isEmpty();
    }

    bool Reference::isImporting() const
    {
        return (type & ImportFrom) != 0 && !path.isEmpty();
    }

    bool Reference::operator<(const Reference& other) const
    {
        if (type != other.type) {
            return type < other.type;
        }
        return path < other.path;
    }

    bool Reference::operator==(const Reference& other) const
    {
        return path == other.path && uuid == other.uuid && password == other.password && type == other.type;
    }

    QString Reference::serialize(const Reference& reference)
    {
        return xmlSerialize([&](QXmlStreamWriter& writer) {
            writer.writeStartElement("Type");
            if ((reference.type & ImportFrom) == ImportFrom) {
                writer.writeEmptyElement("Import");
            }
            if ((reference.type & ExportTo) == ExportTo) {
                writer.writeEmptyElement("Export");
            }
            writer.writeEndElement();
            writer.writeStartElement("Group");
            writer.writeCharacters(reference.uuid.toRfc4122().toBase64());
            writer.writeEndElement();
            writer.writeStartElement("Path");
            writer.writeCharacters(reference.path.toUtf8().toBase64());
            writer.writeEndElement();
            writer.writeStartElement("Password");
            writer.writeCharacters(reference.password.toUtf8().toBase64());
            writer.writeEndElement();
        });
    }

    Reference Reference::deserialize(const QString& raw)
    {
        Reference reference;
        xmlDeserialize(raw, [&](QXmlStreamReader& reader) {
            while (!reader.error() && reader.readNextStartElement()) {
                if (reader.name() == "Type") {
                    while (reader.readNextStartElement()) {
                        if (reader.name() == "Import") {
                            reference.type |= ImportFrom;
                            reader.skipCurrentElement();
                        } else if (reader.name() == "Export") {
                            reference.type |= ExportTo;
                            reader.skipCurrentElement();
                        } else {
                            break;
                        }
                    }
                } else if (reader.name() == "Group") {
                    reference.uuid = QUuid::fromRfc4122(QByteArray::fromBase64(reader.readElementText().toLatin1()));
                } else if (reader.name() == "Path") {
                    reference.path = QString::fromUtf8(QByteArray::fromBase64(reader.readElementText().toLatin1()));
                } else if (reader.name() == "Password") {
                    reference.password = QString::fromUtf8(QByteArray::fromBase64(reader.readElementText().toLatin1()));
                } else {
                    ::qWarning() << "Unknown Reference element" << reader.name();
                    reader.skipCurrentElement();
                }
            }
        });
        return reference;
    }

    QString Sign::serialize(const Sign& sign)
    {
        return xmlSerialize([&](QXmlStreamWriter& writer) {
            writer.writeStartElement("Signature");
            writer.writeCharacters(sign.signature);
            writer.writeEndElement();
            writer.writeStartElement("Certificate");
            Certificate::serialize(writer, sign.certificate);
            writer.writeEndElement();
        });
    }

    Sign Sign::deserialize(const QString& raw)
    {
        Sign sign;
        xmlDeserialize(raw, [&](QXmlStreamReader& reader) {
            while (!reader.error() && reader.readNextStartElement()) {
                if (reader.name() == "Signature") {
                    sign.signature = reader.readElementText();
                } else if (reader.name() == "Certificate") {
                    sign.certificate = KeeShareSettings::Certificate::deserialize(reader);
                } else {
                    ::qWarning() << "Unknown Sign element" << reader.name();
                    reader.skipCurrentElement();
                }
            }
        });
        return sign;
    }
} // namespace KeeShareSettings
