/********************************************************************************
** Form generated from reading UI file 'ExportDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXPORTDIALOG_H
#define UI_EXPORTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include "gui/MessageWidget.h"

QT_BEGIN_NAMESPACE

class Ui_ExportDialog
{
public:
    QVBoxLayout *verticalLayout;
    MessageWidget *messageWidget;
    QLabel *sortingStrategyLabel;
    QComboBox *sortingStrategy;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ExportDialog)
    {
        if (ExportDialog->objectName().isEmpty())
            ExportDialog->setObjectName(QString::fromUtf8("ExportDialog"));
        ExportDialog->resize(186, 164);
        verticalLayout = new QVBoxLayout(ExportDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        messageWidget = new MessageWidget(ExportDialog);
        messageWidget->setObjectName(QString::fromUtf8("messageWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(messageWidget->sizePolicy().hasHeightForWidth());
        messageWidget->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(messageWidget);

        sortingStrategyLabel = new QLabel(ExportDialog);
        sortingStrategyLabel->setObjectName(QString::fromUtf8("sortingStrategyLabel"));

        verticalLayout->addWidget(sortingStrategyLabel);

        sortingStrategy = new QComboBox(ExportDialog);
        sortingStrategy->setObjectName(QString::fromUtf8("sortingStrategy"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sortingStrategy->sizePolicy().hasHeightForWidth());
        sortingStrategy->setSizePolicy(sizePolicy1);

        verticalLayout->addWidget(sortingStrategy);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(ExportDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);

#if QT_CONFIG(shortcut)
        sortingStrategyLabel->setBuddy(sortingStrategy);
#endif // QT_CONFIG(shortcut)

        retranslateUi(ExportDialog);

        QMetaObject::connectSlotsByName(ExportDialog);
    } // setupUi

    void retranslateUi(QDialog *ExportDialog)
    {
        ExportDialog->setWindowTitle(QCoreApplication::translate("ExportDialog", "Export options", nullptr));
        sortingStrategyLabel->setText(QCoreApplication::translate("ExportDialog", "Sort entries by...", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ExportDialog: public Ui_ExportDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXPORTDIALOG_H
