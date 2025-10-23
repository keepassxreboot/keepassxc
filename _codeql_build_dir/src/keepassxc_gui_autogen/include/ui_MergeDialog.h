/********************************************************************************
** Form generated from reading UI file 'MergeDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MERGEDIALOG_H
#define UI_MERGEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MergeDialog
{
public:
    QVBoxLayout *verticalLayout;
    QTableWidget *changeTable;
    QDialogButtonBox *buttonBox;

    void setupUi(QWidget *MergeDialog)
    {
        if (MergeDialog->objectName().isEmpty())
            MergeDialog->setObjectName(QString::fromUtf8("MergeDialog"));
        MergeDialog->resize(800, 450);
        verticalLayout = new QVBoxLayout(MergeDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        changeTable = new QTableWidget(MergeDialog);
        changeTable->setObjectName(QString::fromUtf8("changeTable"));

        verticalLayout->addWidget(changeTable);

        buttonBox = new QDialogButtonBox(MergeDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Abort|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(MergeDialog);

        QMetaObject::connectSlotsByName(MergeDialog);
    } // setupUi

    void retranslateUi(QWidget *MergeDialog)
    {
        MergeDialog->setWindowTitle(QCoreApplication::translate("MergeDialog", "Database Merge Confirmation", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MergeDialog: public Ui_MergeDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MERGEDIALOG_H
