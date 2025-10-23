/********************************************************************************
** Form generated from reading UI file 'EditWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITWIDGET_H
#define UI_EDITWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/CategoryListWidget.h"
#include "gui/MessageWidget.h"

QT_BEGIN_NAMESPACE

class Ui_EditWidget
{
public:
    QVBoxLayout *verticalLayout;
    MessageWidget *messageWidget;
    QHBoxLayout *horizontalLayout_3;
    QLabel *headerLabel;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout;
    CategoryListWidget *categoryList;
    QStackedWidget *stackedWidget;
    QHBoxLayout *horizontalLayout_2;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *EditWidget)
    {
        if (EditWidget->objectName().isEmpty())
            EditWidget->setObjectName(QString::fromUtf8("EditWidget"));
        EditWidget->resize(527, 391);
        verticalLayout = new QVBoxLayout(EditWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        messageWidget = new MessageWidget(EditWidget);
        messageWidget->setObjectName(QString::fromUtf8("messageWidget"));

        verticalLayout->addWidget(messageWidget);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(2, 2, 2, 2);
        headerLabel = new QLabel(EditWidget);
        headerLabel->setObjectName(QString::fromUtf8("headerLabel"));

        horizontalLayout_3->addWidget(headerLabel);


        verticalLayout->addLayout(horizontalLayout_3);

        verticalSpacer = new QSpacerItem(1, 3, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(18);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        categoryList = new CategoryListWidget(EditWidget);
        categoryList->setObjectName(QString::fromUtf8("categoryList"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(categoryList->sizePolicy().hasHeightForWidth());
        categoryList->setSizePolicy(sizePolicy);
        categoryList->setFocusPolicy(Qt::TabFocus);

        horizontalLayout->addWidget(categoryList);

        stackedWidget = new QStackedWidget(EditWidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setFocusPolicy(Qt::TabFocus);

        horizontalLayout->addWidget(stackedWidget);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(-1, 5, -1, -1);
        buttonBox = new QDialogButtonBox(EditWidget);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setFocusPolicy(Qt::TabFocus);
        buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        horizontalLayout_2->addWidget(buttonBox);


        verticalLayout->addLayout(horizontalLayout_2);

        QWidget::setTabOrder(categoryList, stackedWidget);
        QWidget::setTabOrder(stackedWidget, buttonBox);

        retranslateUi(EditWidget);

        stackedWidget->setCurrentIndex(-1);


        QMetaObject::connectSlotsByName(EditWidget);
    } // setupUi

    void retranslateUi(QWidget *EditWidget)
    {
        headerLabel->setText(QString());
        (void)EditWidget;
    } // retranslateUi

};

namespace Ui {
    class EditWidget: public Ui_EditWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITWIDGET_H
