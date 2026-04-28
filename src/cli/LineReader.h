#ifndef LINEREADER_H
#define LINEREADER_H

#include <QString>

class LineReader
{
    bool m_finished{false};

public:
    LineReader() = default;
    LineReader(const LineReader&) = delete;
    ~LineReader() = default;
    LineReader& operator=(const LineReader&) = delete;
    QString readLine(const QString& prompt);
    [[nodiscard]] bool isFinished() const;
};

#endif // LINEREADER_H
