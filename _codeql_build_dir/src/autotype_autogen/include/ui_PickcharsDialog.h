/********************************************************************************
** Form generated from reading UI file 'PickcharsDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PICKCHARSDIALOG_H
#define UI_PICKCHARSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include "gui/PasswordWidget.h"

QT_BEGIN_NAMESPACE

class Ui_PickcharsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QGridLayout *charsGrid;
    PasswordWidget *selectedChars;
    QHBoxLayout *horizontalLayout;
    QCheckBox *pressTab;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *PickcharsDialog)
    {
        if (PickcharsDialog->objectName().isEmpty())
            PickcharsDialog->setObjectName(QString::fromUtf8("PickcharsDialog"));
        PickcharsDialog->resize(418, 188);
        verticalLayout = new QVBoxLayout(PickcharsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetFixedSize);
        label = new QLabel(PickcharsDialog);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(label);

        charsGrid = new QGridLayout();
        charsGrid->setObjectName(QString::fromUtf8("charsGrid"));

        verticalLayout->addLayout(charsGrid);

        selectedChars = new PasswordWidget(PickcharsDialog);
        selectedChars->setObjectName(QString::fromUtf8("selectedChars"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(selectedChars->sizePolicy().hasHeightForWidth());
        selectedChars->setSizePolicy(sizePolicy1);
        selectedChars->setMinimumSize(QSize(150, 0));

        verticalLayout->addWidget(selectedChars);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        pressTab = new QCheckBox(PickcharsDialog);
        pressTab->setObjectName(QString::fromUtf8("pressTab"));
        pressTab->setChecked(true);

        horizontalLayout->addWidget(pressTab);

        buttonBox = new QDialogButtonBox(PickcharsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout->addWidget(buttonBox);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(PickcharsDialog);

        QMetaObject::connectSlotsByName(PickcharsDialog);
    } // setupUi

    void retranslateUi(QDialog *PickcharsDialog)
    {
        PickcharsDialog->setWindowTitle(QCoreApplication::translate("PickcharsDialog", "KeePassXC - Pick Characters", nullptr));
        label->setText(QCoreApplication::translate("PickcharsDialog", "Select characters to type, navigate with arrow keys, Ctrl + S submits.", nullptr));
        pressTab->setText(QCoreApplication::translate("PickcharsDialog", "Press &Tab between characters", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PickcharsDialog: public Ui_PickcharsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PICKCHARSDIALOG_H
