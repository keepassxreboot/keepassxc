/********************************************************************************
** Form generated from reading UI file 'TotpSetupDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TOTPSETUPDIALOG_H
#define UI_TOTPSETUPDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_TotpSetupDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *invalidKeyLabel;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_3;
    QLineEdit *seedEdit;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_3;
    QRadioButton *radioDefault;
    QRadioButton *radioSteam;
    QRadioButton *radioCustom;
    QGroupBox *customSettingsGroup;
    QFormLayout *formLayout_3;
    QLabel *algorithmLabel;
    QComboBox *algorithmComboBox;
    QLabel *stepLabel;
    QSpinBox *stepSpinBox;
    QLabel *digitsLabel;
    QSpinBox *digitsSpinBox;
    QDialogButtonBox *buttonBox;
    QButtonGroup *settingsButtonGroup;

    void setupUi(QDialog *TotpSetupDialog)
    {
        if (TotpSetupDialog->objectName().isEmpty())
            TotpSetupDialog->setObjectName(QString::fromUtf8("TotpSetupDialog"));
        TotpSetupDialog->resize(249, 278);
        verticalLayout = new QVBoxLayout(TotpSetupDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        invalidKeyLabel = new QLabel(TotpSetupDialog);
        invalidKeyLabel->setObjectName(QString::fromUtf8("invalidKeyLabel"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        invalidKeyLabel->setFont(font);
        invalidKeyLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(invalidKeyLabel);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(5, -1, 5, -1);
        label_3 = new QLabel(TotpSetupDialog);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        horizontalLayout_2->addWidget(label_3);

        seedEdit = new QLineEdit(TotpSetupDialog);
        seedEdit->setObjectName(QString::fromUtf8("seedEdit"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(seedEdit->sizePolicy().hasHeightForWidth());
        seedEdit->setSizePolicy(sizePolicy);
        seedEdit->setMinimumSize(QSize(150, 0));

        horizontalLayout_2->addWidget(seedEdit);


        verticalLayout->addLayout(horizontalLayout_2);

        groupBox = new QGroupBox(TotpSetupDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setStyleSheet(QString::fromUtf8("border:none"));
        groupBox->setFlat(false);
        groupBox->setCheckable(false);
        verticalLayout_3 = new QVBoxLayout(groupBox);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        radioDefault = new QRadioButton(groupBox);
        settingsButtonGroup = new QButtonGroup(TotpSetupDialog);
        settingsButtonGroup->setObjectName(QString::fromUtf8("settingsButtonGroup"));
        settingsButtonGroup->addButton(radioDefault);
        radioDefault->setObjectName(QString::fromUtf8("radioDefault"));
        radioDefault->setChecked(true);

        verticalLayout_3->addWidget(radioDefault);

        radioSteam = new QRadioButton(groupBox);
        settingsButtonGroup->addButton(radioSteam);
        radioSteam->setObjectName(QString::fromUtf8("radioSteam"));

        verticalLayout_3->addWidget(radioSteam);

        radioCustom = new QRadioButton(groupBox);
        settingsButtonGroup->addButton(radioCustom);
        radioCustom->setObjectName(QString::fromUtf8("radioCustom"));

        verticalLayout_3->addWidget(radioCustom);


        verticalLayout->addWidget(groupBox);

        customSettingsGroup = new QGroupBox(TotpSetupDialog);
        customSettingsGroup->setObjectName(QString::fromUtf8("customSettingsGroup"));
        customSettingsGroup->setEnabled(false);
        formLayout_3 = new QFormLayout(customSettingsGroup);
        formLayout_3->setObjectName(QString::fromUtf8("formLayout_3"));
        formLayout_3->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout_3->setRowWrapPolicy(QFormLayout::DontWrapRows);
        formLayout_3->setLabelAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);
        formLayout_3->setHorizontalSpacing(7);
        formLayout_3->setVerticalSpacing(7);
        formLayout_3->setContentsMargins(20, -1, 20, -1);
        algorithmLabel = new QLabel(customSettingsGroup);
        algorithmLabel->setObjectName(QString::fromUtf8("algorithmLabel"));

        formLayout_3->setWidget(1, QFormLayout::LabelRole, algorithmLabel);

        algorithmComboBox = new QComboBox(customSettingsGroup);
        algorithmComboBox->setObjectName(QString::fromUtf8("algorithmComboBox"));

        formLayout_3->setWidget(1, QFormLayout::FieldRole, algorithmComboBox);

        stepLabel = new QLabel(customSettingsGroup);
        stepLabel->setObjectName(QString::fromUtf8("stepLabel"));

        formLayout_3->setWidget(2, QFormLayout::LabelRole, stepLabel);

        stepSpinBox = new QSpinBox(customSettingsGroup);
        stepSpinBox->setObjectName(QString::fromUtf8("stepSpinBox"));
        stepSpinBox->setMinimum(1);
        stepSpinBox->setMaximum(86400);
        stepSpinBox->setValue(30);

        formLayout_3->setWidget(2, QFormLayout::FieldRole, stepSpinBox);

        digitsLabel = new QLabel(customSettingsGroup);
        digitsLabel->setObjectName(QString::fromUtf8("digitsLabel"));

        formLayout_3->setWidget(3, QFormLayout::LabelRole, digitsLabel);

        digitsSpinBox = new QSpinBox(customSettingsGroup);
        digitsSpinBox->setObjectName(QString::fromUtf8("digitsSpinBox"));
        digitsSpinBox->setMinimum(6);
        digitsSpinBox->setMaximum(10);
        digitsSpinBox->setSingleStep(1);

        formLayout_3->setWidget(3, QFormLayout::FieldRole, digitsSpinBox);


        verticalLayout->addWidget(customSettingsGroup);

        buttonBox = new QDialogButtonBox(TotpSetupDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);

        customSettingsGroup->raise();
        buttonBox->raise();
        groupBox->raise();
        invalidKeyLabel->raise();
        QWidget::setTabOrder(seedEdit, radioDefault);
        QWidget::setTabOrder(radioDefault, radioSteam);
        QWidget::setTabOrder(radioSteam, radioCustom);
        QWidget::setTabOrder(radioCustom, algorithmComboBox);
        QWidget::setTabOrder(algorithmComboBox, stepSpinBox);
        QWidget::setTabOrder(stepSpinBox, digitsSpinBox);

        retranslateUi(TotpSetupDialog);

        QMetaObject::connectSlotsByName(TotpSetupDialog);
    } // setupUi

    void retranslateUi(QDialog *TotpSetupDialog)
    {
        TotpSetupDialog->setWindowTitle(QCoreApplication::translate("TotpSetupDialog", "Setup TOTP", nullptr));
        invalidKeyLabel->setText(QCoreApplication::translate("TotpSetupDialog", "Error: secret key is invalid", nullptr));
        label_3->setText(QCoreApplication::translate("TotpSetupDialog", "Secret Key:", nullptr));
#if QT_CONFIG(tooltip)
        seedEdit->setToolTip(QCoreApplication::translate("TotpSetupDialog", "Secret key must be in Base32 format", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        seedEdit->setAccessibleName(QCoreApplication::translate("TotpSetupDialog", "Secret key field", nullptr));
#endif // QT_CONFIG(accessibility)
        groupBox->setTitle(QString());
        radioDefault->setText(QCoreApplication::translate("TotpSetupDialog", "Default settings (RFC 6238)", nullptr));
        radioSteam->setText(QCoreApplication::translate("TotpSetupDialog", "Steam\302\256 settings", nullptr));
        radioCustom->setText(QCoreApplication::translate("TotpSetupDialog", "Custom settings:", nullptr));
        customSettingsGroup->setTitle(QCoreApplication::translate("TotpSetupDialog", "Custom Settings", nullptr));
        algorithmLabel->setText(QCoreApplication::translate("TotpSetupDialog", "Algorithm:", nullptr));
        stepLabel->setText(QCoreApplication::translate("TotpSetupDialog", "Time step:", nullptr));
#if QT_CONFIG(accessibility)
        stepSpinBox->setAccessibleName(QCoreApplication::translate("TotpSetupDialog", "Time step field", nullptr));
#endif // QT_CONFIG(accessibility)
        stepSpinBox->setSuffix(QCoreApplication::translate("TotpSetupDialog", " sec", "Seconds"));
        digitsLabel->setText(QCoreApplication::translate("TotpSetupDialog", "Code size:", nullptr));
        digitsSpinBox->setSuffix(QCoreApplication::translate("TotpSetupDialog", " digits", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TotpSetupDialog: public Ui_TotpSetupDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TOTPSETUPDIALOG_H
