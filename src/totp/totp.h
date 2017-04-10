#ifndef QTOTP_H
#define QTOTP_H

#include <QtCore/qglobal.h>

class QTotp
{
public:
    QTotp();
    static int base32_decode(const quint8 *encoded, quint8 *result, int bufSize);
    static QByteArray hmacSha1(QByteArray key, QByteArray baseString);
    static QString generateTotp(const QByteArray key);
};

#endif // QTOTP_H
