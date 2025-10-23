/********************************************************************************
** Form generated from reading UI file 'ImportWizardPageReview.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMPORTWIZARDPAGEREVIEW_H
#define UI_IMPORTWIZARDPAGEREVIEW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QWizardPage>
#include "gui/MessageWidget.h"

QT_BEGIN_NAMESPACE

class Ui_ImportWizardPageReview
{
public:
    QVBoxLayout *verticalLayout;
    MessageWidget *messageWidget;
    QScrollArea *scrollArea;
    QWidget *scrollAreaContents;
    QVBoxLayout *verticalLayout_2;
    QLabel *filenameLabel;
    QLabel *previewLabel;

    void setupUi(QWizardPage *ImportWizardPageReview)
    {
        if (ImportWizardPageReview->objectName().isEmpty())
            ImportWizardPageReview->setObjectName(QString::fromUtf8("ImportWizardPageReview"));
        ImportWizardPageReview->resize(518, 334);
        verticalLayout = new QVBoxLayout(ImportWizardPageReview);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        messageWidget = new MessageWidget(ImportWizardPageReview);
        messageWidget->setObjectName(QString::fromUtf8("messageWidget"));

        verticalLayout->addWidget(messageWidget);

        scrollArea = new QScrollArea(ImportWizardPageReview);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setMinimumSize(QSize(500, 300));
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
        scrollArea->setWidgetResizable(true);
        scrollAreaContents = new QWidget();
        scrollAreaContents->setObjectName(QString::fromUtf8("scrollAreaContents"));
        scrollAreaContents->setGeometry(QRect(0, 0, 498, 298));
        verticalLayout_2 = new QVBoxLayout(scrollAreaContents);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        filenameLabel = new QLabel(scrollAreaContents);
        filenameLabel->setObjectName(QString::fromUtf8("filenameLabel"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        filenameLabel->setFont(font);
        filenameLabel->setText(QString::fromUtf8("filename"));

        verticalLayout_2->addWidget(filenameLabel);

        previewLabel = new QLabel(scrollAreaContents);
        previewLabel->setObjectName(QString::fromUtf8("previewLabel"));
        previewLabel->setText(QString::fromUtf8("Entry count: %1"));

        verticalLayout_2->addWidget(previewLabel);

        scrollArea->setWidget(scrollAreaContents);

        verticalLayout->addWidget(scrollArea);


        retranslateUi(ImportWizardPageReview);

        QMetaObject::connectSlotsByName(ImportWizardPageReview);
    } // setupUi

    void retranslateUi(QWizardPage *ImportWizardPageReview)
    {
        ImportWizardPageReview->setWindowTitle(QCoreApplication::translate("ImportWizardPageReview", "WizardPage", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImportWizardPageReview: public Ui_ImportWizardPageReview {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMPORTWIZARDPAGEREVIEW_H
