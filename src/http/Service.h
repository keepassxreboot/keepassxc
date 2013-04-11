/**
 ***************************************************************************
 * @file Service.h
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#ifndef SERVICE_H
#define SERVICE_H

#include <QtCore/QObject>
#include "gui/DatabaseTabWidget.h"
#include "Server.h"

class Service : public KeepassHttpProtocol::Server
{
    Q_OBJECT

public:
    explicit Service(DatabaseTabWidget *parent = 0);

    virtual bool isDatabaseOpened() const;
    virtual bool openDatabase();
    virtual QString getDatabaseRootUuid();
    virtual QString getDatabaseRecycleBinUuid();
    virtual QString getKey(const QString &id);
    virtual QString storeKey(const QString &key);
    virtual QList<KeepassHttpProtocol::Entry> findMatchingEntries(const QString &id, const QString &url, const QString & submitUrl, const QString & realm);
    virtual int countMatchingEntries(const QString &id, const QString &url, const QString & submitUrl, const QString & realm);
    virtual QList<KeepassHttpProtocol::Entry> searchAllEntries(const QString &id);
    virtual void addEntry(const QString &id, const QString &login, const QString &password, const QString &url, const QString &submitUrl, const QString &realm);
    virtual void updateEntry(const QString &id, const QString &uuid, const QString &login, const QString &password, const QString &url);
    virtual QString generatePassword();

private:
    Entry* getConfigEntry(bool create = false);
    bool matchUrlScheme(const QString &url);
    bool removeFirstDomain(QString &hostname);
    Group *findCreateAddEntryGroup();
    QList<Entry *> searchEntries(const QString &text);

    DatabaseTabWidget * const m_dbTabWidget;
};

#endif // SERVICE_H
