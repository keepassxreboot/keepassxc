
#ifndef KEEPASSXC_GPG_ENCRYPT_H
#define KEEPASSXC_GPG_ENCRYPT_H

#include "DatabaseCommand.h"

class GpgEncrypt : public DatabaseCommand
{
public:
    GpgEncrypt();

    int executeWithDatabase(QSharedPointer<Database> db, QSharedPointer<QCommandLineParser> parser);

    static const QCommandLineOption RecoveryKeyOption;
};

#endif // KEEPASSXC_GPG_ENCRYPT_H
