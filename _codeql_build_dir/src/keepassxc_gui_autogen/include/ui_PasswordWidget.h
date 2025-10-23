/********************************************************************************
** Form generated from reading UI file 'PasswordWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PASSWORDWIDGET_H
#define UI_PASSWORDWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PasswordWidget
{
public:
    QVBoxLayout *verticalLayout;
    QLineEdit *passwordEdit;
    QProgressBar *qualityProgressBar;

    void setupUi(QWidget *PasswordWidget)
    {
        if (PasswordWidget->objectName().isEmpty())
            PasswordWidget->setObjectName(QString::fromUtf8("PasswordWidget"));
        PasswordWidget->resize(471, 25);
        verticalLayout = new QVBoxLayout(PasswordWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        passwordEdit = new QLineEdit(PasswordWidget);
        passwordEdit->setObjectName(QString::fromUtf8("passwordEdit"));

        verticalLayout->addWidget(passwordEdit);

        qualityProgressBar = new QProgressBar(PasswordWidget);
        qualityProgressBar->setObjectName(QString::fromUtf8("qualityProgressBar"));
        qualityProgressBar->setMaximumSize(QSize(16777215, 4));
        qualityProgressBar->setStyleSheet(QString::fromUtf8("QProgressBar {\n"
"              border: none;\n"
"              background-color: transparent;\n"
"              }\n"
"              QProgressBar::chunk {\n"
"              background-color: #c0392b;\n"
"              border-radius: 1px;\n"
"              }\n"
"            "));
        qualityProgressBar->setValue(24);
        qualityProgressBar->setTextVisible(false);

        verticalLayout->addWidget(qualityProgressBar);


        retranslateUi(PasswordWidget);

        QMetaObject::connectSlotsByName(PasswordWidget);
    } // setupUi

    void retranslateUi(QWidget *PasswordWidget)
    {
#if QT_CONFIG(accessibility)
        passwordEdit->setAccessibleDescription(QCoreApplication::translate("PasswordWidget", "Toggle password visibility using Control + H. Open the password generator using Control + G.", nullptr));
#endif // QT_CONFIG(accessibility)
        (void)PasswordWidget;
    } // retranslateUi

};

namespace Ui {
    class PasswordWidget: public Ui_PasswordWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PASSWORDWIDGET_H
