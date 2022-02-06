#ifndef KEEPASSX_WINDOWSHELLO_H
#define KEEPASSX_WINDOWSHELLO_H

#define WINDOWSHELLO_UNDEFINED -1
#define WINDOWSHELLO_AVAILABLE 1
#define WINDOWSHELLO_NOT_AVAILABLE 0


#include <QString>


namespace WindowsHello
{
    bool storeKey(const QString& databasePath, const QByteArray& passwordKey);

    bool getKey(const QString& databasePath, QByteArray& passwordKey);

    bool isAvailable();

    void reset(const QString& databasePath = "");
}

#endif // KEEPASSX_WINDOWSHELLO_H