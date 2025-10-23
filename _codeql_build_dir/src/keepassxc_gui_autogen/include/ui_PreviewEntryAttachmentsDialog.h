/********************************************************************************
** Form generated from reading UI file 'PreviewEntryAttachmentsDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PREVIEWENTRYATTACHMENTSDIALOG_H
#define UI_PREVIEWENTRYATTACHMENTSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>
#include <gui/entry/attachments/AttachmentWidget.h>

QT_BEGIN_NAMESPACE

class Ui_PreviewEntryAttachmentsDialog
{
public:
    QVBoxLayout *verticalLayout;
    AttachmentWidget *attachmentWidget;
    QDialogButtonBox *dialogButtons;

    void setupUi(QDialog *PreviewEntryAttachmentsDialog)
    {
        if (PreviewEntryAttachmentsDialog->objectName().isEmpty())
            PreviewEntryAttachmentsDialog->setObjectName(QString::fromUtf8("PreviewEntryAttachmentsDialog"));
        PreviewEntryAttachmentsDialog->resize(557, 454);
        verticalLayout = new QVBoxLayout(PreviewEntryAttachmentsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(6, 6, 6, 6);
        attachmentWidget = new AttachmentWidget(PreviewEntryAttachmentsDialog);
        attachmentWidget->setObjectName(QString::fromUtf8("attachmentWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(attachmentWidget->sizePolicy().hasHeightForWidth());
        attachmentWidget->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(attachmentWidget);

        dialogButtons = new QDialogButtonBox(PreviewEntryAttachmentsDialog);
        dialogButtons->setObjectName(QString::fromUtf8("dialogButtons"));
        dialogButtons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(dialogButtons);

        verticalLayout->setStretch(0, 1);

        retranslateUi(PreviewEntryAttachmentsDialog);

        QMetaObject::connectSlotsByName(PreviewEntryAttachmentsDialog);
    } // setupUi

    void retranslateUi(QDialog *PreviewEntryAttachmentsDialog)
    {
        PreviewEntryAttachmentsDialog->setWindowTitle(QCoreApplication::translate("PreviewEntryAttachmentsDialog", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PreviewEntryAttachmentsDialog: public Ui_PreviewEntryAttachmentsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PREVIEWENTRYATTACHMENTSDIALOG_H
