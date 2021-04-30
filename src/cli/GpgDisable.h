
#ifndef KEEPASSXC_GPG_DISABLE_H
#define KEEPASSXC_GPG_DISABLE_H

#include "DatabaseCommand.h"

class GpgDisable : public DatabaseCommand
{
public:
    GpgDisable();

    int executeWithDatabase(QSharedPointer<Database> db, QSharedPointer<QCommandLineParser> parser);
};

#endif // KEEPASSXC_GPG_DISABLE_H