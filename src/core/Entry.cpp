/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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
#include "Entry.h"

#include "config-keepassx.h"

#include "core/Database.h"
#include "core/DatabaseIcons.h"
#include "core/Group.h"
#include "core/Metadata.h"
#include "totp/totp.h"

#include <QRegularExpression>

const int Entry::DefaultIconNumber = 0;
const int Entry::ResolveMaximumDepth = 10;
const QString Entry::AutoTypeSequenceUsername = "{USERNAME}{ENTER}";
const QString Entry::AutoTypeSequencePassword = "{PASSWORD}{ENTER}";


Entry::Entry()
    : m_attributes(new EntryAttributes(this))
    , m_attachments(new EntryAttachments(this))
    , m_autoTypeAssociations(new AutoTypeAssociations(this))
    , m_customData(new CustomData(this))
    , m_tmpHistoryItem(nullptr)
    , m_modifiedSinceBegin(false)
    , m_updateTimeinfo(true)
{
    m_data.iconNumber = DefaultIconNumber;
    m_data.autoTypeEnabled = true;
    m_data.autoTypeObfuscation = 0;
    m_data.totpStep = Totp::defaultStep;
    m_data.totpDigits = Totp::defaultDigits;

    connect(m_attributes, SIGNAL(modified()), SLOT(updateTotp()));
    connect(m_attributes, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(m_attributes, SIGNAL(defaultKeyModified()), SLOT(emitDataChanged()));
    connect(m_attachments, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(m_autoTypeAssociations, SIGNAL(modified()), SIGNAL(modified()));
    connect(m_customData, SIGNAL(modified()), this, SIGNAL(modified()));

    connect(this, SIGNAL(modified()), SLOT(updateTimeinfo()));
    connect(this, SIGNAL(modified()), SLOT(updateModifiedSinceBegin()));
}

Entry::~Entry()
{
    if (m_group) {
        m_group->removeEntry(this);

        if (m_group->database()) {
            m_group->database()->addDeletedObject(m_uuid);
        }
    }

    qDeleteAll(m_history);
}

template <class T> inline bool Entry::set(T& property, const T& value)
{
    if (property != value) {
        property = value;
        emit modified();
        return true;
    }
    else {
        return false;
    }
}

void Entry::updateTimeinfo()
{
    if (m_updateTimeinfo) {
        m_data.timeInfo.setLastModificationTime(QDateTime::currentDateTimeUtc());
        m_data.timeInfo.setLastAccessTime(QDateTime::currentDateTimeUtc());
    }
}

void Entry::setUpdateTimeinfo(bool value)
{
    m_updateTimeinfo = value;
}

EntryReferenceType Entry::referenceType(const QString& referenceStr)
{
    const QString referenceLowerStr = referenceStr.toLower();
    EntryReferenceType result = EntryReferenceType::Unknown;
    if (referenceLowerStr == QLatin1String("t")) {
        result = EntryReferenceType::Title;
    } else if (referenceLowerStr == QLatin1String("u")) {
        result = EntryReferenceType::UserName;
    } else if (referenceLowerStr == QLatin1String("p")) {
        result = EntryReferenceType::Password;
    } else if (referenceLowerStr == QLatin1String("a")) {
        result = EntryReferenceType::Url;
    } else if (referenceLowerStr == QLatin1String("n")) {
        result = EntryReferenceType::Notes;
    } else if (referenceLowerStr == QLatin1String("i")) {
        result = EntryReferenceType::Uuid;
    } else if (referenceLowerStr == QLatin1String("o")) {
        result = EntryReferenceType::CustomAttributes;
    }

    return result;
}

Uuid Entry::uuid() const
{
    return m_uuid;
}

QImage Entry::icon() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->icon(m_data.iconNumber);
    }
    else {
        Q_ASSERT(database());

        if (database()) {
            return database()->metadata()->customIcon(m_data.customIcon);
        }
        else {
            return QImage();
        }
    }
}

QPixmap Entry::iconPixmap() const
{
    if (m_data.customIcon.isNull()) {
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }
    else {
        Q_ASSERT(database());

        if (database()) {
            return database()->metadata()->customIconPixmap(m_data.customIcon);
        }
        else {
            return QPixmap();
        }
    }
}

QPixmap Entry::iconScaledPixmap() const
{
    if (m_data.customIcon.isNull()) {
        // built-in icons are 16x16 so don't need to be scaled
        return databaseIcons()->iconPixmap(m_data.iconNumber);
    }
    else {
        Q_ASSERT(database());

        return database()->metadata()->customIconScaledPixmap(m_data.customIcon);
    }
}

int Entry::iconNumber() const
{
    return m_data.iconNumber;
}

Uuid Entry::iconUuid() const
{
    return m_data.customIcon;
}

QColor Entry::foregroundColor() const
{
    return m_data.foregroundColor;
}

QColor Entry::backgroundColor() const
{
    return m_data.backgroundColor;
}

QString Entry::overrideUrl() const
{
    return m_data.overrideUrl;
}

QString Entry::tags() const
{
    return m_data.tags;
}

TimeInfo Entry::timeInfo() const
{
    return m_data.timeInfo;
}

bool Entry::autoTypeEnabled() const
{
    return m_data.autoTypeEnabled;
}

int Entry::autoTypeObfuscation() const
{
    return m_data.autoTypeObfuscation;
}

QString Entry::defaultAutoTypeSequence() const
{
    return m_data.defaultAutoTypeSequence;
}

/**
 * Determine the effective sequence that will be injected
 * This function return an empty string if a parent group has autotype disabled or if the entry has no parent
 */
QString Entry::effectiveAutoTypeSequence() const
{
    if (!autoTypeEnabled()) {
        return {};
    }

    const Group* parent = group();
    if (!parent) {
        return {};
    }

    QString sequence = parent->effectiveAutoTypeSequence();
    if (sequence.isEmpty()) {
        return {};
    }

    if (!m_data.defaultAutoTypeSequence.isEmpty()) {
        return m_data.defaultAutoTypeSequence;
    }

    if (sequence == Group::RootAutoTypeSequence && (!username().isEmpty() || !password().isEmpty())) {
        if (username().isEmpty()) {
            return AutoTypeSequencePassword;
        } else if (password().isEmpty()) {
            return AutoTypeSequenceUsername;
        }
        return Group::RootAutoTypeSequence;
    }

    return sequence;
}

AutoTypeAssociations* Entry::autoTypeAssociations()
{
    return m_autoTypeAssociations;
}

const AutoTypeAssociations* Entry::autoTypeAssociations() const
{
    return m_autoTypeAssociations;
}

QString Entry::title() const
{
    return m_attributes->value(EntryAttributes::TitleKey);
}

QString Entry::url() const
{
    return m_attributes->value(EntryAttributes::URLKey);
}

QString Entry::webUrl() const
{
    QString url = resolveMultiplePlaceholders(m_attributes->value(EntryAttributes::URLKey));
    return resolveUrl(url);
}

QString Entry::displayUrl() const
{
    QString url = maskPasswordPlaceholders(m_attributes->value(EntryAttributes::URLKey));
    return resolveMultiplePlaceholders(url);
}

QString Entry::username() const
{
    return m_attributes->value(EntryAttributes::UserNameKey);
}

QString Entry::password() const
{
    return m_attributes->value(EntryAttributes::PasswordKey);
}

QString Entry::notes() const
{
    return m_attributes->value(EntryAttributes::NotesKey);
}

bool Entry::isExpired() const
{
    return m_data.timeInfo.expires() && m_data.timeInfo.expiryTime() < QDateTime::currentDateTimeUtc();
}

bool Entry::hasReferences() const
{
    const QList<QString> keyList = EntryAttributes::DefaultAttributes;
    for (const QString& key : keyList) {
        if (m_attributes->isReference(key)) {
            return true;
        }
    }
    return false;
}

EntryAttributes* Entry::attributes()
{
    return m_attributes;
}

const EntryAttributes* Entry::attributes() const
{
    return m_attributes;
}

EntryAttachments* Entry::attachments()
{
    return m_attachments;
}

const EntryAttachments* Entry::attachments() const
{
    return m_attachments;
}

CustomData* Entry::customData()
{
    return m_customData;
}

const CustomData* Entry::customData() const
{
    return m_customData;
}

bool Entry::hasTotp() const
{
    return m_attributes->hasKey("TOTP Seed") || m_attributes->hasKey("otp");
}

QString Entry::totp() const
{
    if (hasTotp()) {
        QString seed = totpSeed();
        quint64 time = QDateTime::currentDateTime().toTime_t();
        QString output = Totp::generateTotp(seed.toLatin1(), time, m_data.totpDigits, m_data.totpStep);

        return QString(output);
    }
    return {};
}

void Entry::setTotp(const QString& seed, quint8& step, quint8& digits)
{
    beginUpdate();
    if (step == 0) {
        step = Totp::defaultStep;
    }

    if (digits == 0) {
        digits = Totp::defaultDigits;
    }
    QString data;

    const Totp::Encoder & enc = Totp::encoders.value(digits, Totp::defaultEncoder);

    if (m_attributes->hasKey("otp")) {
        data = QString("key=%1&step=%2&size=%3").arg(seed).arg(step).arg(enc.digits == 0 ? digits : enc.digits);
        if (!enc.name.isEmpty()) {
            data.append("&enocder=").append(enc.name);
        }
        m_attributes->set("otp", data, true);
    } else {
        m_attributes->set("TOTP Seed", seed, true);
        if (!enc.shortName.isEmpty()) {
            data = QString("%1;%2").arg(step).arg(enc.shortName);
        } else {
            data = QString("%1;%2").arg(step).arg(digits);
        }
        m_attributes->set("TOTP Settings", data);
    }
    endUpdate();
}

QString Entry::totpSeed() const
{
    QString secret = "";

    if (m_attributes->hasKey("otp")) {
        secret = m_attributes->value("otp");
    } else if (m_attributes->hasKey("TOTP Seed")) {
        secret = m_attributes->value("TOTP Seed");
    }

    return Totp::parseOtpString(secret, m_data.totpDigits, m_data.totpStep);
}

quint8 Entry::totpStep() const
{
    return m_data.totpStep;
}

quint8 Entry::totpDigits() const
{
    return m_data.totpDigits;
}

void Entry::setUuid(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());
    set(m_uuid, uuid);
}

void Entry::setIcon(int iconNumber)
{
    Q_ASSERT(iconNumber >= 0);

    if (m_data.iconNumber != iconNumber || !m_data.customIcon.isNull()) {
        m_data.iconNumber = iconNumber;
        m_data.customIcon = Uuid();

        emit modified();
        emitDataChanged();
    }
}

void Entry::setIcon(const Uuid& uuid)
{
    Q_ASSERT(!uuid.isNull());

    if (m_data.customIcon != uuid) {
        m_data.customIcon = uuid;
        m_data.iconNumber = 0;

        emit modified();
        emitDataChanged();
    }
}

void Entry::setForegroundColor(const QColor& color)
{
    set(m_data.foregroundColor, color);
}

void Entry::setBackgroundColor(const QColor& color)
{
    set(m_data.backgroundColor, color);
}

void Entry::setOverrideUrl(const QString& url)
{
    set(m_data.overrideUrl, url);
}

void Entry::setTags(const QString& tags)
{
    set(m_data.tags, tags);
}

void Entry::setTimeInfo(const TimeInfo& timeInfo)
{
    m_data.timeInfo = timeInfo;
}

void Entry::setAutoTypeEnabled(bool enable)
{
    set(m_data.autoTypeEnabled, enable);
}

void Entry::setAutoTypeObfuscation(int obfuscation)
{
    set(m_data.autoTypeObfuscation, obfuscation);
}

void Entry::setDefaultAutoTypeSequence(const QString& sequence)
{
    set(m_data.defaultAutoTypeSequence, sequence);
}

void Entry::setTitle(const QString& title)
{
    m_attributes->set(EntryAttributes::TitleKey, title, m_attributes->isProtected(EntryAttributes::TitleKey));
}

void Entry::setUrl(const QString& url)
{
    bool remove = url != m_attributes->value(EntryAttributes::URLKey) &&
                  (m_attributes->value(EntryAttributes::RememberCmdExecAttr) == "1" ||
                   m_attributes->value(EntryAttributes::RememberCmdExecAttr) == "0");
    if (remove) {
        m_attributes->remove(EntryAttributes::RememberCmdExecAttr);
    }
    m_attributes->set(EntryAttributes::URLKey, url, m_attributes->isProtected(EntryAttributes::URLKey));
}

void Entry::setUsername(const QString& username)
{
    m_attributes->set(EntryAttributes::UserNameKey, username, m_attributes->isProtected(EntryAttributes::UserNameKey));
}

void Entry::setPassword(const QString& password)
{
    m_attributes->set(EntryAttributes::PasswordKey, password, m_attributes->isProtected(EntryAttributes::PasswordKey));
}

void Entry::setNotes(const QString& notes)
{
    m_attributes->set(EntryAttributes::NotesKey, notes, m_attributes->isProtected(EntryAttributes::NotesKey));
}

void Entry::setExpires(const bool& value)
{
    if (m_data.timeInfo.expires() != value) {
        m_data.timeInfo.setExpires(value);
        emit modified();
    }
}

void Entry::setExpiryTime(const QDateTime& dateTime)
{
    if (m_data.timeInfo.expiryTime() != dateTime) {
        m_data.timeInfo.setExpiryTime(dateTime);
        emit modified();
    }
}

QList<Entry*> Entry::historyItems()
{
    return m_history;
}

const QList<Entry*>& Entry::historyItems() const
{
    return m_history;
}

void Entry::addHistoryItem(Entry* entry)
{
    Q_ASSERT(!entry->parent());

    m_history.append(entry);
    emit modified();
}

void Entry::removeHistoryItems(const QList<Entry*>& historyEntries)
{
    if (historyEntries.isEmpty()) {
        return;
    }

    for (Entry* entry : historyEntries) {
        Q_ASSERT(!entry->parent());
        Q_ASSERT(entry->uuid() == uuid());
        Q_ASSERT(m_history.contains(entry));

        m_history.removeOne(entry);
        delete entry;
    }

    emit modified();
}

void Entry::truncateHistory()
{
    const Database* db = database();

    if (!db) {
        return;
    }

    int histMaxItems = db->metadata()->historyMaxItems();
    if (histMaxItems > -1) {
        int historyCount = 0;
        QMutableListIterator<Entry*> i(m_history);
        i.toBack();
        while (i.hasPrevious()) {
            historyCount++;
            Entry* entry = i.previous();
            if (historyCount > histMaxItems) {
                delete entry;
                i.remove();
            }
        }
    }

    int histMaxSize = db->metadata()->historyMaxSize();
    if (histMaxSize > -1) {
        int size = 0;
        QSet<QByteArray> foundAttachments = attachments()->values();

        QMutableListIterator<Entry*> i(m_history);
        i.toBack();
        const QRegularExpression delimiter(",|:|;");
        while (i.hasPrevious()) {
            Entry* historyItem = i.previous();

            // don't calculate size if it's already above the maximum
            if (size <= histMaxSize) {
                size += historyItem->attributes()->attributesSize();
                size += historyItem->autoTypeAssociations()->associationsSize();
                size += historyItem->attachments()->attachmentsSize();
                size += historyItem->customData()->dataSize();
                const QStringList tags = historyItem->tags().split(delimiter, QString::SkipEmptyParts);
                for (const QString& tag : tags) {
                    size += tag.toUtf8().size();
                }
                foundAttachments += historyItem->attachments()->values();
            }

            if (size > histMaxSize) {
                delete historyItem;
                i.remove();
            }
        }
    }
}

Entry* Entry::clone(CloneFlags flags) const
{
    Entry* entry = new Entry();
    entry->setUpdateTimeinfo(false);
    if (flags & CloneNewUuid) {
        entry->m_uuid = Uuid::random();
    }
    else {
        entry->m_uuid = m_uuid;
    }
    entry->m_data = m_data;
    entry->m_customData->copyDataFrom(m_customData);
    entry->m_attributes->copyDataFrom(m_attributes);
    entry->m_attachments->copyDataFrom(m_attachments);

    if (flags & CloneUserAsRef) {
        // Build the username reference
        QString username = "{REF:U@I:" + m_uuid.toHex() + "}";
        entry->m_attributes->set(EntryAttributes::UserNameKey, username.toUpper(), m_attributes->isProtected(EntryAttributes::UserNameKey));
    }

    if (flags & ClonePassAsRef) {
        QString password = "{REF:P@I:" + m_uuid.toHex() + "}";
        entry->m_attributes->set(EntryAttributes::PasswordKey, password.toUpper(), m_attributes->isProtected(EntryAttributes::PasswordKey));
    }

    entry->m_autoTypeAssociations->copyDataFrom(m_autoTypeAssociations);
    if (flags & CloneIncludeHistory) {
        for (Entry* historyItem : m_history) {
            Entry* historyItemClone = historyItem->clone(flags & ~CloneIncludeHistory & ~CloneNewUuid);
            historyItemClone->setUpdateTimeinfo(false);
            historyItemClone->setUuid(entry->uuid());
            historyItemClone->setUpdateTimeinfo(true);
            entry->addHistoryItem(historyItemClone);
        }
    }
    entry->setUpdateTimeinfo(true);

    if (flags & CloneResetTimeInfo) {
        QDateTime now = QDateTime::currentDateTimeUtc();
        entry->m_data.timeInfo.setCreationTime(now);
        entry->m_data.timeInfo.setLastModificationTime(now);
        entry->m_data.timeInfo.setLastAccessTime(now);
        entry->m_data.timeInfo.setLocationChanged(now);
    }

    if (flags & CloneRenameTitle)
        entry->setTitle(entry->title() + tr(" - Clone", "Suffix added to cloned entries"));

    return entry;
}

void Entry::copyDataFrom(const Entry* other)
{
    setUpdateTimeinfo(false);
    m_data = other->m_data;
    m_customData->copyDataFrom(other->m_customData);
    m_attributes->copyDataFrom(other->m_attributes);
    m_attachments->copyDataFrom(other->m_attachments);
    m_autoTypeAssociations->copyDataFrom(other->m_autoTypeAssociations);
    setUpdateTimeinfo(true);
}

void Entry::beginUpdate()
{
    Q_ASSERT(!m_tmpHistoryItem);

    m_tmpHistoryItem = new Entry();
    m_tmpHistoryItem->setUpdateTimeinfo(false);
    m_tmpHistoryItem->m_uuid = m_uuid;
    m_tmpHistoryItem->m_data = m_data;
    m_tmpHistoryItem->m_attributes->copyDataFrom(m_attributes);
    m_tmpHistoryItem->m_attachments->copyDataFrom(m_attachments);
    m_tmpHistoryItem->m_autoTypeAssociations->copyDataFrom(m_autoTypeAssociations);

    m_modifiedSinceBegin = false;
}

bool Entry::endUpdate()
{
    Q_ASSERT(m_tmpHistoryItem);
    if (m_modifiedSinceBegin) {
        m_tmpHistoryItem->setUpdateTimeinfo(true);
        addHistoryItem(m_tmpHistoryItem);
        truncateHistory();
    }
    else {
        delete m_tmpHistoryItem;
    }

    m_tmpHistoryItem = nullptr;

    return m_modifiedSinceBegin;
}

void Entry::updateModifiedSinceBegin()
{
    m_modifiedSinceBegin = true;
}

/**
 * Update TOTP data whenever entry attributes have changed.
 */
void Entry::updateTotp()
{
    m_data.totpDigits = Totp::defaultDigits;
    m_data.totpStep = Totp::defaultStep;

    if (!m_attributes->hasKey("TOTP Settings")) {
        return;
    }

    // this regex must be kept in sync with the set of allowed short names Totp::shortNameToEncoder
    QRegularExpression rx(QString("(\\d+);((?:\\d+)|S)"));
    QRegularExpressionMatch m = rx.match(m_attributes->value("TOTP Settings"));
    if (!m.hasMatch()) {
        return;
    }

    m_data.totpStep = static_cast<quint8>(m.captured(1).toUInt());
    if (Totp::shortNameToEncoder.contains(m.captured(2))) {
        m_data.totpDigits = Totp::shortNameToEncoder[m.captured(2)];
    } else {
        m_data.totpDigits = static_cast<quint8>(m.captured(2).toUInt());
    }
}

QString Entry::resolveMultiplePlaceholdersRecursive(const QString& str, int maxDepth) const
{
    if (maxDepth <= 0) {
        qWarning("Maximum depth of replacement has been reached. Entry uuid: %s", qPrintable(uuid().toHex()));
        return str;
    }

    QString result = str;
    QRegExp placeholderRegEx("(\\{[^\\}]+\\})", Qt::CaseInsensitive, QRegExp::RegExp2);
    placeholderRegEx.setMinimal(true);
    int pos = 0;
    while ((pos = placeholderRegEx.indexIn(str, pos)) != -1) {
        const QString found = placeholderRegEx.cap(1);
        result.replace(found, resolvePlaceholderRecursive(found, maxDepth - 1));
        pos += placeholderRegEx.matchedLength();
    }

    if (result != str) {
        result = resolveMultiplePlaceholdersRecursive(result, maxDepth - 1);
    }

    return result;
}

QString Entry::resolvePlaceholderRecursive(const QString& placeholder, int maxDepth) const
{
    if (maxDepth <= 0) {
        qWarning("Maximum depth of replacement has been reached. Entry uuid: %s", qPrintable(uuid().toHex()));
        return placeholder;
    }

    const PlaceholderType typeOfPlaceholder = placeholderType(placeholder);
    switch (typeOfPlaceholder) {
    case PlaceholderType::NotPlaceholder:
    case PlaceholderType::Unknown:
        return resolveMultiplePlaceholdersRecursive(placeholder, maxDepth - 1);
    case PlaceholderType::Title:
        if (placeholderType(title()) == PlaceholderType::Title) {
            return title();
        }
        return resolveMultiplePlaceholdersRecursive(title(), maxDepth - 1);
    case PlaceholderType::UserName:
        if (placeholderType(username()) == PlaceholderType::UserName) {
            return username();
        }
        return resolveMultiplePlaceholdersRecursive(username(), maxDepth - 1);
    case PlaceholderType::Password:
        if (placeholderType(password()) == PlaceholderType::Password) {
            return password();
        }
        return resolveMultiplePlaceholdersRecursive(password(), maxDepth - 1);
    case PlaceholderType::Notes:
        if (placeholderType(notes()) == PlaceholderType::Notes) {
            return notes();
        }
        return resolveMultiplePlaceholdersRecursive(notes(), maxDepth - 1);
    case PlaceholderType::Url:
        if (placeholderType(url()) == PlaceholderType::Url) {
            return url();
        }
        return resolveMultiplePlaceholdersRecursive(url(), maxDepth - 1);
    case PlaceholderType::UrlWithoutScheme:
    case PlaceholderType::UrlScheme:
    case PlaceholderType::UrlHost:
    case PlaceholderType::UrlPort:
    case PlaceholderType::UrlPath:
    case PlaceholderType::UrlQuery:
    case PlaceholderType::UrlFragment:
    case PlaceholderType::UrlUserInfo:
    case PlaceholderType::UrlUserName:
    case PlaceholderType::UrlPassword: {
        const QString strUrl = resolveMultiplePlaceholdersRecursive(url(), maxDepth - 1);
        return resolveUrlPlaceholder(strUrl, typeOfPlaceholder);
    }
    case PlaceholderType::Totp:
        // totp can't have placeholder inside
        return totp();
    case PlaceholderType::CustomAttribute: {
        const QString key = placeholder.mid(3, placeholder.length() - 4); // {S:attr} => mid(3, len - 4)
        return attributes()->hasKey(key) ? attributes()->value(key) : QString();
    }
    case PlaceholderType::Reference:
        return resolveReferencePlaceholderRecursive(placeholder, maxDepth);
    }

    return placeholder;
}

QString Entry::resolveReferencePlaceholderRecursive(const QString& placeholder, int maxDepth) const
{
    if (maxDepth <= 0) {
        qWarning("Maximum depth of replacement has been reached. Entry uuid: %s", qPrintable(uuid().toHex()));
        return placeholder;
    }

    // resolving references in format: {REF:<WantedField>@<SearchIn>:<SearchText>}
    // using format from http://keepass.info/help/base/fieldrefs.html at the time of writing

    QRegularExpressionMatch match = EntryAttributes::matchReference(placeholder);
    if (!match.hasMatch()) {
        return placeholder;
    }

    QString result;
    const QString searchIn = match.captured(EntryAttributes::SearchInGroupName);
    const QString searchText = match.captured(EntryAttributes::SearchTextGroupName);

    const EntryReferenceType searchInType = Entry::referenceType(searchIn);

    Q_ASSERT(m_group);
    Q_ASSERT(m_group->database());
    const Entry* refEntry = m_group->database()->resolveEntry(searchText, searchInType);

    if (refEntry) {
        const QString wantedField = match.captured(EntryAttributes::WantedFieldGroupName);
        result = refEntry->referenceFieldValue(Entry::referenceType(wantedField));

        // Referencing fields of other entries only works with standard fields, not with custom user strings.
        // If you want to reference a custom user string, you need to place a redirection in a standard field
        // of the entry with the custom string, using {S:<Name>}, and reference the standard field.
        result = refEntry->resolveMultiplePlaceholdersRecursive(result, maxDepth - 1);
    }

    return result;
}

QString Entry::referenceFieldValue(EntryReferenceType referenceType) const
{
    switch (referenceType) {
    case EntryReferenceType::Title:
        return title();
    case EntryReferenceType::UserName:
        return username();
    case EntryReferenceType::Password:
        return password();
    case EntryReferenceType::Url:
        return url();
    case EntryReferenceType::Notes:
        return notes();
    case EntryReferenceType::Uuid:
        return uuid().toHex();
    default:
        break;
    }
    return QString();
}

Group* Entry::group()
{
    return m_group;
}

const Group* Entry::group() const
{
    return m_group;
}

void Entry::setGroup(Group* group)
{
    Q_ASSERT(group);

    if (m_group == group) {
        return;
    }

    if (m_group) {
        m_group->removeEntry(this);
        if (m_group->database() && m_group->database() != group->database()) {
            m_group->database()->addDeletedObject(m_uuid);

            // copy custom icon to the new database
            if (!iconUuid().isNull() && group->database()
                    && m_group->database()->metadata()->containsCustomIcon(iconUuid())
                    && !group->database()->metadata()->containsCustomIcon(iconUuid())) {
                group->database()->metadata()->addCustomIcon(iconUuid(), icon());
            }
        }
    }

    m_group = group;
    group->addEntry(this);

    QObject::setParent(group);

    if (m_updateTimeinfo) {
        m_data.timeInfo.setLocationChanged(QDateTime::currentDateTimeUtc());
    }
}

void Entry::emitDataChanged()
{
    emit dataChanged(this);
}

const Database* Entry::database() const
{
    if (m_group) {
        return m_group->database();
    }
    else {
        return nullptr;
    }
}

QString Entry::maskPasswordPlaceholders(const QString& str) const
{
    QString result = str;
    result.replace(QRegExp("(\\{PASSWORD\\})", Qt::CaseInsensitive, QRegExp::RegExp2), "******");
    return result;
}

QString Entry::resolveMultiplePlaceholders(const QString& str) const
{
    return resolveMultiplePlaceholdersRecursive(str, ResolveMaximumDepth);
}

QString Entry::resolvePlaceholder(const QString& placeholder) const
{
    return resolvePlaceholderRecursive(placeholder, ResolveMaximumDepth);
}

QString Entry::resolveUrlPlaceholder(const QString& str, Entry::PlaceholderType placeholderType) const
{
    if (str.isEmpty())
        return QString();

    const QUrl qurl(str);
    switch (placeholderType) {
    case PlaceholderType::UrlWithoutScheme:
        return qurl.toString(QUrl::RemoveScheme | QUrl::FullyDecoded);
    case PlaceholderType::UrlScheme:
        return qurl.scheme();
    case PlaceholderType::UrlHost:
        return qurl.host();
    case PlaceholderType::UrlPort:
        return QString::number(qurl.port());
    case PlaceholderType::UrlPath:
        return qurl.path();
    case PlaceholderType::UrlQuery:
        return qurl.query();
    case PlaceholderType::UrlFragment:
        return qurl.fragment();
    case PlaceholderType::UrlUserInfo:
        return qurl.userInfo();
    case PlaceholderType::UrlUserName:
        return qurl.userName();
    case PlaceholderType::UrlPassword:
        return qurl.password();
    default: {
        Q_ASSERT_X(false, "Entry::resolveUrlPlaceholder", "Bad url placeholder type");
        break;
    }
    }

    return QString();
}

Entry::PlaceholderType Entry::placeholderType(const QString& placeholder) const
{
    if (!placeholder.startsWith(QLatin1Char('{')) || !placeholder.endsWith(QLatin1Char('}'))) {
        return PlaceholderType::NotPlaceholder;
    } else if (placeholder.startsWith(QLatin1Literal("{S:"))) {
        return PlaceholderType::CustomAttribute;
    } else if (placeholder.startsWith(QLatin1Literal("{REF:"))) {
        return PlaceholderType::Reference;
    }

    static const QMap<QString, PlaceholderType> placeholders {
        { QStringLiteral("{TITLE}"), PlaceholderType::Title },
        { QStringLiteral("{USERNAME}"), PlaceholderType::UserName },
        { QStringLiteral("{PASSWORD}"), PlaceholderType::Password },
        { QStringLiteral("{NOTES}"), PlaceholderType::Notes },
        { QStringLiteral("{TOTP}"), PlaceholderType::Totp },
        { QStringLiteral("{URL}"), PlaceholderType::Url },
        { QStringLiteral("{URL:RMVSCM}"), PlaceholderType::UrlWithoutScheme },
        { QStringLiteral("{URL:WITHOUTSCHEME}"), PlaceholderType::UrlWithoutScheme },
        { QStringLiteral("{URL:SCM}"), PlaceholderType::UrlScheme },
        { QStringLiteral("{URL:SCHEME}"), PlaceholderType::UrlScheme },
        { QStringLiteral("{URL:HOST}"), PlaceholderType::UrlHost },
        { QStringLiteral("{URL:PORT}"), PlaceholderType::UrlPort },
        { QStringLiteral("{URL:PATH}"), PlaceholderType::UrlPath },
        { QStringLiteral("{URL:QUERY}"), PlaceholderType::UrlQuery },
        { QStringLiteral("{URL:FRAGMENT}"), PlaceholderType::UrlFragment },
        { QStringLiteral("{URL:USERINFO}"), PlaceholderType::UrlUserInfo },
        { QStringLiteral("{URL:USERNAME}"), PlaceholderType::UrlUserName },
        { QStringLiteral("{URL:PASSWORD}"), PlaceholderType::UrlPassword }
    };

    return placeholders.value(placeholder.toUpper(), PlaceholderType::Unknown);
}

QString Entry::resolveUrl(const QString& url) const
{
    QString newUrl = url;
    if (!url.isEmpty() && !url.contains("://")) {
        // URL doesn't have a protocol, add https by default
        newUrl.prepend("https://");
    }

    if (newUrl.startsWith("cmd://")) {
        QStringList cmdList = newUrl.split(" ");
        for (int i=1; i < cmdList.size(); ++i) {
            // Don't pass arguments to the resolveUrl function (they look like URL's)
            if (!cmdList[i].startsWith("-") && !cmdList[i].startsWith("/")) {
                return resolveUrl(cmdList[i].remove(QRegExp("'|\"")));
            }
        }

        // No URL in this command
        return QString("");
    }

    // Validate the URL
    QUrl tempUrl = QUrl(newUrl);
    if (tempUrl.isValid() && (tempUrl.scheme() == "http" || tempUrl.scheme() == "https")) {
        return tempUrl.url();
    }

    // No valid http URL's found
    return QString("");
}
