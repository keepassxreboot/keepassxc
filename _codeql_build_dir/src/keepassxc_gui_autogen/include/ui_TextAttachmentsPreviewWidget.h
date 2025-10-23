/********************************************************************************
** Form generated from reading UI file 'TextAttachmentsPreviewWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEXTATTACHMENTSPREVIEWWIDGET_H
#define UI_TEXTATTACHMENTSPREVIEWWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TextAttachmentsPreviewWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *typeLabel;
    QComboBox *typeComboBox;
    QSpacerItem *horizontalSpacer;
    QTextBrowser *previewTextBrowser;

    void setupUi(QWidget *TextAttachmentsPreviewWidget)
    {
        if (TextAttachmentsPreviewWidget->objectName().isEmpty())
            TextAttachmentsPreviewWidget->setObjectName(QString::fromUtf8("TextAttachmentsPreviewWidget"));
        TextAttachmentsPreviewWidget->resize(400, 300);
        verticalLayout = new QVBoxLayout(TextAttachmentsPreviewWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        typeLabel = new QLabel(TextAttachmentsPreviewWidget);
        typeLabel->setObjectName(QString::fromUtf8("typeLabel"));

        horizontalLayout->addWidget(typeLabel);

        typeComboBox = new QComboBox(TextAttachmentsPreviewWidget);
        typeComboBox->setObjectName(QString::fromUtf8("typeComboBox"));
        typeComboBox->setEditable(false);

        horizontalLayout->addWidget(typeComboBox);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        previewTextBrowser = new QTextBrowser(TextAttachmentsPreviewWidget);
        previewTextBrowser->setObjectName(QString::fromUtf8("previewTextBrowser"));
        previewTextBrowser->setTabStopDistance(10.000000000000000);

        verticalLayout->addWidget(previewTextBrowser);


        retranslateUi(TextAttachmentsPreviewWidget);

        QMetaObject::connectSlotsByName(TextAttachmentsPreviewWidget);
    } // setupUi

    void retranslateUi(QWidget *TextAttachmentsPreviewWidget)
    {
        TextAttachmentsPreviewWidget->setWindowTitle(QCoreApplication::translate("TextAttachmentsPreviewWidget", "Form", nullptr));
        typeLabel->setText(QCoreApplication::translate("TextAttachmentsPreviewWidget", "Type:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TextAttachmentsPreviewWidget: public Ui_TextAttachmentsPreviewWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TEXTATTACHMENTSPREVIEWWIDGET_H
