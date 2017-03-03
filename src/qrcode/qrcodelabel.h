#ifndef QRCODELABEL_H
#define QRCODELABEL_H

#include <qlabel.h>
#include <QtGui>
#include <qrencode.h>

class QRCodeLabel : public QLabel
{
    Q_OBJECT
public:
    explicit QRCodeLabel(QWidget* parent = nullptr);
    ~QRCodeLabel();
    void setValue(QString text);
protected:
    void paintEvent(QPaintEvent* ev);
    QSize minimumSizeHint() const override;
    const int m_qrCodeSize = 6;
private:
    QRcode* m_qrCode = nullptr;
};

#endif // QRCODELABEL_H
