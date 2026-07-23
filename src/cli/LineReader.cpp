#include "LineReader.h"
#ifdef USE_READLINE
#include <readline/history.h>
#include <readline/readline.h>
#else
#include "TextStream.h"
#endif

QString LineReader::readLine(const QString& prompt)
{
#ifdef USE_READLINE
    char* result = readline(prompt.toStdString().c_str());
    if (!result) {
        m_finished = true;
        return {};
    }
    add_history(result);
    QString qstr(result);
    free(result);
    return qstr;
#else
    static TextStream outStream(stdout, QIODevice::WriteOnly), inStream(stdin, QIODevice::ReadOnly);
    outStream << prompt;
    outStream.flush();
    QString result = inStream.readLine();
    if (result.isNull()) {
        m_finished = true;
    }
    return result;
#endif
}

bool LineReader::isFinished() const
{
    return m_finished;
}
