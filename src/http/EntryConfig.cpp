/**
 ***************************************************************************
 * @file EntryConfig.cpp
 *
 * @brief
 *
 * Copyright (C) 2013
 *
 * @author	Francois Ferrand
 * @date	4/2013
 ***************************************************************************
 */

#include "EntryConfig.h"
#include "core/Entry.h"
#include "core/EntryAttributes.h"
#include "qjson/parser.h"
#include "qjson/qobjecthelper.h"
#include "qjson/serializer.h"

static const char KEEPASSHTTP_NAME[] = "KeePassHttp Settings";  //TODO: duplicated string (also in Service.cpp)

EntryConfig::EntryConfig(QObject *parent) :
    QObject(parent)
{
}

QStringList EntryConfig::allowedHosts() const
{
    return m_allowedHosts.toList();
}

void EntryConfig::setAllowedHosts(const QStringList &allowedHosts)
{
    m_allowedHosts = allowedHosts.toSet();
}

QStringList EntryConfig::deniedHosts() const
{
    return m_deniedHosts.toList();
}

void EntryConfig::setDeniedHosts(const QStringList &deniedHosts)
{
    m_deniedHosts = deniedHosts.toSet();
}

bool EntryConfig::isAllowed(const QString &host)
{
    return m_allowedHosts.contains(host);
}

void EntryConfig::allow(const QString &host)
{
    m_allowedHosts.insert(host);
    m_deniedHosts.remove(host);
}

bool EntryConfig::isDenied(const QString &host)
{
    return m_deniedHosts.contains(host);
}

void EntryConfig::deny(const QString &host)
{
    m_deniedHosts.insert(host);
    m_allowedHosts.remove(host);
}

QString EntryConfig::realm() const
{
    return m_realm;
}

void EntryConfig::setRealm(const QString &realm)
{
    m_realm = realm;
}

bool EntryConfig::load(const Entry *entry)
{
    QString s = entry->attributes()->value(KEEPASSHTTP_NAME);
    if (s.isEmpty())
        return false;

    bool isOk = false;
    QVariant v = QJson::Parser().parse(s.toUtf8(), &isOk);
    if (!isOk || !v.type() == QVariant::Map)
        return false;

    QJson::QObjectHelper::qvariant2qobject(v.toMap(), this);
    return true;
}

void EntryConfig::save(Entry *entry)
{
    QVariant v = QJson::QObjectHelper::qobject2qvariant(this, QJson::QObjectHelper::Flag_None);
    QByteArray json = QJson::Serializer().serialize(v);
    entry->attributes()->set(KEEPASSHTTP_NAME, json);
}
