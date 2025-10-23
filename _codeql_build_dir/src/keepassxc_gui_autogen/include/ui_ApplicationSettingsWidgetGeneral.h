/********************************************************************************
** Form generated from reading UI file 'ApplicationSettingsWidgetGeneral.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_APPLICATIONSETTINGSWIDGETGENERAL_H
#define UI_APPLICATIONSETTINGSWIDGETGENERAL_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/widgets/ShortcutWidget.h"

QT_BEGIN_NAMESPACE

class Ui_ApplicationSettingsWidgetGeneral
{
public:
    QVBoxLayout *verticalLayout_3;
    QTabWidget *generalSettingsTabWidget;
    QWidget *tabGeneral;
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_8;
    QGroupBox *startupGroup;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *singleInstanceCheckBox;
    QCheckBox *launchAtStartup;
    QCheckBox *systrayMinimizeOnStartup;
    QCheckBox *minimizeAfterUnlockCheckBox;
    QHBoxLayout *horizontalLayout_6;
    QCheckBox *rememberLastDatabasesCheckBox;
    QSpinBox *rememberLastDatabasesSpinbox;
    QSpacerItem *horizontalSpacer_9;
    QHBoxLayout *rememberDbSubLayout_2;
    QSpacerItem *toolbarMovableSpacer_3;
    QCheckBox *openPreviousDatabasesOnStartupCheckBox;
    QHBoxLayout *rememberDbSubLayout;
    QSpacerItem *toolbarMovableSpacer_2;
    QCheckBox *rememberLastKeyFilesCheckBox;
    QCheckBox *checkForUpdatesOnStartupCheckBox;
    QHBoxLayout *checkUpdatesSubLayout;
    QSpacerItem *checkUpdatesSpacer;
    QCheckBox *checkForUpdatesIncludeBetasCheckBox;
    QHBoxLayout *horizontalLayout_5;
    QCheckBox *showExpiredEntriesOnDatabaseUnlockCheckBox;
    QSpinBox *showExpiredEntriesOnDatabaseUnlockOffsetSpinBox;
    QSpacerItem *showExpiredEntriesSpacer;
    QGroupBox *saveGroup;
    QVBoxLayout *verticalLayout_5;
    QCheckBox *autoSaveAfterEveryChangeCheckBox;
    QCheckBox *autoSaveOnExitCheckBox;
    QCheckBox *autoSaveNonDataChangesCheckBox;
    QCheckBox *autoReloadOnChangeCheckBox;
    QCheckBox *backupBeforeSaveCheckBox;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_11;
    QLabel *label;
    QSpacerItem *horizontalSpacer_13;
    QLineEdit *backupFilePath;
    QSpacerItem *horizontalSpacer_12;
    QPushButton *backupFilePathPicker;
    QCheckBox *useAlternativeSaveCheckBox;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_8;
    QComboBox *alternativeSaveComboBox;
    QSpacerItem *horizontalSpacer_7;
    QGroupBox *entryGroup;
    QVBoxLayout *verticalLayout_6;
    QCheckBox *ConfirmMoveEntryToRecycleBinCheckBox;
    QCheckBox *EnableCopyOnDoubleClickCheckBox;
    QCheckBox *openUrlOnDoubleClick;
    QCheckBox *useGroupIconOnEntryCreationCheckBox;
    QCheckBox *minimizeOnOpenUrlCheckBox;
    QCheckBox *hideWindowOnCopyCheckBox;
    QHBoxLayout *hideWindowLayout_1;
    QSpacerItem *hideWindowSpacer_1;
    QRadioButton *minimizeOnCopyRadioButton;
    QHBoxLayout *hideWindowLayout_2;
    QSpacerItem *hideWindowSpacer_2;
    QRadioButton *dropToBackgroundOnCopyRadioButton;
    QHBoxLayout *horizontalLayout;
    QLabel *faviconTimeoutLabel;
    QSpinBox *faviconTimeoutSpinBox;
    QSpacerItem *horizontalSpacer_2;
    QGroupBox *generalGroup;
    QVBoxLayout *verticalLayout_7;
    QGridLayout *gridLayout_3;
    QSpacerItem *horizontalSpacer_5;
    QLabel *languageLabel_2;
    QCheckBox *toolbarMovableCheckBox;
    QComboBox *toolButtonStyleComboBox;
    QComboBox *languageComboBox;
    QLabel *toolButtonStyleLabel;
    QLabel *languageLabel_3;
    QLabel *fontSizeLabel;
    QComboBox *fontSizeComboBox;
    QCheckBox *toolbarShowCheckBox;
    QCheckBox *menubarShowCheckBox;
    QCheckBox *colorPasswordsCheckBox;
    QCheckBox *monospaceNotesCheckBox;
    QCheckBox *minimizeOnCloseCheckBox;
    QCheckBox *systrayShowCheckBox;
    QGridLayout *gridLayout_2;
    QSpacerItem *horizontalSpacer_3;
    QComboBox *trayIconAppearance;
    QLabel *trayIconAppearanceLabel;
    QSpacerItem *horizontalSpacer_4;
    QSpacerItem *verticalSpacer_6;
    QWidget *systraySettings;
    QVBoxLayout *systrayLayout;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QCheckBox *systrayMinimizeToTrayCheckBox;
    QHBoxLayout *resetSettingsSubLayout;
    QPushButton *resetSettingsButton;
    QSpacerItem *horizontalSpacer_10;
    QPushButton *importSettingsButton;
    QPushButton *exportSettingsButton;
    QSpacerItem *verticalSpacer;
    QWidget *tabAutotype;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *autoTypeEntryTitleMatchCheckBox;
    QCheckBox *autoTypeEntryURLMatchCheckBox;
    QCheckBox *autoTypeAskCheckBox;
    QHBoxLayout *autoTypeSkipMainWindowLayout;
    QSpacerItem *autoTypeSkipMainWindowSpacer;
    QCheckBox *autoTypeSkipMainWindowConfirmationCheckBox;
    QCheckBox *autoTypeHideExpiredEntryCheckBox;
    QCheckBox *autoTypeRelockDatabaseCheckBox;
    QSpacerItem *verticalSpacer_4;
    QGridLayout *gridLayout;
    QLabel *autoTypeStartDelayLabel;
    QLabel *autoTypeShortcutLabel;
    QSpinBox *autoTypeStartDelaySpinBox;
    QLabel *autoTypeDelayLabel;
    QSpacerItem *horizontalSpacer_6;
    ShortcutWidget *autoTypeShortcutWidget;
    QSpinBox *autoTypeDelaySpinBox;
    QLabel *autoTypeRetypeLabel;
    QSpinBox *autoTypeRetypeTimeSpinBox;
    QSpacerItem *verticalSpacer_3;

    void setupUi(QWidget *ApplicationSettingsWidgetGeneral)
    {
        if (ApplicationSettingsWidgetGeneral->objectName().isEmpty())
            ApplicationSettingsWidgetGeneral->setObjectName(QString::fromUtf8("ApplicationSettingsWidgetGeneral"));
        ApplicationSettingsWidgetGeneral->resize(605, 969);
        verticalLayout_3 = new QVBoxLayout(ApplicationSettingsWidgetGeneral);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        generalSettingsTabWidget = new QTabWidget(ApplicationSettingsWidgetGeneral);
        generalSettingsTabWidget->setObjectName(QString::fromUtf8("generalSettingsTabWidget"));
        tabGeneral = new QWidget();
        tabGeneral->setObjectName(QString::fromUtf8("tabGeneral"));
        verticalLayout = new QVBoxLayout(tabGeneral);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        scrollArea = new QScrollArea(tabGeneral);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(scrollArea->sizePolicy().hasHeightForWidth());
        scrollArea->setSizePolicy(sizePolicy);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setFrameShadow(QFrame::Plain);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 568, 1202));
        verticalLayout_8 = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout_8->setSpacing(20);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(0, 0, -1, 0);
        startupGroup = new QGroupBox(scrollAreaWidgetContents);
        startupGroup->setObjectName(QString::fromUtf8("startupGroup"));
        verticalLayout_4 = new QVBoxLayout(startupGroup);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        singleInstanceCheckBox = new QCheckBox(startupGroup);
        singleInstanceCheckBox->setObjectName(QString::fromUtf8("singleInstanceCheckBox"));
        singleInstanceCheckBox->setChecked(true);

        verticalLayout_4->addWidget(singleInstanceCheckBox);

        launchAtStartup = new QCheckBox(startupGroup);
        launchAtStartup->setObjectName(QString::fromUtf8("launchAtStartup"));

        verticalLayout_4->addWidget(launchAtStartup);

        systrayMinimizeOnStartup = new QCheckBox(startupGroup);
        systrayMinimizeOnStartup->setObjectName(QString::fromUtf8("systrayMinimizeOnStartup"));

        verticalLayout_4->addWidget(systrayMinimizeOnStartup);

        minimizeAfterUnlockCheckBox = new QCheckBox(startupGroup);
        minimizeAfterUnlockCheckBox->setObjectName(QString::fromUtf8("minimizeAfterUnlockCheckBox"));

        verticalLayout_4->addWidget(minimizeAfterUnlockCheckBox);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        rememberLastDatabasesCheckBox = new QCheckBox(startupGroup);
        rememberLastDatabasesCheckBox->setObjectName(QString::fromUtf8("rememberLastDatabasesCheckBox"));
        rememberLastDatabasesCheckBox->setChecked(true);

        horizontalLayout_6->addWidget(rememberLastDatabasesCheckBox);

        rememberLastDatabasesSpinbox = new QSpinBox(startupGroup);
        rememberLastDatabasesSpinbox->setObjectName(QString::fromUtf8("rememberLastDatabasesSpinbox"));
        rememberLastDatabasesSpinbox->setMinimum(1);
        rememberLastDatabasesSpinbox->setMaximum(25);
        rememberLastDatabasesSpinbox->setValue(5);

        horizontalLayout_6->addWidget(rememberLastDatabasesSpinbox);

        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_9);


        verticalLayout_4->addLayout(horizontalLayout_6);

        rememberDbSubLayout_2 = new QHBoxLayout();
        rememberDbSubLayout_2->setSpacing(0);
        rememberDbSubLayout_2->setObjectName(QString::fromUtf8("rememberDbSubLayout_2"));
        rememberDbSubLayout_2->setSizeConstraint(QLayout::SetMaximumSize);
        toolbarMovableSpacer_3 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        rememberDbSubLayout_2->addItem(toolbarMovableSpacer_3);

        openPreviousDatabasesOnStartupCheckBox = new QCheckBox(startupGroup);
        openPreviousDatabasesOnStartupCheckBox->setObjectName(QString::fromUtf8("openPreviousDatabasesOnStartupCheckBox"));
        openPreviousDatabasesOnStartupCheckBox->setChecked(true);

        rememberDbSubLayout_2->addWidget(openPreviousDatabasesOnStartupCheckBox);


        verticalLayout_4->addLayout(rememberDbSubLayout_2);

        rememberDbSubLayout = new QHBoxLayout();
        rememberDbSubLayout->setSpacing(0);
        rememberDbSubLayout->setObjectName(QString::fromUtf8("rememberDbSubLayout"));
        rememberDbSubLayout->setSizeConstraint(QLayout::SetMaximumSize);
        toolbarMovableSpacer_2 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        rememberDbSubLayout->addItem(toolbarMovableSpacer_2);

        rememberLastKeyFilesCheckBox = new QCheckBox(startupGroup);
        rememberLastKeyFilesCheckBox->setObjectName(QString::fromUtf8("rememberLastKeyFilesCheckBox"));
        rememberLastKeyFilesCheckBox->setChecked(true);

        rememberDbSubLayout->addWidget(rememberLastKeyFilesCheckBox);


        verticalLayout_4->addLayout(rememberDbSubLayout);

        checkForUpdatesOnStartupCheckBox = new QCheckBox(startupGroup);
        checkForUpdatesOnStartupCheckBox->setObjectName(QString::fromUtf8("checkForUpdatesOnStartupCheckBox"));

        verticalLayout_4->addWidget(checkForUpdatesOnStartupCheckBox);

        checkUpdatesSubLayout = new QHBoxLayout();
        checkUpdatesSubLayout->setSpacing(0);
        checkUpdatesSubLayout->setObjectName(QString::fromUtf8("checkUpdatesSubLayout"));
        checkUpdatesSpacer = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        checkUpdatesSubLayout->addItem(checkUpdatesSpacer);

        checkForUpdatesIncludeBetasCheckBox = new QCheckBox(startupGroup);
        checkForUpdatesIncludeBetasCheckBox->setObjectName(QString::fromUtf8("checkForUpdatesIncludeBetasCheckBox"));
        checkForUpdatesIncludeBetasCheckBox->setEnabled(false);

        checkUpdatesSubLayout->addWidget(checkForUpdatesIncludeBetasCheckBox);


        verticalLayout_4->addLayout(checkUpdatesSubLayout);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalLayout_5->setSizeConstraint(QLayout::SetMaximumSize);
        showExpiredEntriesOnDatabaseUnlockCheckBox = new QCheckBox(startupGroup);
        showExpiredEntriesOnDatabaseUnlockCheckBox->setObjectName(QString::fromUtf8("showExpiredEntriesOnDatabaseUnlockCheckBox"));

        horizontalLayout_5->addWidget(showExpiredEntriesOnDatabaseUnlockCheckBox);

        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox = new QSpinBox(startupGroup);
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setObjectName(QString::fromUtf8("showExpiredEntriesOnDatabaseUnlockOffsetSpinBox"));
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setEnabled(true);
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setFocusPolicy(Qt::StrongFocus);
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setMinimum(0);
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setMaximum(90);
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setValue(3);

        horizontalLayout_5->addWidget(showExpiredEntriesOnDatabaseUnlockOffsetSpinBox);

        showExpiredEntriesSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(showExpiredEntriesSpacer);


        verticalLayout_4->addLayout(horizontalLayout_5);


        verticalLayout_8->addWidget(startupGroup);

        saveGroup = new QGroupBox(scrollAreaWidgetContents);
        saveGroup->setObjectName(QString::fromUtf8("saveGroup"));
        verticalLayout_5 = new QVBoxLayout(saveGroup);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        autoSaveAfterEveryChangeCheckBox = new QCheckBox(saveGroup);
        autoSaveAfterEveryChangeCheckBox->setObjectName(QString::fromUtf8("autoSaveAfterEveryChangeCheckBox"));

        verticalLayout_5->addWidget(autoSaveAfterEveryChangeCheckBox);

        autoSaveOnExitCheckBox = new QCheckBox(saveGroup);
        autoSaveOnExitCheckBox->setObjectName(QString::fromUtf8("autoSaveOnExitCheckBox"));

        verticalLayout_5->addWidget(autoSaveOnExitCheckBox);

        autoSaveNonDataChangesCheckBox = new QCheckBox(saveGroup);
        autoSaveNonDataChangesCheckBox->setObjectName(QString::fromUtf8("autoSaveNonDataChangesCheckBox"));

        verticalLayout_5->addWidget(autoSaveNonDataChangesCheckBox);

        autoReloadOnChangeCheckBox = new QCheckBox(saveGroup);
        autoReloadOnChangeCheckBox->setObjectName(QString::fromUtf8("autoReloadOnChangeCheckBox"));

        verticalLayout_5->addWidget(autoReloadOnChangeCheckBox);

        backupBeforeSaveCheckBox = new QCheckBox(saveGroup);
        backupBeforeSaveCheckBox->setObjectName(QString::fromUtf8("backupBeforeSaveCheckBox"));

        verticalLayout_5->addWidget(backupBeforeSaveCheckBox);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalSpacer_11 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_11);

        label = new QLabel(saveGroup);
        label->setObjectName(QString::fromUtf8("label"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy1);

        horizontalLayout_4->addWidget(label);

        horizontalSpacer_13 = new QSpacerItem(6, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_13);

        backupFilePath = new QLineEdit(saveGroup);
        backupFilePath->setObjectName(QString::fromUtf8("backupFilePath"));
        backupFilePath->setEnabled(false);

        horizontalLayout_4->addWidget(backupFilePath);

        horizontalSpacer_12 = new QSpacerItem(6, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_12);

        backupFilePathPicker = new QPushButton(saveGroup);
        backupFilePathPicker->setObjectName(QString::fromUtf8("backupFilePathPicker"));
        backupFilePathPicker->setEnabled(false);

        horizontalLayout_4->addWidget(backupFilePathPicker);


        verticalLayout_5->addLayout(horizontalLayout_4);

        useAlternativeSaveCheckBox = new QCheckBox(saveGroup);
        useAlternativeSaveCheckBox->setObjectName(QString::fromUtf8("useAlternativeSaveCheckBox"));

        verticalLayout_5->addWidget(useAlternativeSaveCheckBox);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_8 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_8);

        alternativeSaveComboBox = new QComboBox(saveGroup);
        alternativeSaveComboBox->addItem(QString());
        alternativeSaveComboBox->addItem(QString());
        alternativeSaveComboBox->setObjectName(QString::fromUtf8("alternativeSaveComboBox"));
        alternativeSaveComboBox->setEnabled(false);
        alternativeSaveComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        horizontalLayout_3->addWidget(alternativeSaveComboBox);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_7);


        verticalLayout_5->addLayout(horizontalLayout_3);


        verticalLayout_8->addWidget(saveGroup);

        entryGroup = new QGroupBox(scrollAreaWidgetContents);
        entryGroup->setObjectName(QString::fromUtf8("entryGroup"));
        verticalLayout_6 = new QVBoxLayout(entryGroup);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        ConfirmMoveEntryToRecycleBinCheckBox = new QCheckBox(entryGroup);
        ConfirmMoveEntryToRecycleBinCheckBox->setObjectName(QString::fromUtf8("ConfirmMoveEntryToRecycleBinCheckBox"));

        verticalLayout_6->addWidget(ConfirmMoveEntryToRecycleBinCheckBox);

        EnableCopyOnDoubleClickCheckBox = new QCheckBox(entryGroup);
        EnableCopyOnDoubleClickCheckBox->setObjectName(QString::fromUtf8("EnableCopyOnDoubleClickCheckBox"));

        verticalLayout_6->addWidget(EnableCopyOnDoubleClickCheckBox);

        openUrlOnDoubleClick = new QCheckBox(entryGroup);
        openUrlOnDoubleClick->setObjectName(QString::fromUtf8("openUrlOnDoubleClick"));
        openUrlOnDoubleClick->setChecked(true);

        verticalLayout_6->addWidget(openUrlOnDoubleClick);

        useGroupIconOnEntryCreationCheckBox = new QCheckBox(entryGroup);
        useGroupIconOnEntryCreationCheckBox->setObjectName(QString::fromUtf8("useGroupIconOnEntryCreationCheckBox"));
        useGroupIconOnEntryCreationCheckBox->setChecked(true);

        verticalLayout_6->addWidget(useGroupIconOnEntryCreationCheckBox);

        minimizeOnOpenUrlCheckBox = new QCheckBox(entryGroup);
        minimizeOnOpenUrlCheckBox->setObjectName(QString::fromUtf8("minimizeOnOpenUrlCheckBox"));

        verticalLayout_6->addWidget(minimizeOnOpenUrlCheckBox);

        hideWindowOnCopyCheckBox = new QCheckBox(entryGroup);
        hideWindowOnCopyCheckBox->setObjectName(QString::fromUtf8("hideWindowOnCopyCheckBox"));

        verticalLayout_6->addWidget(hideWindowOnCopyCheckBox);

        hideWindowLayout_1 = new QHBoxLayout();
        hideWindowLayout_1->setSpacing(0);
        hideWindowLayout_1->setObjectName(QString::fromUtf8("hideWindowLayout_1"));
        hideWindowSpacer_1 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        hideWindowLayout_1->addItem(hideWindowSpacer_1);

        minimizeOnCopyRadioButton = new QRadioButton(entryGroup);
        minimizeOnCopyRadioButton->setObjectName(QString::fromUtf8("minimizeOnCopyRadioButton"));
        minimizeOnCopyRadioButton->setEnabled(false);

        hideWindowLayout_1->addWidget(minimizeOnCopyRadioButton);


        verticalLayout_6->addLayout(hideWindowLayout_1);

        hideWindowLayout_2 = new QHBoxLayout();
        hideWindowLayout_2->setSpacing(0);
        hideWindowLayout_2->setObjectName(QString::fromUtf8("hideWindowLayout_2"));
        hideWindowSpacer_2 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        hideWindowLayout_2->addItem(hideWindowSpacer_2);

        dropToBackgroundOnCopyRadioButton = new QRadioButton(entryGroup);
        dropToBackgroundOnCopyRadioButton->setObjectName(QString::fromUtf8("dropToBackgroundOnCopyRadioButton"));
        dropToBackgroundOnCopyRadioButton->setEnabled(false);

        hideWindowLayout_2->addWidget(dropToBackgroundOnCopyRadioButton);


        verticalLayout_6->addLayout(hideWindowLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        faviconTimeoutLabel = new QLabel(entryGroup);
        faviconTimeoutLabel->setObjectName(QString::fromUtf8("faviconTimeoutLabel"));

        horizontalLayout->addWidget(faviconTimeoutLabel);

        faviconTimeoutSpinBox = new QSpinBox(entryGroup);
        faviconTimeoutSpinBox->setObjectName(QString::fromUtf8("faviconTimeoutSpinBox"));
        faviconTimeoutSpinBox->setEnabled(true);
        faviconTimeoutSpinBox->setFocusPolicy(Qt::StrongFocus);
        faviconTimeoutSpinBox->setMinimum(1);
        faviconTimeoutSpinBox->setMaximum(60);
        faviconTimeoutSpinBox->setValue(10);

        horizontalLayout->addWidget(faviconTimeoutSpinBox);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout_6->addLayout(horizontalLayout);


        verticalLayout_8->addWidget(entryGroup);

        generalGroup = new QGroupBox(scrollAreaWidgetContents);
        generalGroup->setObjectName(QString::fromUtf8("generalGroup"));
        verticalLayout_7 = new QVBoxLayout(generalGroup);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        gridLayout_3->setHorizontalSpacing(10);
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_5, 0, 3, 1, 1);

        languageLabel_2 = new QLabel(generalGroup);
        languageLabel_2->setObjectName(QString::fromUtf8("languageLabel_2"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(languageLabel_2->sizePolicy().hasHeightForWidth());
        languageLabel_2->setSizePolicy(sizePolicy2);
        languageLabel_2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(languageLabel_2, 0, 0, 1, 1);

        toolbarMovableCheckBox = new QCheckBox(generalGroup);
        toolbarMovableCheckBox->setObjectName(QString::fromUtf8("toolbarMovableCheckBox"));
        toolbarMovableCheckBox->setEnabled(true);
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(toolbarMovableCheckBox->sizePolicy().hasHeightForWidth());
        toolbarMovableCheckBox->setSizePolicy(sizePolicy3);

        gridLayout_3->addWidget(toolbarMovableCheckBox, 1, 2, 1, 1);

        toolButtonStyleComboBox = new QComboBox(generalGroup);
        toolButtonStyleComboBox->setObjectName(QString::fromUtf8("toolButtonStyleComboBox"));
        sizePolicy3.setHeightForWidth(toolButtonStyleComboBox->sizePolicy().hasHeightForWidth());
        toolButtonStyleComboBox->setSizePolicy(sizePolicy3);
        toolButtonStyleComboBox->setFocusPolicy(Qt::StrongFocus);
        toolButtonStyleComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_3->addWidget(toolButtonStyleComboBox, 1, 1, 1, 1);

        languageComboBox = new QComboBox(generalGroup);
        languageComboBox->setObjectName(QString::fromUtf8("languageComboBox"));
        sizePolicy3.setHeightForWidth(languageComboBox->sizePolicy().hasHeightForWidth());
        languageComboBox->setSizePolicy(sizePolicy3);
        languageComboBox->setFocusPolicy(Qt::StrongFocus);
        languageComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_3->addWidget(languageComboBox, 0, 1, 1, 1);

        toolButtonStyleLabel = new QLabel(generalGroup);
        toolButtonStyleLabel->setObjectName(QString::fromUtf8("toolButtonStyleLabel"));
        toolButtonStyleLabel->setEnabled(true);
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(toolButtonStyleLabel->sizePolicy().hasHeightForWidth());
        toolButtonStyleLabel->setSizePolicy(sizePolicy4);
        toolButtonStyleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(toolButtonStyleLabel, 1, 0, 1, 1);

        languageLabel_3 = new QLabel(generalGroup);
        languageLabel_3->setObjectName(QString::fromUtf8("languageLabel_3"));

        gridLayout_3->addWidget(languageLabel_3, 0, 2, 1, 1);

        fontSizeLabel = new QLabel(generalGroup);
        fontSizeLabel->setObjectName(QString::fromUtf8("fontSizeLabel"));
        fontSizeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_3->addWidget(fontSizeLabel, 2, 0, 1, 1);

        fontSizeComboBox = new QComboBox(generalGroup);
        fontSizeComboBox->setObjectName(QString::fromUtf8("fontSizeComboBox"));
        fontSizeComboBox->setFocusPolicy(Qt::StrongFocus);
        fontSizeComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_3->addWidget(fontSizeComboBox, 2, 1, 1, 1);


        verticalLayout_7->addLayout(gridLayout_3);

        toolbarShowCheckBox = new QCheckBox(generalGroup);
        toolbarShowCheckBox->setObjectName(QString::fromUtf8("toolbarShowCheckBox"));

        verticalLayout_7->addWidget(toolbarShowCheckBox);

        menubarShowCheckBox = new QCheckBox(generalGroup);
        menubarShowCheckBox->setObjectName(QString::fromUtf8("menubarShowCheckBox"));

        verticalLayout_7->addWidget(menubarShowCheckBox);

        colorPasswordsCheckBox = new QCheckBox(generalGroup);
        colorPasswordsCheckBox->setObjectName(QString::fromUtf8("colorPasswordsCheckBox"));

        verticalLayout_7->addWidget(colorPasswordsCheckBox);

        monospaceNotesCheckBox = new QCheckBox(generalGroup);
        monospaceNotesCheckBox->setObjectName(QString::fromUtf8("monospaceNotesCheckBox"));

        verticalLayout_7->addWidget(monospaceNotesCheckBox);

        minimizeOnCloseCheckBox = new QCheckBox(generalGroup);
        minimizeOnCloseCheckBox->setObjectName(QString::fromUtf8("minimizeOnCloseCheckBox"));

        verticalLayout_7->addWidget(minimizeOnCloseCheckBox);

        systrayShowCheckBox = new QCheckBox(generalGroup);
        systrayShowCheckBox->setObjectName(QString::fromUtf8("systrayShowCheckBox"));

        verticalLayout_7->addWidget(systrayShowCheckBox);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setSpacing(0);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setSizeConstraint(QLayout::SetMaximumSize);
        horizontalSpacer_3 = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_3, 0, 0, 1, 1);

        trayIconAppearance = new QComboBox(generalGroup);
        trayIconAppearance->setObjectName(QString::fromUtf8("trayIconAppearance"));
        trayIconAppearance->setEnabled(false);
        sizePolicy3.setHeightForWidth(trayIconAppearance->sizePolicy().hasHeightForWidth());
        trayIconAppearance->setSizePolicy(sizePolicy3);
        trayIconAppearance->setFocusPolicy(Qt::StrongFocus);
        trayIconAppearance->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout_2->addWidget(trayIconAppearance, 0, 3, 1, 1);

        trayIconAppearanceLabel = new QLabel(generalGroup);
        trayIconAppearanceLabel->setObjectName(QString::fromUtf8("trayIconAppearanceLabel"));
        trayIconAppearanceLabel->setEnabled(false);

        gridLayout_2->addWidget(trayIconAppearanceLabel, 0, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_4, 0, 4, 1, 1);

        verticalSpacer_6 = new QSpacerItem(6, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout_2->addItem(verticalSpacer_6, 0, 2, 1, 1);


        verticalLayout_7->addLayout(gridLayout_2);

        systraySettings = new QWidget(generalGroup);
        systraySettings->setObjectName(QString::fromUtf8("systraySettings"));
        systrayLayout = new QVBoxLayout(systraySettings);
        systrayLayout->setObjectName(QString::fromUtf8("systrayLayout"));
        systrayLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setSizeConstraint(QLayout::SetMaximumSize);
        horizontalSpacer = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        systrayMinimizeToTrayCheckBox = new QCheckBox(systraySettings);
        systrayMinimizeToTrayCheckBox->setObjectName(QString::fromUtf8("systrayMinimizeToTrayCheckBox"));
        systrayMinimizeToTrayCheckBox->setEnabled(false);
        sizePolicy3.setHeightForWidth(systrayMinimizeToTrayCheckBox->sizePolicy().hasHeightForWidth());
        systrayMinimizeToTrayCheckBox->setSizePolicy(sizePolicy3);

        horizontalLayout_2->addWidget(systrayMinimizeToTrayCheckBox);


        systrayLayout->addLayout(horizontalLayout_2);


        verticalLayout_7->addWidget(systraySettings);


        verticalLayout_8->addWidget(generalGroup);

        resetSettingsSubLayout = new QHBoxLayout();
        resetSettingsSubLayout->setSpacing(6);
        resetSettingsSubLayout->setObjectName(QString::fromUtf8("resetSettingsSubLayout"));
        resetSettingsSubLayout->setSizeConstraint(QLayout::SetMaximumSize);
        resetSettingsButton = new QPushButton(scrollAreaWidgetContents);
        resetSettingsButton->setObjectName(QString::fromUtf8("resetSettingsButton"));

        resetSettingsSubLayout->addWidget(resetSettingsButton);

        horizontalSpacer_10 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        resetSettingsSubLayout->addItem(horizontalSpacer_10);

        importSettingsButton = new QPushButton(scrollAreaWidgetContents);
        importSettingsButton->setObjectName(QString::fromUtf8("importSettingsButton"));

        resetSettingsSubLayout->addWidget(importSettingsButton);

        exportSettingsButton = new QPushButton(scrollAreaWidgetContents);
        exportSettingsButton->setObjectName(QString::fromUtf8("exportSettingsButton"));

        resetSettingsSubLayout->addWidget(exportSettingsButton);


        verticalLayout_8->addLayout(resetSettingsSubLayout);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_8->addItem(verticalSpacer);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

        generalSettingsTabWidget->addTab(tabGeneral, QString());
        tabAutotype = new QWidget();
        tabAutotype->setObjectName(QString::fromUtf8("tabAutotype"));
        verticalLayout_2 = new QVBoxLayout(tabAutotype);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(10, 10, 10, 10);
        autoTypeEntryTitleMatchCheckBox = new QCheckBox(tabAutotype);
        autoTypeEntryTitleMatchCheckBox->setObjectName(QString::fromUtf8("autoTypeEntryTitleMatchCheckBox"));

        verticalLayout_2->addWidget(autoTypeEntryTitleMatchCheckBox);

        autoTypeEntryURLMatchCheckBox = new QCheckBox(tabAutotype);
        autoTypeEntryURLMatchCheckBox->setObjectName(QString::fromUtf8("autoTypeEntryURLMatchCheckBox"));

        verticalLayout_2->addWidget(autoTypeEntryURLMatchCheckBox);

        autoTypeAskCheckBox = new QCheckBox(tabAutotype);
        autoTypeAskCheckBox->setObjectName(QString::fromUtf8("autoTypeAskCheckBox"));
        autoTypeAskCheckBox->setChecked(true);

        verticalLayout_2->addWidget(autoTypeAskCheckBox);

        autoTypeSkipMainWindowLayout = new QHBoxLayout();
        autoTypeSkipMainWindowLayout->setSpacing(0);
        autoTypeSkipMainWindowLayout->setObjectName(QString::fromUtf8("autoTypeSkipMainWindowLayout"));
        autoTypeSkipMainWindowSpacer = new QSpacerItem(30, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        autoTypeSkipMainWindowLayout->addItem(autoTypeSkipMainWindowSpacer);

        autoTypeSkipMainWindowConfirmationCheckBox = new QCheckBox(tabAutotype);
        autoTypeSkipMainWindowConfirmationCheckBox->setObjectName(QString::fromUtf8("autoTypeSkipMainWindowConfirmationCheckBox"));
        autoTypeSkipMainWindowConfirmationCheckBox->setEnabled(false);
        autoTypeSkipMainWindowConfirmationCheckBox->setChecked(false);

        autoTypeSkipMainWindowLayout->addWidget(autoTypeSkipMainWindowConfirmationCheckBox);


        verticalLayout_2->addLayout(autoTypeSkipMainWindowLayout);

        autoTypeHideExpiredEntryCheckBox = new QCheckBox(tabAutotype);
        autoTypeHideExpiredEntryCheckBox->setObjectName(QString::fromUtf8("autoTypeHideExpiredEntryCheckBox"));

        verticalLayout_2->addWidget(autoTypeHideExpiredEntryCheckBox);

        autoTypeRelockDatabaseCheckBox = new QCheckBox(tabAutotype);
        autoTypeRelockDatabaseCheckBox->setObjectName(QString::fromUtf8("autoTypeRelockDatabaseCheckBox"));

        verticalLayout_2->addWidget(autoTypeRelockDatabaseCheckBox);

        verticalSpacer_4 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_2->addItem(verticalSpacer_4);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(8);
        autoTypeStartDelayLabel = new QLabel(tabAutotype);
        autoTypeStartDelayLabel->setObjectName(QString::fromUtf8("autoTypeStartDelayLabel"));
        autoTypeStartDelayLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(autoTypeStartDelayLabel, 2, 0, 1, 1);

        autoTypeShortcutLabel = new QLabel(tabAutotype);
        autoTypeShortcutLabel->setObjectName(QString::fromUtf8("autoTypeShortcutLabel"));
        autoTypeShortcutLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(autoTypeShortcutLabel, 0, 0, 1, 1);

        autoTypeStartDelaySpinBox = new QSpinBox(tabAutotype);
        autoTypeStartDelaySpinBox->setObjectName(QString::fromUtf8("autoTypeStartDelaySpinBox"));
        sizePolicy4.setHeightForWidth(autoTypeStartDelaySpinBox->sizePolicy().hasHeightForWidth());
        autoTypeStartDelaySpinBox->setSizePolicy(sizePolicy4);
        autoTypeStartDelaySpinBox->setMinimum(100);
        autoTypeStartDelaySpinBox->setMaximum(10000);
        autoTypeStartDelaySpinBox->setSingleStep(100);
        autoTypeStartDelaySpinBox->setValue(500);

        gridLayout->addWidget(autoTypeStartDelaySpinBox, 2, 1, 1, 1);

        autoTypeDelayLabel = new QLabel(tabAutotype);
        autoTypeDelayLabel->setObjectName(QString::fromUtf8("autoTypeDelayLabel"));
        autoTypeDelayLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(autoTypeDelayLabel, 3, 0, 1, 1);

        horizontalSpacer_6 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_6, 0, 2, 1, 1);

        autoTypeShortcutWidget = new ShortcutWidget(tabAutotype);
        autoTypeShortcutWidget->setObjectName(QString::fromUtf8("autoTypeShortcutWidget"));
        sizePolicy4.setHeightForWidth(autoTypeShortcutWidget->sizePolicy().hasHeightForWidth());
        autoTypeShortcutWidget->setSizePolicy(sizePolicy4);

        gridLayout->addWidget(autoTypeShortcutWidget, 0, 1, 1, 1);

        autoTypeDelaySpinBox = new QSpinBox(tabAutotype);
        autoTypeDelaySpinBox->setObjectName(QString::fromUtf8("autoTypeDelaySpinBox"));
        sizePolicy4.setHeightForWidth(autoTypeDelaySpinBox->sizePolicy().hasHeightForWidth());
        autoTypeDelaySpinBox->setSizePolicy(sizePolicy4);
        autoTypeDelaySpinBox->setMaximum(1000);
        autoTypeDelaySpinBox->setValue(25);

        gridLayout->addWidget(autoTypeDelaySpinBox, 3, 1, 1, 1);

        autoTypeRetypeLabel = new QLabel(tabAutotype);
        autoTypeRetypeLabel->setObjectName(QString::fromUtf8("autoTypeRetypeLabel"));
        autoTypeRetypeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(autoTypeRetypeLabel, 1, 0, 1, 1);

        autoTypeRetypeTimeSpinBox = new QSpinBox(tabAutotype);
        autoTypeRetypeTimeSpinBox->setObjectName(QString::fromUtf8("autoTypeRetypeTimeSpinBox"));
        autoTypeRetypeTimeSpinBox->setMinimum(0);
        autoTypeRetypeTimeSpinBox->setMaximum(60);
        autoTypeRetypeTimeSpinBox->setValue(15);

        gridLayout->addWidget(autoTypeRetypeTimeSpinBox, 1, 1, 1, 1);


        verticalLayout_2->addLayout(gridLayout);

        verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);

        generalSettingsTabWidget->addTab(tabAutotype, QString());

        verticalLayout_3->addWidget(generalSettingsTabWidget);

#if QT_CONFIG(shortcut)
        label->setBuddy(backupFilePath);
        faviconTimeoutLabel->setBuddy(faviconTimeoutSpinBox);
        languageLabel_2->setBuddy(languageComboBox);
        toolButtonStyleLabel->setBuddy(toolButtonStyleComboBox);
        fontSizeLabel->setBuddy(fontSizeComboBox);
        trayIconAppearanceLabel->setBuddy(trayIconAppearance);
        autoTypeStartDelayLabel->setBuddy(autoTypeStartDelaySpinBox);
        autoTypeShortcutLabel->setBuddy(autoTypeShortcutWidget);
        autoTypeDelayLabel->setBuddy(autoTypeDelaySpinBox);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(generalSettingsTabWidget, scrollArea);
        QWidget::setTabOrder(scrollArea, singleInstanceCheckBox);
        QWidget::setTabOrder(singleInstanceCheckBox, launchAtStartup);
        QWidget::setTabOrder(launchAtStartup, systrayMinimizeOnStartup);
        QWidget::setTabOrder(systrayMinimizeOnStartup, minimizeAfterUnlockCheckBox);
        QWidget::setTabOrder(minimizeAfterUnlockCheckBox, rememberLastDatabasesCheckBox);
        QWidget::setTabOrder(rememberLastDatabasesCheckBox, rememberLastDatabasesSpinbox);
        QWidget::setTabOrder(rememberLastDatabasesSpinbox, openPreviousDatabasesOnStartupCheckBox);
        QWidget::setTabOrder(openPreviousDatabasesOnStartupCheckBox, rememberLastKeyFilesCheckBox);
        QWidget::setTabOrder(rememberLastKeyFilesCheckBox, checkForUpdatesOnStartupCheckBox);
        QWidget::setTabOrder(checkForUpdatesOnStartupCheckBox, checkForUpdatesIncludeBetasCheckBox);
        QWidget::setTabOrder(checkForUpdatesIncludeBetasCheckBox, showExpiredEntriesOnDatabaseUnlockCheckBox);
        QWidget::setTabOrder(showExpiredEntriesOnDatabaseUnlockCheckBox, showExpiredEntriesOnDatabaseUnlockOffsetSpinBox);
        QWidget::setTabOrder(showExpiredEntriesOnDatabaseUnlockOffsetSpinBox, autoSaveAfterEveryChangeCheckBox);
        QWidget::setTabOrder(autoSaveAfterEveryChangeCheckBox, autoSaveOnExitCheckBox);
        QWidget::setTabOrder(autoSaveOnExitCheckBox, autoSaveNonDataChangesCheckBox);
        QWidget::setTabOrder(autoSaveNonDataChangesCheckBox, autoReloadOnChangeCheckBox);
        QWidget::setTabOrder(autoReloadOnChangeCheckBox, backupBeforeSaveCheckBox);
        QWidget::setTabOrder(backupBeforeSaveCheckBox, backupFilePath);
        QWidget::setTabOrder(backupFilePath, backupFilePathPicker);
        QWidget::setTabOrder(backupFilePathPicker, useAlternativeSaveCheckBox);
        QWidget::setTabOrder(useAlternativeSaveCheckBox, alternativeSaveComboBox);
        QWidget::setTabOrder(alternativeSaveComboBox, ConfirmMoveEntryToRecycleBinCheckBox);
        QWidget::setTabOrder(ConfirmMoveEntryToRecycleBinCheckBox, EnableCopyOnDoubleClickCheckBox);
        QWidget::setTabOrder(EnableCopyOnDoubleClickCheckBox, openUrlOnDoubleClick);
        QWidget::setTabOrder(openUrlOnDoubleClick, useGroupIconOnEntryCreationCheckBox);
        QWidget::setTabOrder(useGroupIconOnEntryCreationCheckBox, minimizeOnOpenUrlCheckBox);
        QWidget::setTabOrder(minimizeOnOpenUrlCheckBox, hideWindowOnCopyCheckBox);
        QWidget::setTabOrder(hideWindowOnCopyCheckBox, minimizeOnCopyRadioButton);
        QWidget::setTabOrder(minimizeOnCopyRadioButton, dropToBackgroundOnCopyRadioButton);
        QWidget::setTabOrder(dropToBackgroundOnCopyRadioButton, faviconTimeoutSpinBox);
        QWidget::setTabOrder(faviconTimeoutSpinBox, languageComboBox);
        QWidget::setTabOrder(languageComboBox, toolButtonStyleComboBox);
        QWidget::setTabOrder(toolButtonStyleComboBox, toolbarMovableCheckBox);
        QWidget::setTabOrder(toolbarMovableCheckBox, toolbarShowCheckBox);
        QWidget::setTabOrder(toolbarShowCheckBox, menubarShowCheckBox);
        QWidget::setTabOrder(menubarShowCheckBox, colorPasswordsCheckBox);
        QWidget::setTabOrder(colorPasswordsCheckBox, monospaceNotesCheckBox);
        QWidget::setTabOrder(monospaceNotesCheckBox, minimizeOnCloseCheckBox);
        QWidget::setTabOrder(minimizeOnCloseCheckBox, systrayShowCheckBox);
        QWidget::setTabOrder(systrayShowCheckBox, trayIconAppearance);
        QWidget::setTabOrder(trayIconAppearance, systrayMinimizeToTrayCheckBox);
        QWidget::setTabOrder(systrayMinimizeToTrayCheckBox, resetSettingsButton);
        QWidget::setTabOrder(resetSettingsButton, importSettingsButton);
        QWidget::setTabOrder(importSettingsButton, exportSettingsButton);
        QWidget::setTabOrder(exportSettingsButton, autoTypeEntryTitleMatchCheckBox);
        QWidget::setTabOrder(autoTypeEntryTitleMatchCheckBox, autoTypeEntryURLMatchCheckBox);
        QWidget::setTabOrder(autoTypeEntryURLMatchCheckBox, autoTypeAskCheckBox);
        QWidget::setTabOrder(autoTypeAskCheckBox, autoTypeHideExpiredEntryCheckBox);
        QWidget::setTabOrder(autoTypeHideExpiredEntryCheckBox, autoTypeRelockDatabaseCheckBox);
        QWidget::setTabOrder(autoTypeRelockDatabaseCheckBox, autoTypeShortcutWidget);
        QWidget::setTabOrder(autoTypeShortcutWidget, autoTypeRetypeTimeSpinBox);
        QWidget::setTabOrder(autoTypeRetypeTimeSpinBox, autoTypeStartDelaySpinBox);
        QWidget::setTabOrder(autoTypeStartDelaySpinBox, autoTypeDelaySpinBox);

        retranslateUi(ApplicationSettingsWidgetGeneral);

        generalSettingsTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ApplicationSettingsWidgetGeneral);
    } // setupUi

    void retranslateUi(QWidget *ApplicationSettingsWidgetGeneral)
    {
        startupGroup->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Startup", nullptr));
        singleInstanceCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Start only a single instance of KeePassXC", nullptr));
        launchAtStartup->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Automatically launch KeePassXC at system startup", nullptr));
        systrayMinimizeOnStartup->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Minimize window at application startup", nullptr));
        minimizeAfterUnlockCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Minimize window after unlocking database", nullptr));
        rememberLastDatabasesCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Remember previously used databases", nullptr));
        rememberLastDatabasesSpinbox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", " recent files", nullptr));
        openPreviousDatabasesOnStartupCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Load previously open databases on startup", nullptr));
        rememberLastKeyFilesCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Remember database key files and security dongles", nullptr));
        checkForUpdatesOnStartupCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Check for updates at application startup once per week", nullptr));
        checkForUpdatesIncludeBetasCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Include beta releases when checking for updates", nullptr));
        showExpiredEntriesOnDatabaseUnlockCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "On database unlock, show entries that will expire within", nullptr));
#if QT_CONFIG(accessibility)
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "On database unlock, show entries that will expire within ", nullptr));
#endif // QT_CONFIG(accessibility)
        showExpiredEntriesOnDatabaseUnlockOffsetSpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", " days", "number of days warning for password expiration"));
        saveGroup->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "File Management", nullptr));
        autoSaveAfterEveryChangeCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Automatically save after every change", nullptr));
        autoSaveOnExitCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Automatically save when locking database", nullptr));
        autoSaveNonDataChangesCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Automatically save non-data changes when locking database", nullptr));
        autoReloadOnChangeCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Automatically reload the database when modified externally", nullptr));
        backupBeforeSaveCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Backup database file before saving", nullptr));
        label->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Destination format:", nullptr));
#if QT_CONFIG(tooltip)
        backupFilePath->setToolTip(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "<html><head/><body><p><span style=\" font-weight:600;\">{DB_FILENAME}</span> is replaced with the filename of the saved database without extension</p><p><span style=\" font-weight:600;\">{TIME:&lt;format&gt;}</span> is replaced with the specified time format (default: dd_MM_yyyy_hh-mm-ss)</p><p>See the User Guide for more details</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        backupFilePath->setPlaceholderText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "{DB_FILENAME}.old.kdbx", nullptr));
        backupFilePathPicker->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Choose folder...", nullptr));
        useAlternativeSaveCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Use alternative saving method (may solve problems with Dropbox, Google Drive, GVFS, etc.)", nullptr));
        alternativeSaveComboBox->setItemText(0, QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Temporary file moved into place", nullptr));
        alternativeSaveComboBox->setItemText(1, QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Directly write to database file (dangerous)", nullptr));

        entryGroup->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Entry Management", nullptr));
        ConfirmMoveEntryToRecycleBinCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Show confirmation before moving entries to recycle bin", nullptr));
        EnableCopyOnDoubleClickCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Copy data on double clicking field in entry view", nullptr));
        openUrlOnDoubleClick->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Open browser on double clicking URL field in entry view", nullptr));
        useGroupIconOnEntryCreationCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Use group icon on entry creation", nullptr));
        minimizeOnOpenUrlCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Minimize when opening a URL", nullptr));
        hideWindowOnCopyCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Hide window when copying to clipboard", nullptr));
        minimizeOnCopyRadioButton->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Minimize", nullptr));
        dropToBackgroundOnCopyRadioButton->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Drop to background", nullptr));
        faviconTimeoutLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Favicon download timeout:", nullptr));
#if QT_CONFIG(accessibility)
        faviconTimeoutSpinBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Website icon download timeout in seconds", nullptr));
#endif // QT_CONFIG(accessibility)
        faviconTimeoutSpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", " sec", "Seconds"));
        generalGroup->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "User Interface", nullptr));
        languageLabel_2->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Language:", nullptr));
        toolbarMovableCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Movable toolbar", nullptr));
#if QT_CONFIG(accessibility)
        toolButtonStyleComboBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Toolbar button style", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        languageComboBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Language selection", nullptr));
#endif // QT_CONFIG(accessibility)
        toolButtonStyleLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Toolbar button style:", nullptr));
        languageLabel_3->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "(restart program to activate)", nullptr));
        fontSizeLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Font size:", nullptr));
#if QT_CONFIG(accessibility)
        fontSizeComboBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Font size selection", nullptr));
#endif // QT_CONFIG(accessibility)
        toolbarShowCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Show toolbar", nullptr));
#if QT_CONFIG(tooltip)
        menubarShowCheckBox->setToolTip(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Show the menu bar by pressing the Alt key", nullptr));
#endif // QT_CONFIG(tooltip)
        menubarShowCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Show menubar", nullptr));
        colorPasswordsCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Show passwords in color", nullptr));
        monospaceNotesCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Use monospaced font for notes", nullptr));
        minimizeOnCloseCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Minimize instead of app exit", nullptr));
        systrayShowCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Show a system tray icon", nullptr));
#if QT_CONFIG(accessibility)
        trayIconAppearance->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Tray icon type", nullptr));
#endif // QT_CONFIG(accessibility)
        trayIconAppearanceLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Tray icon type:", nullptr));
        systrayMinimizeToTrayCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Hide window to system tray when minimized", nullptr));
        resetSettingsButton->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Reset settings to default\342\200\246", nullptr));
        importSettingsButton->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Import settings\342\200\246", nullptr));
        exportSettingsButton->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Export settings\342\200\246", nullptr));
        generalSettingsTabWidget->setTabText(generalSettingsTabWidget->indexOf(tabGeneral), QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Basic Settings", nullptr));
        autoTypeEntryTitleMatchCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Use entry title to match windows for global Auto-Type", nullptr));
        autoTypeEntryURLMatchCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Use entry URL to match windows for global Auto-Type", nullptr));
        autoTypeAskCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Always ask before performing Auto-Type", nullptr));
        autoTypeSkipMainWindowConfirmationCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Skip confirmation for main window Auto-Type actions", nullptr));
        autoTypeHideExpiredEntryCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Hide expired entries from Auto-Type", nullptr));
        autoTypeRelockDatabaseCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Re-lock previously locked database after performing Auto-Type", nullptr));
        autoTypeStartDelayLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Auto-Type start delay:", nullptr));
        autoTypeShortcutLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Global Auto-Type shortcut:", nullptr));
#if QT_CONFIG(accessibility)
        autoTypeStartDelaySpinBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Auto-type start delay milliseconds", nullptr));
#endif // QT_CONFIG(accessibility)
        autoTypeStartDelaySpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", " ms", "Milliseconds"));
        autoTypeStartDelaySpinBox->setPrefix(QString());
        autoTypeDelayLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Auto-Type typing delay:", nullptr));
#if QT_CONFIG(accessibility)
        autoTypeShortcutWidget->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Global auto-type shortcut", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        autoTypeDelaySpinBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Auto-type character typing delay milliseconds", nullptr));
#endif // QT_CONFIG(accessibility)
        autoTypeDelaySpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", " ms", "Milliseconds"));
        autoTypeDelaySpinBox->setPrefix(QString());
        autoTypeRetypeLabel->setText(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Remember last typed entry for:", nullptr));
        autoTypeRetypeTimeSpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetGeneral", " sec", "Seconds"));
        generalSettingsTabWidget->setTabText(generalSettingsTabWidget->indexOf(tabAutotype), QCoreApplication::translate("ApplicationSettingsWidgetGeneral", "Auto-Type", nullptr));
        (void)ApplicationSettingsWidgetGeneral;
    } // retranslateUi

};

namespace Ui {
    class ApplicationSettingsWidgetGeneral: public Ui_ApplicationSettingsWidgetGeneral {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_APPLICATIONSETTINGSWIDGETGENERAL_H
