#include "PolkitDbusTypes.h"

void PolkitSubject::registerMetaType()
{
    qRegisterMetaType<PolkitSubject>("PolkitSubject");
    qDBusRegisterMetaType<PolkitSubject>();
}

QDBusArgument& operator<<(QDBusArgument& argument, const PolkitSubject& subject)
{
    argument.beginStructure();
    argument << subject.kind << subject.details;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitSubject& subject)
{
    argument.beginStructure();
    argument >> subject.kind >> subject.details;
    argument.endStructure();
    return argument;
}

void PolkitAuthorizationResults::registerMetaType()
{
    qRegisterMetaType<PolkitAuthorizationResults>("PolkitAuthorizationResults");
    qDBusRegisterMetaType<PolkitAuthorizationResults>();
}

QDBusArgument& operator<<(QDBusArgument& argument, const PolkitAuthorizationResults& res)
{
    argument.beginStructure();
    argument << res.is_authorized << res.is_challenge << res.details;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, PolkitAuthorizationResults& res)
{
    argument.beginStructure();
    argument >> res.is_authorized >> res.is_challenge >> res.details;
    argument.endStructure();
    return argument;
}
