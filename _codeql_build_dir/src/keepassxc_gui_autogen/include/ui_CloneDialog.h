/********************************************************************************
** Form generated from reading UI file 'CloneDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CLONEDIALOG_H
#define UI_CLONEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_CloneDialog
{
public:
    QVBoxLayout *verticalLayout;
    QCheckBox *titleClone;
    QCheckBox *referencesClone;
    QCheckBox *historyClone;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CloneDialog)
    {
        if (CloneDialog->objectName().isEmpty())
            CloneDialog->setObjectName(QString::fromUtf8("CloneDialog"));
        CloneDialog->resize(319, 132);
        CloneDialog->setModal(true);
        verticalLayout = new QVBoxLayout(CloneDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        titleClone = new QCheckBox(CloneDialog);
        titleClone->setObjectName(QString::fromUtf8("titleClone"));
        titleClone->setChecked(true);

        verticalLayout->addWidget(titleClone);

        referencesClone = new QCheckBox(CloneDialog);
        referencesClone->setObjectName(QString::fromUtf8("referencesClone"));

        verticalLayout->addWidget(referencesClone);

        historyClone = new QCheckBox(CloneDialog);
        historyClone->setObjectName(QString::fromUtf8("historyClone"));
        historyClone->setChecked(true);

        verticalLayout->addWidget(historyClone);

        verticalSpacer = new QSpacerItem(20, 6, QSizePolicy::Minimum, QSizePolicy::Minimum);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(CloneDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CloneDialog);

        QMetaObject::connectSlotsByName(CloneDialog);
    } // setupUi

    void retranslateUi(QDialog *CloneDialog)
    {
        CloneDialog->setWindowTitle(QCoreApplication::translate("CloneDialog", "Clone Entry Options", nullptr));
        titleClone->setText(QCoreApplication::translate("CloneDialog", "Append ' - Clone' to title", nullptr));
        referencesClone->setText(QCoreApplication::translate("CloneDialog", "Replace username and password with references", nullptr));
        historyClone->setText(QCoreApplication::translate("CloneDialog", "Copy history", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CloneDialog: public Ui_CloneDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CLONEDIALOG_H
