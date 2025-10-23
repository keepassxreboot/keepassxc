/********************************************************************************
** Form generated from reading UI file 'DatabaseSettingsWidgetRemote.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATABASESETTINGSWIDGETREMOTE_H
#define UI_DATABASESETTINGSWIDGETREMOTE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/MessageWidget.h"

QT_BEGIN_NAMESPACE

class Ui_DatabaseSettingsWidgetRemote
{
public:
    QVBoxLayout *verticalLayout;
    MessageWidget *messageWidget;
    QGroupBox *syncCommandsGroupBox;
    QHBoxLayout *horizontalLayout;
    QListWidget *settingsListWidget;
    QVBoxLayout *verticalLayout_2;
    QPushButton *removeSettingsButton;
    QSpacerItem *verticalSpacer;
    QGroupBox *commandSettingsGroupBox;
    QVBoxLayout *verticalLayout_3;
    QGridLayout *gridLayout;
    QLabel *label;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *nameLineEdit;
    QPushButton *saveSettingsButton;
    QTabWidget *commandTabWidget;
    QWidget *downloadTab;
    QGridLayout *gridLayout_4;
    QLabel *downloadCommandInputLabel;
    QLabel *downloadCommandLabel;
    QPlainTextEdit *inputForDownload;
    QHBoxLayout *horizontalLayout_3;
    QLineEdit *downloadCommand;
    QPushButton *testDownloadCommandButton;
    QLabel *downloadTimeoutLabel;
    QSpinBox *downloadTimeoutSec;
    QWidget *uploadTab;
    QGridLayout *gridLayout_5;
    QPlainTextEdit *inputForUpload;
    QLabel *uploadCommandInputLabel;
    QLineEdit *uploadCommand;
    QLabel *uploadCommandLabel;
    QLabel *uploadTimeoutLabel;
    QSpinBox *uploadTimeoutSec;

    void setupUi(QWidget *DatabaseSettingsWidgetRemote)
    {
        if (DatabaseSettingsWidgetRemote->objectName().isEmpty())
            DatabaseSettingsWidgetRemote->setObjectName(QString::fromUtf8("DatabaseSettingsWidgetRemote"));
        DatabaseSettingsWidgetRemote->resize(652, 516);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DatabaseSettingsWidgetRemote->sizePolicy().hasHeightForWidth());
        DatabaseSettingsWidgetRemote->setSizePolicy(sizePolicy);
        DatabaseSettingsWidgetRemote->setMinimumSize(QSize(450, 0));
        verticalLayout = new QVBoxLayout(DatabaseSettingsWidgetRemote);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        messageWidget = new MessageWidget(DatabaseSettingsWidgetRemote);
        messageWidget->setObjectName(QString::fromUtf8("messageWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(messageWidget->sizePolicy().hasHeightForWidth());
        messageWidget->setSizePolicy(sizePolicy1);

        verticalLayout->addWidget(messageWidget);

        syncCommandsGroupBox = new QGroupBox(DatabaseSettingsWidgetRemote);
        syncCommandsGroupBox->setObjectName(QString::fromUtf8("syncCommandsGroupBox"));
        horizontalLayout = new QHBoxLayout(syncCommandsGroupBox);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        settingsListWidget = new QListWidget(syncCommandsGroupBox);
        settingsListWidget->setObjectName(QString::fromUtf8("settingsListWidget"));

        horizontalLayout->addWidget(settingsListWidget);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        removeSettingsButton = new QPushButton(syncCommandsGroupBox);
        removeSettingsButton->setObjectName(QString::fromUtf8("removeSettingsButton"));
        removeSettingsButton->setLayoutDirection(Qt::LeftToRight);

        verticalLayout_2->addWidget(removeSettingsButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);


        horizontalLayout->addLayout(verticalLayout_2);


        verticalLayout->addWidget(syncCommandsGroupBox);

        commandSettingsGroupBox = new QGroupBox(DatabaseSettingsWidgetRemote);
        commandSettingsGroupBox->setObjectName(QString::fromUtf8("commandSettingsGroupBox"));
        sizePolicy1.setHeightForWidth(commandSettingsGroupBox->sizePolicy().hasHeightForWidth());
        commandSettingsGroupBox->setSizePolicy(sizePolicy1);
        verticalLayout_3 = new QVBoxLayout(commandSettingsGroupBox);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setSizeConstraint(QLayout::SetMinimumSize);
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(commandSettingsGroupBox);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        nameLineEdit = new QLineEdit(commandSettingsGroupBox);
        nameLineEdit->setObjectName(QString::fromUtf8("nameLineEdit"));

        horizontalLayout_2->addWidget(nameLineEdit);

        saveSettingsButton = new QPushButton(commandSettingsGroupBox);
        saveSettingsButton->setObjectName(QString::fromUtf8("saveSettingsButton"));

        horizontalLayout_2->addWidget(saveSettingsButton);


        gridLayout->addLayout(horizontalLayout_2, 0, 1, 1, 1);

        commandTabWidget = new QTabWidget(commandSettingsGroupBox);
        commandTabWidget->setObjectName(QString::fromUtf8("commandTabWidget"));
        downloadTab = new QWidget();
        downloadTab->setObjectName(QString::fromUtf8("downloadTab"));
        gridLayout_4 = new QGridLayout(downloadTab);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        downloadCommandInputLabel = new QLabel(downloadTab);
        downloadCommandInputLabel->setObjectName(QString::fromUtf8("downloadCommandInputLabel"));
        downloadCommandInputLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout_4->addWidget(downloadCommandInputLabel, 1, 0, 1, 1);

        downloadCommandLabel = new QLabel(downloadTab);
        downloadCommandLabel->setObjectName(QString::fromUtf8("downloadCommandLabel"));

        gridLayout_4->addWidget(downloadCommandLabel, 0, 0, 1, 1);

        inputForDownload = new QPlainTextEdit(downloadTab);
        inputForDownload->setObjectName(QString::fromUtf8("inputForDownload"));

        gridLayout_4->addWidget(inputForDownload, 1, 1, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        downloadCommand = new QLineEdit(downloadTab);
        downloadCommand->setObjectName(QString::fromUtf8("downloadCommand"));

        horizontalLayout_3->addWidget(downloadCommand);

        testDownloadCommandButton = new QPushButton(downloadTab);
        testDownloadCommandButton->setObjectName(QString::fromUtf8("testDownloadCommandButton"));

        horizontalLayout_3->addWidget(testDownloadCommandButton);


        gridLayout_4->addLayout(horizontalLayout_3, 0, 1, 1, 1);

        downloadTimeoutLabel = new QLabel(downloadTab);
        downloadTimeoutLabel->setObjectName(QString::fromUtf8("downloadTimeoutLabel"));

        gridLayout_4->addWidget(downloadTimeoutLabel, 2, 0, 1, 1);

        downloadTimeoutSec = new QSpinBox(downloadTab);
        downloadTimeoutSec->setObjectName(QString::fromUtf8("downloadTimeoutSec"));
        downloadTimeoutSec->setMinimum(1);
        downloadTimeoutSec->setMaximum(300);
        downloadTimeoutSec->setValue(10);

        gridLayout_4->addWidget(downloadTimeoutSec, 2, 1, 1, 1);

        commandTabWidget->addTab(downloadTab, QString());
        uploadTab = new QWidget();
        uploadTab->setObjectName(QString::fromUtf8("uploadTab"));
        gridLayout_5 = new QGridLayout(uploadTab);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        inputForUpload = new QPlainTextEdit(uploadTab);
        inputForUpload->setObjectName(QString::fromUtf8("inputForUpload"));

        gridLayout_5->addWidget(inputForUpload, 1, 1, 1, 1);

        uploadCommandInputLabel = new QLabel(uploadTab);
        uploadCommandInputLabel->setObjectName(QString::fromUtf8("uploadCommandInputLabel"));
        uploadCommandInputLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        gridLayout_5->addWidget(uploadCommandInputLabel, 1, 0, 1, 1);

        uploadCommand = new QLineEdit(uploadTab);
        uploadCommand->setObjectName(QString::fromUtf8("uploadCommand"));

        gridLayout_5->addWidget(uploadCommand, 0, 1, 1, 1);

        uploadCommandLabel = new QLabel(uploadTab);
        uploadCommandLabel->setObjectName(QString::fromUtf8("uploadCommandLabel"));

        gridLayout_5->addWidget(uploadCommandLabel, 0, 0, 1, 1);

        uploadTimeoutLabel = new QLabel(uploadTab);
        uploadTimeoutLabel->setObjectName(QString::fromUtf8("uploadTimeoutLabel"));

        gridLayout_5->addWidget(uploadTimeoutLabel, 2, 0, 1, 1);

        uploadTimeoutSec = new QSpinBox(uploadTab);
        uploadTimeoutSec->setObjectName(QString::fromUtf8("uploadTimeoutSec"));
        uploadTimeoutSec->setMinimum(1);
        uploadTimeoutSec->setMaximum(300);
        uploadTimeoutSec->setValue(10);

        gridLayout_5->addWidget(uploadTimeoutSec, 2, 1, 1, 1);

        commandTabWidget->addTab(uploadTab, QString());

        gridLayout->addWidget(commandTabWidget, 1, 1, 1, 1);


        verticalLayout_3->addLayout(gridLayout);


        verticalLayout->addWidget(commandSettingsGroupBox);

        verticalLayout->setStretch(1, 1);
        verticalLayout->setStretch(2, 3);

        retranslateUi(DatabaseSettingsWidgetRemote);

        commandTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(DatabaseSettingsWidgetRemote);
    } // setupUi

    void retranslateUi(QWidget *DatabaseSettingsWidgetRemote)
    {
        syncCommandsGroupBox->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Sync Commands", nullptr));
        removeSettingsButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Remove", nullptr));
        commandSettingsGroupBox->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Command Settings", nullptr));
        label->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Name", nullptr));
        saveSettingsButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Save", nullptr));
        downloadCommandInputLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Input:", nullptr));
        downloadCommandLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Command:", nullptr));
#if QT_CONFIG(accessibility)
        inputForDownload->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Download input field", nullptr));
#endif // QT_CONFIG(accessibility)
        inputForDownload->setPlaceholderText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "e.g.:\n"
"get DatabaseOnRemote.kdbx {TEMP_DATABASE}\n"
"exit\n"
"---\n"
"{TEMP_DATABASE} is used as placeholder to store the database in a temporary location\n"
"The command has to exit. In case of `sftp` as last command `exit` has to be sent\n"
"            ", nullptr));
#if QT_CONFIG(accessibility)
        downloadCommand->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Download command field", nullptr));
#endif // QT_CONFIG(accessibility)
        downloadCommand->setPlaceholderText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "e.g.: \"sftp user@hostname\" or \"scp user@hostname:DatabaseOnRemote.kdbx {TEMP_DATABASE}\"", nullptr));
        testDownloadCommandButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Test", nullptr));
        downloadTimeoutLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Timeout:", nullptr));
        downloadTimeoutSec->setSuffix(QCoreApplication::translate("DatabaseSettingsWidgetRemote", " seconds", nullptr));
        commandTabWidget->setTabText(commandTabWidget->indexOf(downloadTab), QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Download", nullptr));
#if QT_CONFIG(accessibility)
        inputForUpload->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Upload input field", nullptr));
#endif // QT_CONFIG(accessibility)
        inputForUpload->setPlaceholderText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "e.g.:\n"
"put {TEMP_DATABASE} DatabaseOnRemote.kdbx\n"
"exit\n"
"---\n"
"{TEMP_DATABASE} is used as placeholder to store the database in a temporary location\n"
"The command has to exit. In case of `sftp` as last command `exit` has to be sent\n"
"            ", nullptr));
        uploadCommandInputLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Input:", nullptr));
#if QT_CONFIG(accessibility)
        uploadCommand->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Upload command field", nullptr));
#endif // QT_CONFIG(accessibility)
        uploadCommand->setPlaceholderText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "e.g.: \"sftp user@hostname\" or \"scp {TEMP_DATABASE} user@hostname:DatabaseOnRemote.kdbx\"", nullptr));
        uploadCommandLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Command:", nullptr));
        uploadTimeoutLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Timeout:", nullptr));
        uploadTimeoutSec->setSuffix(QCoreApplication::translate("DatabaseSettingsWidgetRemote", " seconds", nullptr));
        commandTabWidget->setTabText(commandTabWidget->indexOf(uploadTab), QCoreApplication::translate("DatabaseSettingsWidgetRemote", "Upload", nullptr));
        (void)DatabaseSettingsWidgetRemote;
    } // retranslateUi

};

namespace Ui {
    class DatabaseSettingsWidgetRemote: public Ui_DatabaseSettingsWidgetRemote {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATABASESETTINGSWIDGETREMOTE_H
