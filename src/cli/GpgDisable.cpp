#include "GpgDisable.h"

#include "Utils.h"
#include "cli/TextStream.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Global.h"
#include "core/Group.h"

GpgDisable::GpgDisable()
{
    name = QString("gpgdisable");
    description = QObject::tr("Disable encryption for a specified path");
    positionalArguments.append({QString("path"), QObject::tr("Path to disable encryption for."), QString("[group]")});
}

int GpgDisable::executeWithDatabase(QSharedPointer<Database> database, QSharedPointer<QCommandLineParser> parser)
{
    auto& out = Utils::STDOUT;
    auto& err = Utils::STDERR;

    const QStringList args = parser->positionalArguments();

    const QString& path = args.at(1);

    Group* group = database->rootGroup()->findGroupByPath(path);
    if(!group){
        err << QObject::tr("Cannot find group %1.").arg(path) << endl;
        return EXIT_FAILURE;
    }
    group->customData()->set("GPG-Key","disable");
    QString errorMessage;
    if (!database->save(&errorMessage, true, false)) {
        err << QObject::tr("Writing the database failed: %1").arg(errorMessage) << endl;
        return EXIT_FAILURE;
    }

    err << QObject::tr("Disabled GPG encryption for path %1").arg(path) << endl;
    return EXIT_SUCCESS;
}