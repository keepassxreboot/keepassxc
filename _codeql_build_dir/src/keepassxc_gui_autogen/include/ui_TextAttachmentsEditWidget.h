/********************************************************************************
** Form generated from reading UI file 'TextAttachmentsEditWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEXTATTACHMENTSEDITWIDGET_H
#define UI_TEXTATTACHMENTSEDITWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TextAttachmentsEditWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QPushButton *previewPushButton;
    QSpacerItem *horizontalSpacer;
    QTextEdit *attachmentsTextEdit;

    void setupUi(QWidget *TextAttachmentsEditWidget)
    {
        if (TextAttachmentsEditWidget->objectName().isEmpty())
            TextAttachmentsEditWidget->setObjectName(QString::fromUtf8("TextAttachmentsEditWidget"));
        TextAttachmentsEditWidget->resize(400, 300);
        verticalLayout = new QVBoxLayout(TextAttachmentsEditWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        previewPushButton = new QPushButton(TextAttachmentsEditWidget);
        previewPushButton->setObjectName(QString::fromUtf8("previewPushButton"));
        previewPushButton->setCheckable(false);
        previewPushButton->setChecked(false);

        horizontalLayout->addWidget(previewPushButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        attachmentsTextEdit = new QTextEdit(TextAttachmentsEditWidget);
        attachmentsTextEdit->setObjectName(QString::fromUtf8("attachmentsTextEdit"));
        attachmentsTextEdit->setTabStopDistance(10.000000000000000);

        verticalLayout->addWidget(attachmentsTextEdit);


        retranslateUi(TextAttachmentsEditWidget);

        QMetaObject::connectSlotsByName(TextAttachmentsEditWidget);
    } // setupUi

    void retranslateUi(QWidget *TextAttachmentsEditWidget)
    {
        TextAttachmentsEditWidget->setWindowTitle(QString());
        previewPushButton->setText(QCoreApplication::translate("TextAttachmentsEditWidget", "Preview", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TextAttachmentsEditWidget: public Ui_TextAttachmentsEditWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TEXTATTACHMENTSEDITWIDGET_H
