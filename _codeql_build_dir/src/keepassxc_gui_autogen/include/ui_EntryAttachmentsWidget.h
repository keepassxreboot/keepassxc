/********************************************************************************
** Form generated from reading UI file 'EntryAttachmentsWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ENTRYATTACHMENTSWIDGET_H
#define UI_ENTRYATTACHMENTSWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EntryAttachmentsWidget
{
public:
    QHBoxLayout *horizontalLayout;
    QTableView *attachmentsView;
    QWidget *actionsWidget;
    QVBoxLayout *verticalLayout;
    QPushButton *addAttachmentButton;
    QPushButton *editAttachmentButton;
    QFrame *editPreviewLine;
    QPushButton *previewAttachmentButton;
    QPushButton *openAttachmentButton;
    QPushButton *saveAttachmentButton;
    QFrame *previewRemoveLine;
    QPushButton *removeAttachmentButton;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *EntryAttachmentsWidget)
    {
        if (EntryAttachmentsWidget->objectName().isEmpty())
            EntryAttachmentsWidget->setObjectName(QString::fromUtf8("EntryAttachmentsWidget"));
        EntryAttachmentsWidget->resize(332, 312);
        horizontalLayout = new QHBoxLayout(EntryAttachmentsWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        attachmentsView = new QTableView(EntryAttachmentsWidget);
        attachmentsView->setObjectName(QString::fromUtf8("attachmentsView"));
        attachmentsView->setEditTriggers(QAbstractItemView::EditKeyPressed|QAbstractItemView::SelectedClicked);
        attachmentsView->setSelectionBehavior(QAbstractItemView::SelectRows);
        attachmentsView->setCornerButtonEnabled(false);
        attachmentsView->verticalHeader()->setVisible(false);

        horizontalLayout->addWidget(attachmentsView);

        actionsWidget = new QWidget(EntryAttachmentsWidget);
        actionsWidget->setObjectName(QString::fromUtf8("actionsWidget"));
        verticalLayout = new QVBoxLayout(actionsWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        addAttachmentButton = new QPushButton(actionsWidget);
        addAttachmentButton->setObjectName(QString::fromUtf8("addAttachmentButton"));
        addAttachmentButton->setEnabled(false);

        verticalLayout->addWidget(addAttachmentButton);

        editAttachmentButton = new QPushButton(actionsWidget);
        editAttachmentButton->setObjectName(QString::fromUtf8("editAttachmentButton"));
        editAttachmentButton->setEnabled(false);

        verticalLayout->addWidget(editAttachmentButton);

        editPreviewLine = new QFrame(actionsWidget);
        editPreviewLine->setObjectName(QString::fromUtf8("editPreviewLine"));
        editPreviewLine->setFrameShape(QFrame::HLine);
        editPreviewLine->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(editPreviewLine);

        previewAttachmentButton = new QPushButton(actionsWidget);
        previewAttachmentButton->setObjectName(QString::fromUtf8("previewAttachmentButton"));
        previewAttachmentButton->setEnabled(false);

        verticalLayout->addWidget(previewAttachmentButton);

        openAttachmentButton = new QPushButton(actionsWidget);
        openAttachmentButton->setObjectName(QString::fromUtf8("openAttachmentButton"));
        openAttachmentButton->setEnabled(false);

        verticalLayout->addWidget(openAttachmentButton);

        saveAttachmentButton = new QPushButton(actionsWidget);
        saveAttachmentButton->setObjectName(QString::fromUtf8("saveAttachmentButton"));
        saveAttachmentButton->setEnabled(false);

        verticalLayout->addWidget(saveAttachmentButton);

        previewRemoveLine = new QFrame(actionsWidget);
        previewRemoveLine->setObjectName(QString::fromUtf8("previewRemoveLine"));
        previewRemoveLine->setEnabled(true);
        previewRemoveLine->setFrameShape(QFrame::HLine);
        previewRemoveLine->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(previewRemoveLine);

        removeAttachmentButton = new QPushButton(actionsWidget);
        removeAttachmentButton->setObjectName(QString::fromUtf8("removeAttachmentButton"));
        removeAttachmentButton->setEnabled(false);

        verticalLayout->addWidget(removeAttachmentButton);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        horizontalLayout->addWidget(actionsWidget);


        retranslateUi(EntryAttachmentsWidget);

        QMetaObject::connectSlotsByName(EntryAttachmentsWidget);
    } // setupUi

    void retranslateUi(QWidget *EntryAttachmentsWidget)
    {
        EntryAttachmentsWidget->setWindowTitle(QCoreApplication::translate("EntryAttachmentsWidget", "Form", nullptr));
#if QT_CONFIG(accessibility)
        attachmentsView->setAccessibleName(QCoreApplication::translate("EntryAttachmentsWidget", "Attachments", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        addAttachmentButton->setAccessibleName(QCoreApplication::translate("EntryAttachmentsWidget", "Add new attachment", nullptr));
#endif // QT_CONFIG(accessibility)
        addAttachmentButton->setText(QCoreApplication::translate("EntryAttachmentsWidget", "Add file\342\200\246", nullptr));
        editAttachmentButton->setText(QCoreApplication::translate("EntryAttachmentsWidget", "Edit", nullptr));
        previewAttachmentButton->setText(QCoreApplication::translate("EntryAttachmentsWidget", "Preview", nullptr));
#if QT_CONFIG(accessibility)
        openAttachmentButton->setAccessibleName(QCoreApplication::translate("EntryAttachmentsWidget", "Open selected attachment", nullptr));
#endif // QT_CONFIG(accessibility)
        openAttachmentButton->setText(QCoreApplication::translate("EntryAttachmentsWidget", "Open", nullptr));
#if QT_CONFIG(accessibility)
        saveAttachmentButton->setAccessibleName(QCoreApplication::translate("EntryAttachmentsWidget", "Save selected attachment to disk", nullptr));
#endif // QT_CONFIG(accessibility)
        saveAttachmentButton->setText(QCoreApplication::translate("EntryAttachmentsWidget", "Save\342\200\246", nullptr));
#if QT_CONFIG(accessibility)
        removeAttachmentButton->setAccessibleName(QCoreApplication::translate("EntryAttachmentsWidget", "Remove selected attachment", nullptr));
#endif // QT_CONFIG(accessibility)
        removeAttachmentButton->setText(QCoreApplication::translate("EntryAttachmentsWidget", "Remove", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EntryAttachmentsWidget: public Ui_EntryAttachmentsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ENTRYATTACHMENTSWIDGET_H
