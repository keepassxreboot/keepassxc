/**
 ***************************************************************************
 * @file EntryConfig.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef ENTRYCONFIG_H
#define ENTRYCONFIG_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSet>

class Entry;

class EntryConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList Allow READ allowedHosts WRITE setAllowedHosts)
    Q_PROPERTY(QStringList Deny  READ deniedHosts  WRITE setDeniedHosts )
    Q_PROPERTY(QString     Realm READ realm        WRITE setRealm       )

public:
    EntryConfig(QObject * object = 0);

    bool load(const Entry * entry);
    void save(Entry * entry);
    bool isAllowed(const QString & host);
    void allow(const QString & host);
    bool isDenied(const QString & host);
    void deny(const QString & host);
    QString realm() const;
    void setRealm(const QString &realm);

private:
    QStringList allowedHosts() const;
    void setAllowedHosts(const QStringList &allowedHosts);
    QStringList deniedHosts() const;
    void setDeniedHosts(const QStringList &deniedHosts);

    QSet<QString> m_allowedHosts;
    QSet<QString> m_deniedHosts;
    QString       m_realm;
};

#endif // ENTRYCONFIG_H
