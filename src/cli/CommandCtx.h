#ifndef COMMANDCTX_H
#define COMMANDCTX_H

#include <memory>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QStack>

#include "core/Database.h"
#include "Utils.h"


enum class Runmode
{
    Version,
    DebugInfo,
    Help,
    SingleCmd,
    InteractiveCmd
};

constexpr int runmodeMask(Runmode mode)
{
    return 1 << static_cast<int>(mode);
}
constexpr int runmodeMaskBoth()
{
    return runmodeMask(Runmode::SingleCmd) | runmodeMask(Runmode::InteractiveCmd);
}

class Command;

struct CommandCtx
{
    CommandCtx(QCommandLineParser& parser, const QStringList& args)
    {
        cmdInit();
        parseArgs(parser, args);
    }

    bool error()                        const { return !m_errors.empty(); }
    const QStack<QString>& getErrors()  const { return m_errors; }
    void clearErrors()                        { m_errors.clear(); }
    Runmode getRunmode()                const { return m_runmode; }
    void startInteractive()                   { m_runmode = Runmode::InteractiveCmd; }
    void stopInteractive()                    { m_runmode = Runmode::SingleCmd; }
    bool hasDb()                        const { return !!m_db; }

    QSharedPointer<Command> getCmd(const QString& name)
    {
        const auto cmd = m_commands.find(name);
        return cmd != m_commands.end()
            ? *cmd
            : QSharedPointer<Command>(nullptr);
    }

    template<class F>
    void forEachCmd(const F& f) const
    {
        for (const auto& cmd : m_commands) {
            f(*cmd);
        }
    }

    Database& getDb()
    {
        Q_ASSERT(m_db);
        Q_ASSERT(m_db->isInitialized());
        return *m_db;
    }

    void setDb(std::unique_ptr<Database> db)
    {
        Q_ASSERT(db->isInitialized());
        m_db = std::move(db);
    }

    void logError(const QString& msg)
    {
        m_errors.push(msg);
    }

private:
    void cmdInit();
    int parseArgs(QCommandLineParser& parser, const QStringList& args);

    Runmode m_runmode;
    QMap<QString, QSharedPointer<Command>> m_commands;
    std::unique_ptr<Database> m_db;
    QStack<QString> m_errors;
};

#define BREAK_IF(COND, RET_VAL, CTX, ERR_MSG)   \
    if (!!(COND)) {                             \
        (CTX).logError((ERR_MSG));              \
        return (RET_VAL);                       \
    }

#endif // COMMANDCTX_H
