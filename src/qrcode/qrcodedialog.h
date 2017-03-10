#ifndef QRCODEDIALOG_H
#define QRCODEDIALOG_H

#include <QDialog>
#include "qrcodelabel.h"

namespace Ui {
    class QRCodeDialog;
}

class QRCodeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QRCodeDialog(QWidget* parent = nullptr, QString text = nullptr);
    ~QRCodeDialog();

private slots:
    void on_closeButton_clicked();

private:
    Ui::QRCodeDialog* m_ui;
    QRCodeLabel* m_qrCodeImageLabel;
};

#endif // QRCODEDIALOG_H
