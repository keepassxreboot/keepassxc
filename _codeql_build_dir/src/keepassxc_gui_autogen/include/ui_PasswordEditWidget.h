/********************************************************************************
** Form generated from reading UI file 'PasswordEditWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PASSWORDEDITWIDGET_H
#define UI_PASSWORDEDITWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>
#include "gui/PasswordWidget.h"

QT_BEGIN_NAMESPACE

class Ui_PasswordEditWidget
{
public:
    QFormLayout *formLayout;
    QLabel *enterPasswordLabel;
    PasswordWidget *enterPasswordEdit;
    QLabel *repeatPasswordLabel;
    PasswordWidget *repeatPasswordEdit;

    void setupUi(QWidget *PasswordEditWidget)
    {
        if (PasswordEditWidget->objectName().isEmpty())
            PasswordEditWidget->setObjectName(QString::fromUtf8("PasswordEditWidget"));
        PasswordEditWidget->resize(571, 78);
        formLayout = new QFormLayout(PasswordEditWidget);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setContentsMargins(0, 0, 0, 0);
        enterPasswordLabel = new QLabel(PasswordEditWidget);
        enterPasswordLabel->setObjectName(QString::fromUtf8("enterPasswordLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, enterPasswordLabel);

        enterPasswordEdit = new PasswordWidget(PasswordEditWidget);
        enterPasswordEdit->setObjectName(QString::fromUtf8("enterPasswordEdit"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(enterPasswordEdit->sizePolicy().hasHeightForWidth());
        enterPasswordEdit->setSizePolicy(sizePolicy);
        enterPasswordEdit->setMinimumSize(QSize(300, 0));
        enterPasswordEdit->setFocusPolicy(Qt::StrongFocus);

        formLayout->setWidget(0, QFormLayout::FieldRole, enterPasswordEdit);

        repeatPasswordLabel = new QLabel(PasswordEditWidget);
        repeatPasswordLabel->setObjectName(QString::fromUtf8("repeatPasswordLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, repeatPasswordLabel);

        repeatPasswordEdit = new PasswordWidget(PasswordEditWidget);
        repeatPasswordEdit->setObjectName(QString::fromUtf8("repeatPasswordEdit"));
        sizePolicy.setHeightForWidth(repeatPasswordEdit->sizePolicy().hasHeightForWidth());
        repeatPasswordEdit->setSizePolicy(sizePolicy);
        repeatPasswordEdit->setMinimumSize(QSize(300, 0));
        repeatPasswordEdit->setFocusPolicy(Qt::StrongFocus);

        formLayout->setWidget(1, QFormLayout::FieldRole, repeatPasswordEdit);

        QWidget::setTabOrder(enterPasswordEdit, repeatPasswordEdit);

        retranslateUi(PasswordEditWidget);

        QMetaObject::connectSlotsByName(PasswordEditWidget);
    } // setupUi

    void retranslateUi(QWidget *PasswordEditWidget)
    {
        enterPasswordLabel->setText(QCoreApplication::translate("PasswordEditWidget", "Enter password:", nullptr));
#if QT_CONFIG(accessibility)
        enterPasswordEdit->setAccessibleName(QCoreApplication::translate("PasswordEditWidget", "Password field", nullptr));
#endif // QT_CONFIG(accessibility)
        repeatPasswordLabel->setText(QCoreApplication::translate("PasswordEditWidget", "Confirm password:", nullptr));
#if QT_CONFIG(accessibility)
        repeatPasswordEdit->setAccessibleName(QCoreApplication::translate("PasswordEditWidget", "Repeat password field", nullptr));
#endif // QT_CONFIG(accessibility)
        (void)PasswordEditWidget;
    } // retranslateUi

};

namespace Ui {
    class PasswordEditWidget: public Ui_PasswordEditWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PASSWORDEDITWIDGET_H
