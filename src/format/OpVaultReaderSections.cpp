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

#include "OpData01.h"
#include "OpVaultReader.h"

#include "core/Group.h"
#include "core/Tools.h"
#include "crypto/CryptoHash.h"
#include "crypto/SymmetricCipher.h"
#include "totp/totp.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QUuid>

namespace
{
    QDateTime resolveDate(const QString& kind, const QJsonValue& value)
    {
        QDateTime date;
        if (kind == "monthYear") {
            // 1Password programmers are sadistic...
            auto dateValue = QString::number(value.toInt());
            date = QDateTime::fromString(dateValue, "yyyyMM");
            date.setTimeSpec(Qt::UTC);
        } else if (value.isString()) {
            date = QDateTime::fromTime_t(value.toString().toUInt(), Qt::UTC);
        } else {
            date = QDateTime::fromTime_t(value.toInt(), Qt::UTC);
        }
        return date;
    }
} // namespace

void OpVaultReader::fillFromSection(Entry* entry, const QJsonObject& section)
{
    const auto uuid = entry->uuid();
    QString sectionName = section["name"].toString();

    if (!section.contains("fields")) {
        auto sectionNameLC = sectionName.toLower();
        auto sectionTitleLC = section["title"].toString("").toLower();
        if (!(sectionNameLC == "linked items" && sectionTitleLC == "related items")) {
            qWarning() << R"(Skipping "fields"-less Section in UUID ")" << uuid << "\": <<" << section << ">>";
        }
        return;
    } else if (!section["fields"].isArray()) {
        qWarning() << R"(Skipping non-Array "fields" in UUID ")" << uuid << "\"\n";
        return;
    }

    // If we have a default section name then replace with the section title if not empty
    if (sectionName.startsWith("Section_") && !section["title"].toString().isEmpty()) {
        sectionName = section["title"].toString();
    }

    QJsonArray sectionFields = section["fields"].toArray();
    for (const QJsonValue sectionField : sectionFields) {
        if (!sectionField.isObject()) {
            qWarning() << R"(Skipping non-Object "fields" in UUID ")" << uuid << "\": << " << sectionField << ">>";
            continue;
        }
        QJsonObject field = sectionField.toObject();
        fillFromSectionField(entry, sectionName, field);
    }
}

void OpVaultReader::fillFromSectionField(Entry* entry, const QString& sectionName, QJsonObject& field)
{
    if (!field.contains("v")) {
        // for our purposes, we don't care if there isn't a value in the field
        return;
    }

    // Ignore "a" and "inputTraits" fields, they don't apply to KPXC

    auto attrName = resolveAttributeName(sectionName, field["n"].toString(), field["t"].toString());
    auto attrValue = field.value("v").toString();
    auto kind = field["k"].toString();

    if (attrName.startsWith("TOTP_")) {
        if (attrValue.startsWith("otpauth://")) {
            QUrlQuery query(attrValue);
            // at least as of 1Password 7, they don't append the digits= and period= which totp.cpp requires
            if (!query.hasQueryItem("digits")) {
                query.addQueryItem("digits", QString("%1").arg(Totp::DEFAULT_DIGITS));
            }
            if (!query.hasQueryItem("period")) {
                query.addQueryItem("period", QString("%1").arg(Totp::DEFAULT_STEP));
            }
            attrValue = query.toString(QUrl::FullyEncoded);
            entry->setTotp(Totp::parseSettings(attrValue));
        } else {
            entry->setTotp(Totp::parseSettings({}, attrValue));
        }

    } else if (attrName.startsWith("expir", Qt::CaseInsensitive)) {
        QDateTime expiry = resolveDate(kind, field.value("v"));
        if (expiry.isValid()) {
            entry->setExpiryTime(expiry);
            entry->setExpires(true);
        } else {
            qWarning() << QString("[%1] Invalid expiration date found: %2").arg(entry->title(), attrValue);
        }
    } else {
        if (kind == "date" || kind == "monthYear") {
            QDateTime date = resolveDate(kind, field.value("v"));
            if (date.isValid()) {
                entry->attributes()->set(attrName, date.toString(Qt::SystemLocaleShortDate));
            } else {
                qWarning()
                    << QString("[%1] Invalid date attribute found: %2 = %3").arg(entry->title(), attrName, attrValue);
            }
        } else if (kind == "address") {
            // Expand address into multiple attributes
            auto addrFields = field.value("v").toObject().toVariantMap();
            for (auto part : addrFields.keys()) {
                entry->attributes()->set(attrName + QString("_%1").arg(part), addrFields.value(part).toString());
            }
        } else {
            entry->attributes()->set(attrName, attrValue, (kind == "password" || kind == "concealed"));
        }
    }
}

QString OpVaultReader::resolveAttributeName(const QString& section, const QString& name, const QString& text)
{
    // Special case for TOTP
    if (name.startsWith("TOTP_")) {
        return name;
    }

    auto lowName = name.toLower();
    auto lowText = text.toLower();
    if (section.isEmpty() || name.startsWith("address")) {
        // Empty section implies these are core attributes
        // try to find username, password, url
        if (lowName == "password" || lowText == "password") {
            return EntryAttributes::PasswordKey;
        } else if (lowName == "username" || lowText == "username") {
            return EntryAttributes::UserNameKey;
        } else if (lowName == "url" || lowText == "url" || lowName == "hostname" || lowText == "server"
                   || lowName == "website") {
            return EntryAttributes::URLKey;
        }
        return name;
    }

    return QString("%1_%2").arg(section, name);
}
