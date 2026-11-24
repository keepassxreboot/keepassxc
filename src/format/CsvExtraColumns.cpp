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

#include "CsvExtraColumns.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

#include "core/AutoTypeAssociations.h"
#include "core/Clock.h"
#include "core/Entry.h"
#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"
#include "core/Tools.h"

namespace
{
    // The headings, as the export writes them
    const char* const ColExpires = "Expires";
    const char* const ColExpiryTime = "Expiry Time";
    const char* const ColLastAccessed = "Last Accessed";
    const char* const ColLocationChanged = "Location Changed";
    const char* const ColUsageCount = "Usage Count";
    const char* const ColUuid = "UUID";
    const char* const ColCustomAttributes = "Custom Attributes";
    const char* const ColAutoType = "Auto-Type";
    const char* const ColAttachments = "Attachments";
    const char* const ColHistory = "History";
    const char* const ColProtectedAttributes = "Protected Attributes";
    const char* const ColOverrideUrl = "Override URL";
    const char* const ColForegroundColor = "Foreground Color";
    const char* const ColBackgroundColor = "Background Color";
    const char* const ColExcludeFromReports = "Exclude From Reports";
    const char* const ColCustomData = "Custom Data";
    const char* const ColExactText = "Exact Text";

    // A time as the export writes it (ISO 8601), or seconds / milliseconds since 1970 as other programs write it
    bool parseTime(const QString& text, QDateTime& time)
    {
        const auto trimmed = text.trimmed();
        if (trimmed.contains(QRegularExpression("^\\d+$"))) {
            auto t = trimmed.toLongLong();
            if (t <= INT32_MAX) {
                t *= 1000;
            }
            time = Clock::datetimeUtc(t);
            return true;
        }
        time = QDateTime::fromString(trimmed, Qt::ISODate);
        if (!time.isValid()) {
            return false;
        }
        time = time.toUTC();
        return true;
    }

    // true and the object, or false when the text is not a JSON object
    bool parseObject(const QString& text, QJsonObject& object)
    {
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(text.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) {
            return false;
        }
        object = document.object();
        return true;
    }

    bool parseArray(const QString& text, QJsonArray& array)
    {
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(text.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError || !document.isArray()) {
            return false;
        }
        array = document.array();
        return true;
    }

    // The text of a JSON value: a string as it is, anything else as JSON text
    QString valueText(const QJsonValue& value)
    {
        if (value.isString()) {
            return value.toString();
        }
        if (value.isObject()) {
            return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
        }
        if (value.isArray()) {
            return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
        }
        return value.toVariant().toString();
    }

    // Keeps the text of a cell that could not be understood, so that nothing is lost
    void keepRaw(Entry* entry, const QString& heading, const QString& text)
    {
        CsvExtraColumns::addCustomField(entry, heading + " (CSV text that was not understood)", text, false);
    }

    // The names of the hidden custom fields: a JSON array of names
    QSet<QString> hiddenNames(const QString& text, bool* understood)
    {
        QSet<QString> names;
        *understood = true;
        if (text.trimmed().isEmpty()) {
            return names;
        }
        QJsonArray array;
        if (!parseArray(text, array)) {
            *understood = false;
            return names;
        }
        for (const auto& item : array) {
            names.insert(valueText(item));
        }
        return names;
    }

    // Custom fields from a JSON object, as in the "Custom Attributes" column and in a history entry. Returns false when
    // the text is not a JSON object.
    //
    // overwritable: fields the entry already has that come from another column (the TOTP column creates the "otp"
    // field), which the exact text of this column replaces. flagsKnown: the file says which fields are hidden; if not,
    // the hidden flag a field already has is kept.
    bool addCustomFields(Entry* entry,
                         const QString& text,
                         const QSet<QString>& hidden,
                         const QSet<QString>& overwritable,
                         bool flagsKnown,
                         const QString& rowLabel,
                         QStringList& problems)
    {
        if (text.trimmed().isEmpty()) {
            return true;
        }
        QJsonObject object;
        if (!parseObject(text, object)) {
            return false;
        }
        for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
            if (overwritable.contains(it.key())) {
                const bool isHidden =
                    hidden.contains(it.key()) || (!flagsKnown && entry->attributes()->isProtected(it.key()));
                entry->attributes()->set(it.key(), valueText(it.value()), isHidden);
                continue;
            }
            const auto used =
                CsvExtraColumns::addCustomField(entry, it.key(), valueText(it.value()), hidden.contains(it.key()));
            if (used != it.key()) {
                problems << QString("%1: the custom field \"%2\" has the name of a standard field or of another field; "
                                    "it was kept as \"%3\"")
                                .arg(rowLabel, it.key(), used);
            }
        }
        return true;
    }

    void applyHistory(Entry* entry,
                      const TimeInfo& timeInfo,
                      const QString& text,
                      const QString& rowLabel,
                      QStringList& problems)
    {
        QJsonArray snapshots;
        if (!parseArray(text, snapshots)) {
            problems << QString("%1: the history could not be read and was kept as a custom field").arg(rowLabel);
            keepRaw(entry, ColHistory, text);
            return;
        }

        int number = 0;
        for (const auto& snapshotValue : snapshots) {
            ++number;
            if (!snapshotValue.isObject()) {
                problems
                    << QString("%1: older version %2 could not be read and was not imported").arg(rowLabel).arg(number);
                continue;
            }
            const auto snapshot = snapshotValue.toObject();

            // An older version is an entry of its own with the id of the entry it belongs to
            auto* item = new Entry();
            item->setUuid(entry->uuid());
            item->setTitle(valueText(snapshot.value("title")));
            item->setUsername(valueText(snapshot.value("username")));
            item->setPassword(valueText(snapshot.value("password")));
            item->setUrl(valueText(snapshot.value("url")));
            item->setNotes(valueText(snapshot.value("notes")));

            bool understood = true;
            const auto hidden = hiddenNames(valueText(snapshot.value("protectedAttributes")), &understood);
            if (snapshot.contains("attributes")) {
                // The custom fields are a JSON object inside the snapshot (not text)
                const auto attributes = snapshot.value("attributes");
                const auto attributesText = valueText(attributes);
                if (!attributes.isObject()
                    || !addCustomFields(item, attributesText, hidden, {}, true, rowLabel, problems)) {
                    problems << QString("%1: the custom fields of older version %2 could not be read")
                                    .arg(rowLabel)
                                    .arg(number);
                }
            }

            TimeInfo times;
            times.setCreationTime(timeInfo.creationTime());
            QDateTime modified;
            if (!parseTime(valueText(snapshot.value("modified")), modified)) {
                modified = timeInfo.lastModificationTime();
                problems << QString("%1: older version %2 has no valid date; the date of the entry was used")
                                .arg(rowLabel)
                                .arg(number);
            }
            times.setLastModificationTime(modified);
            times.setLastAccessTime(modified);
            item->setTimeInfo(times);

            entry->addHistoryItem(item);
        }
    }
} // namespace

const QStringList& CsvExtraColumns::names()
{
    static const QStringList list{ColExpires,
                                  ColExpiryTime,
                                  ColLastAccessed,
                                  ColLocationChanged,
                                  ColUsageCount,
                                  ColUuid,
                                  ColCustomAttributes,
                                  ColAutoType,
                                  ColAttachments,
                                  ColHistory,
                                  ColProtectedAttributes,
                                  ColOverrideUrl,
                                  ColForegroundColor,
                                  ColBackgroundColor,
                                  ColExcludeFromReports,
                                  ColCustomData,
                                  ColExactText};
    return list;
}

bool CsvExtraColumns::exactTexts(const QString& cell, QHash<QString, QString>& texts)
{
    if (cell.trimmed().isEmpty()) {
        return true;
    }
    QJsonObject object;
    if (!parseObject(cell, object)) {
        return false;
    }
    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        texts.insert(it.key(), valueText(it.value()));
    }
    return true;
}

QString CsvExtraColumns::addCustomField(Entry* entry, const QString& name, const QString& value, bool hidden)
{
    QString key = name;
    if (EntryAttributes::isDefaultAttribute(key) || entry->attributes()->hasKey(key)) {
        key = name + " (CSV)";
        for (int n = 2; entry->attributes()->hasKey(key); ++n) {
            key = QString("%1 (CSV %2)").arg(name).arg(n);
        }
    }
    entry->attributes()->set(key, value, hidden);
    return key;
}

void CsvExtraColumns::apply(Entry* entry,
                            TimeInfo& timeInfo,
                            const QHash<QString, QString>& cells,
                            QSet<QUuid>& usedIds,
                            const QString& rowLabel,
                            QStringList& problems)
{
    // Cells whose exact text is in the "Exact Text" column (the plain cell holds a replacement for half an emoji)
    QHash<QString, QString> exact;
    if (!exactTexts(cells.value(QLatin1String(ColExactText)), exact)) {
        problems << QString("%1: the exact text of some cells could not be read and was kept as a custom field")
                        .arg(rowLabel);
        keepRaw(entry, ColExactText, cells.value(QLatin1String(ColExactText)));
        exact.clear();
    }
    auto cell = [&cells, &exact](const char* heading) {
        return exact.contains(QLatin1String(heading)) ? exact.value(QLatin1String(heading))
                                                      : cells.value(QLatin1String(heading));
    };
    auto hasCell = [&cell](const char* heading) { return !cell(heading).trimmed().isEmpty(); };

    // The entry's id: kept when it is valid and not used by another row of the file
    if (hasCell(ColUuid)) {
        const auto text = cell(ColUuid).trimmed();
        const auto id = Tools::isValidUuid(text) ? Tools::hexToUuid(text) : QUuid();
        if (id.isNull()) {
            problems << QString("%1: the entry id \"%2\" is not valid; a new id was given").arg(rowLabel, text);
        } else if (usedIds.contains(id)) {
            problems
                << QString("%1: the entry id appears more than once in the file; a new id was given").arg(rowLabel);
        } else {
            entry->setUuid(id);
        }
    }
    usedIds.insert(entry->uuid());

    // Dates and counters
    auto time = [&](const char* heading, auto setter) {
        if (!hasCell(heading)) {
            return;
        }
        QDateTime parsed;
        if (parseTime(cell(heading), parsed)) {
            setter(parsed);
        } else {
            problems << QString("%1: \"%2\" is not a valid date (%3)").arg(rowLabel, cell(heading), heading);
        }
    };
    time(ColExpiryTime, [&timeInfo](const QDateTime& t) { timeInfo.setExpiryTime(t); });
    time(ColLastAccessed, [&timeInfo](const QDateTime& t) { timeInfo.setLastAccessTime(t); });
    time(ColLocationChanged, [&timeInfo](const QDateTime& t) { timeInfo.setLocationChanged(t); });

    auto flag = [&](const char* heading, auto setter) {
        if (!hasCell(heading)) {
            return;
        }
        const auto text = cell(heading).trimmed();
        if (text.compare("true", Qt::CaseInsensitive) == 0) {
            setter(true);
        } else if (text.compare("false", Qt::CaseInsensitive) == 0) {
            setter(false);
        } else {
            problems << QString("%1: \"%2\" is not true or false (%3)").arg(rowLabel, text, heading);
        }
    };
    flag(ColExpires, [&timeInfo](bool b) { timeInfo.setExpires(b); });
    flag(ColExcludeFromReports, [entry](bool b) { entry->setExcludeFromReports(b); });

    if (hasCell(ColUsageCount)) {
        bool ok = false;
        const int count = cell(ColUsageCount).trimmed().toInt(&ok);
        if (ok) {
            timeInfo.setUsageCount(count);
        } else {
            problems << QString("%1: \"%2\" is not a number (%3)").arg(rowLabel, cell(ColUsageCount), ColUsageCount);
        }
    }

    // Properties
    if (hasCell(ColOverrideUrl)) {
        entry->setOverrideUrl(cell(ColOverrideUrl));
    }
    if (hasCell(ColForegroundColor)) {
        entry->setForegroundColor(cell(ColForegroundColor).trimmed());
    }
    if (hasCell(ColBackgroundColor)) {
        entry->setBackgroundColor(cell(ColBackgroundColor).trimmed());
    }

    // Custom fields, and which of them are hidden
    bool understood = true;
    const auto hidden = hiddenNames(cell(ColProtectedAttributes), &understood);
    if (!understood) {
        problems << QString("%1: the list of hidden custom fields could not be read; none were hidden").arg(rowLabel);
        keepRaw(entry, ColProtectedAttributes, cell(ColProtectedAttributes));
    }
    const auto existingFields = entry->attributes()->customKeys(); // made by the TOTP column
    const QSet<QString> overwritable(existingFields.cbegin(), existingFields.cend());
    const bool flagsKnown = cells.contains(QLatin1String(ColProtectedAttributes));
    if (!addCustomFields(entry, cell(ColCustomAttributes), hidden, overwritable, flagsKnown, rowLabel, problems)) {
        problems << QString("%1: the custom fields could not be read and were kept as one custom field").arg(rowLabel);
        keepRaw(entry, ColCustomAttributes, cell(ColCustomAttributes));
    }

    // Auto-Type
    if (hasCell(ColAutoType)) {
        QJsonObject settings;
        if (parseObject(cell(ColAutoType), settings)) {
            if (settings.contains("enabled")) {
                entry->setAutoTypeEnabled(settings.value("enabled").toBool(true));
            }
            if (settings.contains("obfuscation")) {
                entry->setAutoTypeObfuscation(settings.value("obfuscation").toInt(0));
            }
            if (settings.contains("defaultSequence")) {
                entry->setDefaultAutoTypeSequence(valueText(settings.value("defaultSequence")));
            }
            for (const auto& association : settings.value("associations").toArray()) {
                const auto object = association.toObject();
                entry->autoTypeAssociations()->add(
                    {valueText(object.value("window")), valueText(object.value("sequence"))});
            }
        } else {
            problems << QString("%1: the auto-type settings could not be read and were kept as a custom field")
                            .arg(rowLabel);
            keepRaw(entry, ColAutoType, cell(ColAutoType));
        }
    }

    // Attachments: name -> base64 text
    if (hasCell(ColAttachments)) {
        QJsonObject attachments;
        if (parseObject(cell(ColAttachments), attachments)) {
            for (auto it = attachments.constBegin(); it != attachments.constEnd(); ++it) {
                const auto decoded = QByteArray::fromBase64Encoding(valueText(it.value()).trimmed().toLatin1(),
                                                                    QByteArray::Base64Encoding
                                                                        | QByteArray::AbortOnBase64DecodingErrors);
                if (decoded) {
                    entry->attachments()->set(it.key(), decoded.decoded);
                } else {
                    problems << QString("%1: the attachment \"%2\" is not valid base64 and was kept as a custom field")
                                    .arg(rowLabel, it.key());
                    CsvExtraColumns::addCustomField(entry,
                                                    "Attachment " + it.key() + " (CSV text that was not understood)",
                                                    valueText(it.value()),
                                                    false);
                }
            }
        } else {
            problems << QString("%1: the attachments could not be read and were kept as a custom field").arg(rowLabel);
            keepRaw(entry, ColAttachments, cell(ColAttachments));
        }
    }

    // Data of the browser integration and other programs
    if (hasCell(ColCustomData)) {
        QJsonObject data;
        if (parseObject(cell(ColCustomData), data)) {
            for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
                entry->customData()->set(it.key(), valueText(it.value()));
            }
        } else {
            problems << QString("%1: the custom data could not be read and was kept as a custom field").arg(rowLabel);
            keepRaw(entry, ColCustomData, cell(ColCustomData));
        }
    }

    // History
    if (hasCell(ColHistory)) {
        applyHistory(entry, timeInfo, cell(ColHistory), rowLabel, problems);
    }
}
