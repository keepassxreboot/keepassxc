/********************************************************************************
** Form generated from reading UI file 'DatabaseSettingsWidgetGeneral.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATABASESETTINGSWIDGETGENERAL_H
#define UI_DATABASESETTINGSWIDGETGENERAL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DatabaseSettingsWidgetGeneral
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QFormLayout *formLayout;
    QLabel *dbNameLabel;
    QLineEdit *dbNameEdit;
    QLabel *dbDescriptionLabel;
    QLineEdit *dbDescriptionEdit;
    QLabel *defaultUsernameLabel;
    QLineEdit *defaultUsernameEdit;
    QGroupBox *groupBox_4;
    QVBoxLayout *verticalLayout_3;
    QLabel *warningLabel;
    QFormLayout *formLayout_2;
    QLabel *dbPublicSummaryLabel;
    QLineEdit *dbPublicName;
    QLabel *dbPublicColorLabel;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *dbPublicColorButton;
    QPushButton *dbPublicColorClearButton;
    QSpacerItem *horizontalSpacer_3;
    QLabel *dbPublicIconLabel;
    QHBoxLayout *horizontalLayout_4;
    QPushButton *dbPublicIconButton;
    QPushButton *dbPublicIconClearButton;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *groupBox_2;
    QHBoxLayout *horizontalLayout;
    QGridLayout *gridLayout;
    QCheckBox *historyMaxItemsCheckBox;
    QCheckBox *recycleBinEnabledCheckBox;
    QCheckBox *historyMaxSizeCheckBox;
    QSpinBox *historyMaxSizeSpinBox;
    QSpinBox *historyMaxItemsSpinBox;
    QSpacerItem *horizontalSpacer;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *compressionCheckbox;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *autosaveDelayCheckBox;
    QSpinBox *autosaveDelaySpinBox;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *DatabaseSettingsWidgetGeneral)
    {
        if (DatabaseSettingsWidgetGeneral->objectName().isEmpty())
            DatabaseSettingsWidgetGeneral->setObjectName(QString::fromUtf8("DatabaseSettingsWidgetGeneral"));
        DatabaseSettingsWidgetGeneral->resize(453, 647);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DatabaseSettingsWidgetGeneral->sizePolicy().hasHeightForWidth());
        DatabaseSettingsWidgetGeneral->setSizePolicy(sizePolicy);
        DatabaseSettingsWidgetGeneral->setMinimumSize(QSize(450, 0));
        verticalLayout = new QVBoxLayout(DatabaseSettingsWidgetGeneral);
        verticalLayout->setSpacing(20);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(DatabaseSettingsWidgetGeneral);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        formLayout = new QFormLayout(groupBox);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        dbNameLabel = new QLabel(groupBox);
        dbNameLabel->setObjectName(QString::fromUtf8("dbNameLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, dbNameLabel);

        dbNameEdit = new QLineEdit(groupBox);
        dbNameEdit->setObjectName(QString::fromUtf8("dbNameEdit"));

        formLayout->setWidget(0, QFormLayout::FieldRole, dbNameEdit);

        dbDescriptionLabel = new QLabel(groupBox);
        dbDescriptionLabel->setObjectName(QString::fromUtf8("dbDescriptionLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, dbDescriptionLabel);

        dbDescriptionEdit = new QLineEdit(groupBox);
        dbDescriptionEdit->setObjectName(QString::fromUtf8("dbDescriptionEdit"));

        formLayout->setWidget(1, QFormLayout::FieldRole, dbDescriptionEdit);

        defaultUsernameLabel = new QLabel(groupBox);
        defaultUsernameLabel->setObjectName(QString::fromUtf8("defaultUsernameLabel"));

        formLayout->setWidget(2, QFormLayout::LabelRole, defaultUsernameLabel);

        defaultUsernameEdit = new QLineEdit(groupBox);
        defaultUsernameEdit->setObjectName(QString::fromUtf8("defaultUsernameEdit"));
        defaultUsernameEdit->setEnabled(true);

        formLayout->setWidget(2, QFormLayout::FieldRole, defaultUsernameEdit);


        verticalLayout->addWidget(groupBox);

        groupBox_4 = new QGroupBox(DatabaseSettingsWidgetGeneral);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        verticalLayout_3 = new QVBoxLayout(groupBox_4);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        warningLabel = new QLabel(groupBox_4);
        warningLabel->setObjectName(QString::fromUtf8("warningLabel"));
        QFont font;
        font.setItalic(true);
        warningLabel->setFont(font);

        verticalLayout_3->addWidget(warningLabel);

        formLayout_2 = new QFormLayout();
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        formLayout_2->setContentsMargins(-1, 0, -1, -1);
        dbPublicSummaryLabel = new QLabel(groupBox_4);
        dbPublicSummaryLabel->setObjectName(QString::fromUtf8("dbPublicSummaryLabel"));

        formLayout_2->setWidget(0, QFormLayout::LabelRole, dbPublicSummaryLabel);

        dbPublicName = new QLineEdit(groupBox_4);
        dbPublicName->setObjectName(QString::fromUtf8("dbPublicName"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, dbPublicName);

        dbPublicColorLabel = new QLabel(groupBox_4);
        dbPublicColorLabel->setObjectName(QString::fromUtf8("dbPublicColorLabel"));

        formLayout_2->setWidget(1, QFormLayout::LabelRole, dbPublicColorLabel);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        dbPublicColorButton = new QPushButton(groupBox_4);
        dbPublicColorButton->setObjectName(QString::fromUtf8("dbPublicColorButton"));
        dbPublicColorButton->setMaximumSize(QSize(30, 30));

        horizontalLayout_3->addWidget(dbPublicColorButton);

        dbPublicColorClearButton = new QPushButton(groupBox_4);
        dbPublicColorClearButton->setObjectName(QString::fromUtf8("dbPublicColorClearButton"));

        horizontalLayout_3->addWidget(dbPublicColorClearButton);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        formLayout_2->setLayout(1, QFormLayout::FieldRole, horizontalLayout_3);

        dbPublicIconLabel = new QLabel(groupBox_4);
        dbPublicIconLabel->setObjectName(QString::fromUtf8("dbPublicIconLabel"));

        formLayout_2->setWidget(2, QFormLayout::LabelRole, dbPublicIconLabel);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        dbPublicIconButton = new QPushButton(groupBox_4);
        dbPublicIconButton->setObjectName(QString::fromUtf8("dbPublicIconButton"));
        dbPublicIconButton->setMaximumSize(QSize(30, 30));
        dbPublicIconButton->setIconSize(QSize(30, 30));

        horizontalLayout_4->addWidget(dbPublicIconButton);

        dbPublicIconClearButton = new QPushButton(groupBox_4);
        dbPublicIconClearButton->setObjectName(QString::fromUtf8("dbPublicIconClearButton"));

        horizontalLayout_4->addWidget(dbPublicIconClearButton);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_4);


        formLayout_2->setLayout(2, QFormLayout::FieldRole, horizontalLayout_4);


        verticalLayout_3->addLayout(formLayout_2);


        verticalLayout->addWidget(groupBox_4);

        groupBox_2 = new QGroupBox(DatabaseSettingsWidgetGeneral);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        horizontalLayout = new QHBoxLayout(groupBox_2);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        historyMaxItemsCheckBox = new QCheckBox(groupBox_2);
        historyMaxItemsCheckBox->setObjectName(QString::fromUtf8("historyMaxItemsCheckBox"));

        gridLayout->addWidget(historyMaxItemsCheckBox, 0, 0, 1, 1);

        recycleBinEnabledCheckBox = new QCheckBox(groupBox_2);
        recycleBinEnabledCheckBox->setObjectName(QString::fromUtf8("recycleBinEnabledCheckBox"));

        gridLayout->addWidget(recycleBinEnabledCheckBox, 2, 0, 1, 1);

        historyMaxSizeCheckBox = new QCheckBox(groupBox_2);
        historyMaxSizeCheckBox->setObjectName(QString::fromUtf8("historyMaxSizeCheckBox"));

        gridLayout->addWidget(historyMaxSizeCheckBox, 1, 0, 1, 1);

        historyMaxSizeSpinBox = new QSpinBox(groupBox_2);
        historyMaxSizeSpinBox->setObjectName(QString::fromUtf8("historyMaxSizeSpinBox"));
        historyMaxSizeSpinBox->setMinimum(1);
        historyMaxSizeSpinBox->setMaximum(2000000000);

        gridLayout->addWidget(historyMaxSizeSpinBox, 1, 1, 1, 1);

        historyMaxItemsSpinBox = new QSpinBox(groupBox_2);
        historyMaxItemsSpinBox->setObjectName(QString::fromUtf8("historyMaxItemsSpinBox"));
        historyMaxItemsSpinBox->setMaximum(2000000000);

        gridLayout->addWidget(historyMaxItemsSpinBox, 0, 1, 1, 1);


        horizontalLayout->addLayout(gridLayout);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(DatabaseSettingsWidgetGeneral);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_2 = new QVBoxLayout(groupBox_3);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        compressionCheckbox = new QCheckBox(groupBox_3);
        compressionCheckbox->setObjectName(QString::fromUtf8("compressionCheckbox"));
        compressionCheckbox->setChecked(true);

        verticalLayout_2->addWidget(compressionCheckbox);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        autosaveDelayCheckBox = new QCheckBox(groupBox_3);
        autosaveDelayCheckBox->setObjectName(QString::fromUtf8("autosaveDelayCheckBox"));

        horizontalLayout_2->addWidget(autosaveDelayCheckBox);

        autosaveDelaySpinBox = new QSpinBox(groupBox_3);
        autosaveDelaySpinBox->setObjectName(QString::fromUtf8("autosaveDelaySpinBox"));
        autosaveDelaySpinBox->setMinimum(0);
        autosaveDelaySpinBox->setMaximum(420000000);
        autosaveDelaySpinBox->setValue(5);

        horizontalLayout_2->addWidget(autosaveDelaySpinBox);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout_2->addLayout(horizontalLayout_2);


        verticalLayout->addWidget(groupBox_3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(DatabaseSettingsWidgetGeneral);

        QMetaObject::connectSlotsByName(DatabaseSettingsWidgetGeneral);
    } // setupUi

    void retranslateUi(QWidget *DatabaseSettingsWidgetGeneral)
    {
        groupBox->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database Metadata", nullptr));
        dbNameLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database name:", nullptr));
#if QT_CONFIG(accessibility)
        dbNameEdit->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database name field", nullptr));
#endif // QT_CONFIG(accessibility)
        dbDescriptionLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database description:", nullptr));
#if QT_CONFIG(accessibility)
        dbDescriptionEdit->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database description field", nullptr));
#endif // QT_CONFIG(accessibility)
        defaultUsernameLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Default username:", nullptr));
#if QT_CONFIG(accessibility)
        defaultUsernameEdit->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Default username field", nullptr));
#endif // QT_CONFIG(accessibility)
        groupBox_4->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Public Database Metadata", nullptr));
        warningLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Warning: the following settings are not encrypted.", nullptr));
        dbPublicSummaryLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Display name:", nullptr));
#if QT_CONFIG(tooltip)
        dbPublicName->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Publicly visible display name used on the unlock dialog", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        dbPublicName->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database public display name", nullptr));
#endif // QT_CONFIG(accessibility)
        dbPublicColorLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Display color:", nullptr));
#if QT_CONFIG(tooltip)
        dbPublicColorButton->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Publicly visible color used on the unlock dialog", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        dbPublicColorButton->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Database public display color chooser", nullptr));
#endif // QT_CONFIG(accessibility)
        dbPublicColorButton->setText(QString());
        dbPublicColorClearButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Clear", nullptr));
        dbPublicIconLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Display icon:", nullptr));
        dbPublicIconButton->setText(QString());
        dbPublicIconClearButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Clear", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "History Settings", nullptr));
#if QT_CONFIG(tooltip)
        historyMaxItemsCheckBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "When saving this setting or editing an entry\n"
"the oldest history items of an entry will be\n"
"removed such that only the specified amount\n"
"of entries remain at most.", nullptr));
#endif // QT_CONFIG(tooltip)
        historyMaxItemsCheckBox->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Limit the amount of history items per entry to:", nullptr));
#if QT_CONFIG(tooltip)
        recycleBinEnabledCheckBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Move entries to a recycle bin group\n"
"instead of deleting them from the database.\n"
"Entries deleted from the recycle bin are\n"
"removed from the database.", nullptr));
#endif // QT_CONFIG(tooltip)
        recycleBinEnabledCheckBox->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Use recycle bin", nullptr));
#if QT_CONFIG(tooltip)
        historyMaxSizeCheckBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "When saving this setting or editing an entry\n"
"the oldest history items of an entry will be\n"
"removed such that the remaining history items\n"
"add up to the specified amount at most.", nullptr));
#endif // QT_CONFIG(tooltip)
        historyMaxSizeCheckBox->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Limit the total size of history items per entry to:", nullptr));
#if QT_CONFIG(tooltip)
        historyMaxSizeSpinBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Maximum size of history per entry", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        historyMaxSizeSpinBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Maximum size of history per entry", nullptr));
#endif // QT_CONFIG(accessibility)
        historyMaxSizeSpinBox->setSuffix(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", " MiB", nullptr));
#if QT_CONFIG(tooltip)
        historyMaxItemsSpinBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Maximum number of history items per entry", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        historyMaxItemsSpinBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Maximum number of history items per entry", nullptr));
#endif // QT_CONFIG(accessibility)
        groupBox_3->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Additional Database Settings", nullptr));
        compressionCheckbox->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Enable compression (recommended)", nullptr));
#if QT_CONFIG(tooltip)
        autosaveDelayCheckBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Autosave delay since last change", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        autosaveDelayCheckBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Autosave delay since last change checkbox", nullptr));
#endif // QT_CONFIG(accessibility)
        autosaveDelayCheckBox->setText(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Autosave delay", nullptr));
#if QT_CONFIG(tooltip)
        autosaveDelaySpinBox->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Autosave delay since last change in minutes", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        autosaveDelaySpinBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", "Autosave delay since last change in minutes", nullptr));
#endif // QT_CONFIG(accessibility)
        autosaveDelaySpinBox->setSuffix(QCoreApplication::translate("DatabaseSettingsWidgetGeneral", " min", nullptr));
        (void)DatabaseSettingsWidgetGeneral;
    } // retranslateUi

};

namespace Ui {
    class DatabaseSettingsWidgetGeneral: public Ui_DatabaseSettingsWidgetGeneral {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATABASESETTINGSWIDGETGENERAL_H
