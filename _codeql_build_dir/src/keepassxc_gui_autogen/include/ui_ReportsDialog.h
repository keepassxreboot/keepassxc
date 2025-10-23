/********************************************************************************
** Form generated from reading UI file 'ReportsDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPORTSDIALOG_H
#define UI_REPORTSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/CategoryListWidget.h"

QT_BEGIN_NAMESPACE

class Ui_ReportsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    CategoryListWidget *categoryList;
    QStackedWidget *stackedWidget;
    QHBoxLayout *horizontalLayout;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *ReportsDialog)
    {
        if (ReportsDialog->objectName().isEmpty())
            ReportsDialog->setObjectName(QString::fromUtf8("ReportsDialog"));
        verticalLayout = new QVBoxLayout(ReportsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        categoryList = new CategoryListWidget(ReportsDialog);
        categoryList->setObjectName(QString::fromUtf8("categoryList"));

        horizontalLayout_2->addWidget(categoryList);

        stackedWidget = new QStackedWidget(ReportsDialog);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));

        horizontalLayout_2->addWidget(stackedWidget);

        horizontalLayout_2->setStretch(1, 1);

        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        buttonBox = new QDialogButtonBox(ReportsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        horizontalLayout->addWidget(buttonBox);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(ReportsDialog);

        stackedWidget->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(ReportsDialog);
    } // setupUi

    void retranslateUi(QWidget *ReportsDialog)
    {
        (void)ReportsDialog;
    } // retranslateUi

};

namespace Ui {
    class ReportsDialog: public Ui_ReportsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPORTSDIALOG_H
