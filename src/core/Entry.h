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

#include <QtCore/QHash>
#include <QtCore/QUrl>
#include <QtGui/QColor>
#include <QtGui/QIcon>

#include "TimeInfo.h"
#include "Uuid.h"

class Database;
class Group;

struct AutoTypeAssociation
{
    QString window;
    QString sequence;
};

class Entry : public QObject
{
    Q_OBJECT

public:
    Entry();
    ~Entry();
    Uuid uuid() const;
    QIcon icon() const;
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
    const QHash<QString, QString>& attributes() const;
    const QHash<QString, QByteArray>& attachments() const;
    QString title() const;
    QString url() const;
    QString username() const;
    QString password() const;
    QString notes() const;

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
    void addAttribute(const QString& key, const QString& value);
    void addAttachment(const QString& key, const QByteArray& value);
    void setTitle(const QString& title);
    void setUrl(const QString& url);
    void setUsername(const QString& username);
    void setPassword(const QString& password);
    void setNotes(const QString& notes);

    QList<Entry*> historyItems();
    const QList<Entry*>& historyItems() const;
    void addHistoryItem(Entry* entry);

    Group* group();
    void setGroup(Group* group);


Q_SIGNALS:
    void dataChanged(Entry* entry);

private:
    Uuid m_uuid;
    int m_iconNumber;
    Uuid m_customIcon;
    QColor m_foregroundColor;
    QColor m_backgroundColor;
    QString m_overrideUrl;
    QString m_tags;
    TimeInfo m_timeInfo;
    bool m_autoTypeEnabled;
    int m_autoTypeObfuscation;
    QString m_defaultAutoTypeSequence;
    QList<AutoTypeAssociation> m_autoTypeAssociations;
    QHash<QString, QString> m_attributes;
    QHash<QString, QByteArray> m_binaries;

    QList<Entry*> m_history;
    Group* m_group;
    const Database* m_db;
};

#endif // KEEPASSX_ENTRY_H
