#ifndef KEEPASSXC_QRDECODER_H
#define KEEPASSXC_QRDECODER_H

class QImage;
class QString;

namespace QrDecoder
{
    QString decode(const QImage& image);
}

#endif // KEEPASSXC_QRDECODER_H
