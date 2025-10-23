/********************************************************************************
** Form generated from reading UI file 'KeyFileEditWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_KEYFILEEDITWIDGET_H
#define UI_KEYFILEEDITWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_KeyFileEditWidget
{
public:
    QGridLayout *gridLayout;
    QPushButton *createKeyFileButton;
    QLabel *instructions;
    QLabel *instructions_2;
    QSpacerItem *verticalSpacer;
    QLineEdit *keyFileLineEdit;
    QPushButton *browseKeyFileButton;

    void setupUi(QWidget *KeyFileEditWidget)
    {
        if (KeyFileEditWidget->objectName().isEmpty())
            KeyFileEditWidget->setObjectName(QString::fromUtf8("KeyFileEditWidget"));
        KeyFileEditWidget->resize(566, 94);
        gridLayout = new QGridLayout(KeyFileEditWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        createKeyFileButton = new QPushButton(KeyFileEditWidget);
        createKeyFileButton->setObjectName(QString::fromUtf8("createKeyFileButton"));

        gridLayout->addWidget(createKeyFileButton, 0, 1, 1, 1);

        instructions = new QLabel(KeyFileEditWidget);
        instructions->setObjectName(QString::fromUtf8("instructions"));
        instructions->setWordWrap(true);

        gridLayout->addWidget(instructions, 0, 0, 1, 1);

        instructions_2 = new QLabel(KeyFileEditWidget);
        instructions_2->setObjectName(QString::fromUtf8("instructions_2"));
        QFont font;
        font.setItalic(true);
        instructions_2->setFont(font);
        instructions_2->setWordWrap(true);

        gridLayout->addWidget(instructions_2, 3, 0, 1, 1);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 4, 0, 1, 1);

        keyFileLineEdit = new QLineEdit(KeyFileEditWidget);
        keyFileLineEdit->setObjectName(QString::fromUtf8("keyFileLineEdit"));
        keyFileLineEdit->setClearButtonEnabled(true);

        gridLayout->addWidget(keyFileLineEdit, 1, 0, 1, 1);

        browseKeyFileButton = new QPushButton(KeyFileEditWidget);
        browseKeyFileButton->setObjectName(QString::fromUtf8("browseKeyFileButton"));

        gridLayout->addWidget(browseKeyFileButton, 1, 1, 1, 1);


        retranslateUi(KeyFileEditWidget);

        QMetaObject::connectSlotsByName(KeyFileEditWidget);
    } // setupUi

    void retranslateUi(QWidget *KeyFileEditWidget)
    {
#if QT_CONFIG(accessibility)
        createKeyFileButton->setAccessibleName(QCoreApplication::translate("KeyFileEditWidget", "Generate a new key file", nullptr));
#endif // QT_CONFIG(accessibility)
        createKeyFileButton->setText(QCoreApplication::translate("KeyFileEditWidget", "Generate", nullptr));
        instructions->setText(QCoreApplication::translate("KeyFileEditWidget", "Generate a new key file or choose an existing one to protect your database.", nullptr));
        instructions_2->setText(QCoreApplication::translate("KeyFileEditWidget", "Note: Do NOT use a file that may change as that will prevent you from unlocking your database.", nullptr));
#if QT_CONFIG(accessibility)
        browseKeyFileButton->setAccessibleName(QCoreApplication::translate("KeyFileEditWidget", "Browse for key file", nullptr));
#endif // QT_CONFIG(accessibility)
        browseKeyFileButton->setText(QCoreApplication::translate("KeyFileEditWidget", "Browse\342\200\246", nullptr));
        (void)KeyFileEditWidget;
    } // retranslateUi

};

namespace Ui {
    class KeyFileEditWidget: public Ui_KeyFileEditWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_KEYFILEEDITWIDGET_H
