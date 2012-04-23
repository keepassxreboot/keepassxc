/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_ENTRY_H
#define KEEPASSX_ENTRY_H

#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QUrl>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QPixmapCache>

#include "core/EntryAttachments.h"
#include "core/EntryAttributes.h"
#include "core/TimeInfo.h"
#include "core/Uuid.h"

class Database;
class Group;

struct AutoTypeAssociation
{
    QString window;
    QString sequence;
};

Q_DECLARE_TYPEINFO(AutoTypeAssociation, Q_MOVABLE_TYPE);

struct EntryData
{
    int iconNumber;
    Uuid customIcon;
    QColor foregroundColor;
    QColor backgroundColor;
    QString overrideUrl;
    QString tags;
    bool autoTypeEnabled;
    int autoTypeObfuscation;
    QString defaultAutoTypeSequence;
    QList<AutoTypeAssociation> autoTypeAssociations;
    TimeInfo timeInfo;
};

class Entry : public QObject
{
    Q_OBJECT

public:
    Entry();
    ~Entry();
    Uuid uuid() const;
    QImage icon() const;
    QPixmap iconPixmap() const;
    int iconNumber() const;
    Uuid iconUuid() const;
    QColor foregroundColor() const;
    QColor backgroundColor() const;
    QString overrideUrl() const;
    QString tags() const;
    TimeInfo timeInfo() const;
    bool autoTypeEnabled() const;
    int autoTypeObfuscation() const;
    QString defaultAutoTypeSequence() const;
    const QList<AutoTypeAssociation>& autoTypeAssociations() const;
    QString title() const;
    QString url() const;
    QString username() const;
    QString password() const;
    QString notes() const;
    EntryAttributes* attributes();
    const EntryAttributes* attributes() const;
    EntryAttachments* attachments();
    const EntryAttachments* attachments() const;

    void setUuid(const Uuid& uuid);
    void setIcon(int iconNumber);
    void setIcon(const Uuid& uuid);
    void setForegroundColor(const QColor& color);
    void setBackgroundColor(const QColor& color);
    void setOverrideUrl(const QString& url);
    void setTags(const QString& tags);
    void setTimeInfo(const TimeInfo& timeInfo);
    void setAutoTypeEnabled(bool enable);
    void setAutoTypeObfuscation(int obfuscation);
    void setDefaultAutoTypeSequence(const QString& sequence);
    void addAutoTypeAssociation(const AutoTypeAssociation& assoc);
    void setTitle(const QString& title);
    void setUrl(const QString& url);
    void setUsername(const QString& username);
    void setPassword(const QString& password);
    void setNotes(const QString& notes);
    void setExpires(const bool& value);
    void setExpiryTime(const QDateTime& dateTime);

    QList<Entry*> historyItems();
    const QList<Entry*>& historyItems() const;
    void addHistoryItem(Entry* entry);
    /**
     * Call before and after set*() methods to create a history item
     * if the entry has been changed.
     */
    void beginUpdate();
    void endUpdate();

    Group* group();
    void setGroup(Group* group);

    void setUpdateTimeinfo(bool value);

Q_SIGNALS:
    /**
     * Emitted when a default attribute has been changed.
     */
    void dataChanged(Entry* entry);

    void modified();

private Q_SLOTS:
    void emitDataChanged();
    void updateTimeinfo();
    void updateModifiedSinceBegin();

private:
    const Database* database() const;
    template <class T> inline bool set(T& property, const T& value);

    Uuid m_uuid;
    EntryData m_data;
    EntryAttributes* m_attributes;
    EntryAttachments* m_attachments;

    QList<Entry*> m_history;
    Entry* m_tmpHistoryItem;
    bool m_modifiedSinceBegin;
    QPointer<Group> m_group;
    QPixmapCache::Key m_pixmapCacheKey;
    bool m_updateTimeinfo;
};

#endif // KEEPASSX_ENTRY_H
