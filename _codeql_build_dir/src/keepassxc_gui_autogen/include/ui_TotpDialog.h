/********************************************************************************
** Form generated from reading UI file 'TotpDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TOTPDIALOG_H
#define UI_TOTPDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TotpDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *totpLabel;
    QProgressBar *progressBar;
    QLabel *timerLabel;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *TotpDialog)
    {
        if (TotpDialog->objectName().isEmpty())
            TotpDialog->setObjectName(QString::fromUtf8("TotpDialog"));
        TotpDialog->resize(264, 194);
        verticalLayout = new QVBoxLayout(TotpDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        totpLabel = new QLabel(TotpDialog);
        totpLabel->setObjectName(QString::fromUtf8("totpLabel"));
        QFont font;
        font.setPointSize(53);
        totpLabel->setFont(font);
        totpLabel->setText(QString::fromUtf8("000000"));
        totpLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(totpLabel);

        progressBar = new QProgressBar(TotpDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);
        progressBar->setTextVisible(false);

        verticalLayout->addWidget(progressBar);

        timerLabel = new QLabel(TotpDialog);
        timerLabel->setObjectName(QString::fromUtf8("timerLabel"));

        verticalLayout->addWidget(timerLabel);

        buttonBox = new QDialogButtonBox(TotpDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Close|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(TotpDialog);

        QMetaObject::connectSlotsByName(TotpDialog);
    } // setupUi

    void retranslateUi(QWidget *TotpDialog)
    {
        TotpDialog->setWindowTitle(QCoreApplication::translate("TotpDialog", "Timed Password", nullptr));
        timerLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class TotpDialog: public Ui_TotpDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TOTPDIALOG_H
