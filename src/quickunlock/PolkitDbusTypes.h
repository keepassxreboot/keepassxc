#ifndef KEEPASSX_POLKITDBUSTYPES_H
#define KEEPASSX_POLKITDBUSTYPES_H

#include <QtDBus>

class PolkitSubject
{
public:
    QString kind;
    QVariantMap details;

    static void registerMetaType();

    friend QDBusArgument& operator<<(QDBusArgument& argument, const PolkitSubject& subject);

    friend const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitSubject& subject);
};

class PolkitAuthorizationResults
{
public:
    bool is_authorized;
    bool is_challenge;
    QMap<QString, QString> details;

    static void registerMetaType();

    friend QDBusArgument& operator<<(QDBusArgument& argument, const PolkitAuthorizationResults& subject);

    friend const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitAuthorizationResults& subject);
};

Q_DECLARE_METATYPE(PolkitSubject);
Q_DECLARE_METATYPE(PolkitAuthorizationResults);

#endif // KEEPASSX_POLKITDBUSTYPES_H
