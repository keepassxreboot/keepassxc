#pragma once

#include <QByteArray>
#include <QString>

namespace attachments
{
    struct Attachment
    {
        QString name;
        QByteArray data;
    };

    enum class OpenMode
    {
        ReadOnly,
        ReadWrite
    };

} // namespace attachments
