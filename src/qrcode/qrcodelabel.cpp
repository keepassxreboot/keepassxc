#include "qrcodelabel.h"

QRCodeLabel::QRCodeLabel(QWidget* parent) :
    QLabel(parent)
{
}

void QRCodeLabel::setValue(QString text)
{
    if (text != nullptr) {
        auto* str = text.toUtf8().data();
        m_qrCode = QRcode_encodeString8bit(str, 0, QR_ECLEVEL_L);
    }
}

QSize QRCodeLabel::minimumSizeHint()const
{
    if (m_qrCode) {
        auto value = m_qrCode->width * m_qrCodeSize;
        return QSize(value, value);
    }
    else {
        return QSize(1,1);
    }
}

void QRCodeLabel::paintEvent(QPaintEvent*)
{
    if (!m_qrCode) {
        return;
    }

    QPainter painter(this);
    int margin = 0;
    int ox = margin * m_qrCodeSize;
    int oy = margin * m_qrCodeSize;
    int width = m_qrCode->width;
    QRect rect;
    QColor color;
    unsigned char* p = m_qrCode->data;

    for (int y = 0; y < width; y++) {
        for (int x = 0; x < width; x++) {
            rect.setX(ox + x * m_qrCodeSize);
            rect.setY(oy + y * m_qrCodeSize);
            rect.setWidth(m_qrCodeSize);
            rect.setHeight(m_qrCodeSize);
            if (*p & 1) {
                color = QColor("black");
            } else {
                color = QColor("white");
            }

            painter.drawRect(rect);
            painter.fillRect(rect, color);
            p++;
        }
    }
}

QRCodeLabel::~QRCodeLabel()
{
    delete m_qrCode;
}
