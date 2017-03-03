#include "qrcodedialog.h"
#include "ui_qrcodedialog.h"
#include <qrencode.h>

QRCodeDialog::QRCodeDialog(QWidget* parent, QString text) :
    QDialog(parent),
    m_ui(new Ui::QRCodeDialog)
{
    m_ui->setupUi(this);
    m_qrCodeImageLabel = new QRCodeLabel(this);
    m_qrCodeImageLabel->setObjectName(QStringLiteral("m_qrCodeImageLabel"));
    if (text != nullptr) {
        m_qrCodeImageLabel->setValue(text);
    }
    m_ui->verticalLayout->addWidget(m_qrCodeImageLabel);
}

void QRCodeDialog::on_closeButton_clicked()
{
    close();
}

QRCodeDialog::~QRCodeDialog()
{
    delete m_qrCodeImageLabel;
    delete m_ui;
}
