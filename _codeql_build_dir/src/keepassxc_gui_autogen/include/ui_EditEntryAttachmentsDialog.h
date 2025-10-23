/********************************************************************************
** Form generated from reading UI file 'EditEntryAttachmentsDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYATTACHMENTSDIALOG_H
#define UI_EDITENTRYATTACHMENTSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QVBoxLayout>
#include <gui/entry/attachments/AttachmentWidget.h>

QT_BEGIN_NAMESPACE

class Ui_EditEntryAttachmentsDialog
{
public:
    QVBoxLayout *verticalLayout;
    AttachmentWidget *attachmentWidget;
    QDialogButtonBox *dialogButtons;

    void setupUi(QDialog *EditEntryAttachmentsDialog)
    {
        if (EditEntryAttachmentsDialog->objectName().isEmpty())
            EditEntryAttachmentsDialog->setObjectName(QString::fromUtf8("EditEntryAttachmentsDialog"));
        EditEntryAttachmentsDialog->resize(447, 424);
        EditEntryAttachmentsDialog->setWindowTitle(QString::fromUtf8(""));
        verticalLayout = new QVBoxLayout(EditEntryAttachmentsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        attachmentWidget = new AttachmentWidget(EditEntryAttachmentsDialog);
        attachmentWidget->setObjectName(QString::fromUtf8("attachmentWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(attachmentWidget->sizePolicy().hasHeightForWidth());
        attachmentWidget->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(attachmentWidget);

        dialogButtons = new QDialogButtonBox(EditEntryAttachmentsDialog);
        dialogButtons->setObjectName(QString::fromUtf8("dialogButtons"));
        dialogButtons->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(dialogButtons);


        retranslateUi(EditEntryAttachmentsDialog);

        QMetaObject::connectSlotsByName(EditEntryAttachmentsDialog);
    } // setupUi

    void retranslateUi(QDialog *EditEntryAttachmentsDialog)
    {
        (void)EditEntryAttachmentsDialog;
    } // retranslateUi

};

namespace Ui {
    class EditEntryAttachmentsDialog: public Ui_EditEntryAttachmentsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYATTACHMENTSDIALOG_H
