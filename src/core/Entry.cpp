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

const int Entry::DefaultIconNumber = 0;

Entry::Entry()
    : m_attributes(new EntryAttributes(this))
    , m_attachments(new EntryAttachments(this))
    , m_autoTypeAssociations(new AutoTypeAssociations(this))
    , m_tmpHistoryItem(nullptr)
    , m_modifiedSinceBegin(false)
    , m_updateTimeinfo(true)
{
    m_data.iconNumber = DefaultIconNumber;
    m_data.autoTypeEnabled = true;
    m_data.autoTypeObfuscation = 0;
    m_data.totpStep = QTotp::defaultStep;
    m_data.totpDigits = QTotp::defaultDigits;

    connect(m_attributes, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(m_attributes, SIGNAL(defaultKeyModified()), SLOT(emitDataChanged()));
    connect(m_attachments, SIGNAL(modified()), this, SIGNAL(modified()));
    connect(m_autoTypeAssociations, SIGNAL(modified()), SIGNAL(modified()));

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

QString Entry::effectiveAutoTypeSequence() const
{
    if (!m_data.defaultAutoTypeSequence.isEmpty()) {
        return m_data.defaultAutoTypeSequence;
    }
    QString sequence;

    const Group* grp = group();
    if(grp) {
      sequence = grp->effectiveAutoTypeSequence();
    } else {
      return QString();
    }

    if (sequence.isEmpty() && (!username().isEmpty() || !password().isEmpty())) {
        if (username().isEmpty()) {
            sequence = "{PASSWORD}{ENTER}";
        }
       else if (password().isEmpty()) {
          sequence = "{USERNAME}{ENTER}";
        }
        else {
            sequence = "{USERNAME}{TAB}{PASSWORD}{ENTER}";
        }
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
    return resolveUrl(m_attributes->value(EntryAttributes::URLKey));
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

bool Entry::hasTotp() const
{
    return m_attributes->hasKey("TOTP Seed") || m_attributes->hasKey("otp");
}

QString Entry::totp() const
{
    if (hasTotp()) {
        QString seed = totpSeed();
        quint64 time = QDateTime::currentDateTime().toTime_t();
        QString output = QTotp::generateTotp(seed.toLatin1(), time, m_data.totpDigits, m_data.totpStep);

        return QString(output);
    } else {
        return QString("");
    }
}

void Entry::setTotp(const QString& seed, quint8& step, quint8& digits)
{
    if (step == 0) {
        step = QTotp::defaultStep;
    }

    if (digits == 0) {
        digits = QTotp::defaultDigits;
    }

    if (m_attributes->hasKey("otp")) {
        m_attributes->set("otp", QString("key=%1&step=%2&size=%3").arg(seed).arg(step).arg(digits), true);
    } else {
        m_attributes->set("TOTP Seed", seed, true);
        m_attributes->set("TOTP Settings", QString("%1;%2").arg(step).arg(digits));
    }
}

QString Entry::totpSeed() const
{
    QString secret = "";

    if (m_attributes->hasKey("otp")) {
        secret = m_attributes->value("otp");
    } else if (m_attributes->hasKey("TOTP Seed")) {
        secret = m_attributes->value("TOTP Seed");
    }

    m_data.totpDigits = QTotp::defaultDigits;
    m_data.totpStep = QTotp::defaultStep;

    if (m_attributes->hasKey("TOTP Settings")) {
        QRegExp rx("(\\d+);(\\d)", Qt::CaseInsensitive, QRegExp::RegExp);
        int pos = rx.indexIn(m_attributes->value("TOTP Settings"));
        if (pos > -1) {
            m_data.totpStep = rx.cap(1).toUInt();
            m_data.totpDigits = rx.cap(2).toUInt();
        }
    }

    return QTotp::parseOtpString(secret, m_data.totpDigits, m_data.totpStep);
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
        QSet<QByteArray> foundAttachments = attachments()->values().toSet();

        QMutableListIterator<Entry*> i(m_history);
        i.toBack();
        while (i.hasPrevious()) {
            Entry* historyItem = i.previous();

            // don't calculate size if it's already above the maximum
            if (size <= histMaxSize) {
                size += historyItem->attributes()->attributesSize();

                const QSet<QByteArray> newAttachments = historyItem->attachments()->values().toSet() - foundAttachments;
                for (const QByteArray& attachment : newAttachments) {
                    size += attachment.size();
                }
                foundAttachments += newAttachments;
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
    entry->m_attributes->copyDataFrom(m_attributes);
    entry->m_attachments->copyDataFrom(m_attachments);

    if (flags & CloneUserAsRef) {
        // Build the username refrence
        QString username = "{REF:U@I:" + m_uuid.toHex() + "}";
        entry->m_attributes->set(EntryAttributes::UserNameKey, username.toUpper(), m_attributes->isProtected(EntryAttributes::UserNameKey));
    }

    if (flags & ClonePassAsRef) {
        QString password = "{REF:P@I:" + m_uuid.toHex() + "}";
        entry->m_attributes->set(EntryAttributes::PasswordKey, password.toUpper(), m_attributes->isProtected(EntryAttributes::PasswordKey));
    }

    entry->m_autoTypeAssociations->copyDataFrom(this->m_autoTypeAssociations);
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
        entry->setTitle(entry->title() + tr(" - Clone"));

    return entry;
}

void Entry::copyDataFrom(const Entry* other)
{
    setUpdateTimeinfo(false);
    m_data = other->m_data;
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

QString Entry::maskPasswordPlaceholders(const QString &str) const
{
    QString result = str;
    result.replace(QRegExp("(\\{PASSWORD\\})", Qt::CaseInsensitive, QRegExp::RegExp2), "******");
    return result;
}

QString Entry::resolveMultiplePlaceholders(const QString& str) const
{
    QString result = str;
    QRegExp tmplRegEx("(\\{.*\\})", Qt::CaseInsensitive, QRegExp::RegExp2);
    tmplRegEx.setMinimal(true);
    QStringList tmplList;
    int pos = 0;

    while ((pos = tmplRegEx.indexIn(str, pos)) != -1) {
        QString found = tmplRegEx.cap(1);
        result.replace(found,resolvePlaceholder(found));
        pos += tmplRegEx.matchedLength();
    }

    return result;
}

QString Entry::resolvePlaceholder(const QString& str) const
{
    QString result = str;

    const QList<QString> keyList = attributes()->keys();
    for (const QString& key : keyList) {
        Qt::CaseSensitivity cs = Qt::CaseInsensitive;
        QString k = key;

        if (!EntryAttributes::isDefaultAttribute(key)) {
            cs = Qt::CaseSensitive;
            k.prepend("{S:");
        } else {
            k.prepend("{");
        }


        k.append("}");
        if (result.compare(k,cs)==0) {
            result.replace(result,attributes()->value(key));
            break;
        }
    }

    // resolving references in format: {REF:<WantedField>@I:<uuid of referenced entry>}
    // using format from http://keepass.info/help/base/fieldrefs.html at the time of writing,
    // but supporting lookups of standard fields and references by UUID only

    QRegExp* tmpRegExp = m_attributes->referenceRegExp();
    if (tmpRegExp->indexIn(result) != -1) {
        // cap(0) contains the whole reference
        // cap(1) contains which field is wanted
        // cap(2) contains the uuid of the referenced entry
        Entry* tmpRefEntry = m_group->database()->resolveEntry(Uuid(QByteArray::fromHex(tmpRegExp->cap(2).toLatin1())));
        if (tmpRefEntry) {
            // entry found, get the relevant field
            QString tmpRefField = tmpRegExp->cap(1).toLower();
            if (tmpRefField == "t") result.replace(tmpRegExp->cap(0), tmpRefEntry->title(), Qt::CaseInsensitive);
            else if (tmpRefField == "u") result.replace(tmpRegExp->cap(0), tmpRefEntry->username(), Qt::CaseInsensitive);
            else if (tmpRefField == "p") result.replace(tmpRegExp->cap(0), tmpRefEntry->password(), Qt::CaseInsensitive);
            else if (tmpRefField == "a") result.replace(tmpRegExp->cap(0), tmpRefEntry->url(), Qt::CaseInsensitive);
            else if (tmpRefField == "n") result.replace(tmpRegExp->cap(0), tmpRefEntry->notes(), Qt::CaseInsensitive);
        }
    }

    return result;
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
