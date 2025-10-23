/********************************************************************************
** Form generated from reading UI file 'DatabaseOpenWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATABASEOPENWIDGET_H
#define UI_DATABASEOPENWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/MessageWidget.h"
#include "gui/PasswordWidget.h"
#include "gui/widgets/ElidedLabel.h"

QT_BEGIN_NAMESPACE

class Ui_DatabaseOpenWidget
{
public:
    QVBoxLayout *verticalLayout;
    MessageWidget *messageWidget;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_3;
    QWidget *formContainer;
    QVBoxLayout *verticalLayout_4;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout;
    QLabel *dbIconLabel;
    QLabel *labelHeadline;
    QSpacerItem *horizontalSpacer_2;
    ElidedLabel *fileNameLabel;
    QSpacerItem *verticalSpacer_3;
    QStackedWidget *centralStack;
    QWidget *mainPage;
    QVBoxLayout *verticalLayout_6;
    QFrame *enterPasswordComponent;
    QVBoxLayout *enterPasswordComponentLayout;
    QLabel *passwordLabel;
    QVBoxLayout *verticalLayout_2;
    PasswordWidget *editPassword;
    QProgressBar *hardwareKeyProgress;
    QFrame *selectKeyFileComponent;
    QVBoxLayout *selectKeyFileComponentLayout;
    QLabel *selectKeyFileLabel;
    QHBoxLayout *horizontalLayout_7;
    PasswordWidget *keyFileLineEdit;
    QPushButton *buttonBrowseFile;
    QFrame *addAdditionalKeysComponent;
    QHBoxLayout *addAdditionalKeysComponentLayout;
    QFrame *hardwareKeyComponent;
    QHBoxLayout *hardwareKeyComponentLayout;
    QCheckBox *useHardwareKeyCheckBox;
    QComboBox *hardwareKeyCombo;
    QLabel *noHardwareKeysFoundLabel;
    QSpacerItem *horizontalSpacer;
    QPushButton *refreshHardwareKeys;
    QLabel *addKeyFileLinkLabel;
    QHBoxLayout *dialogButtonsLayout;
    QDialogButtonBox *buttonBox;
    QWidget *quickUnlockPage;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_5;
    QVBoxLayout *verticalLayout_5;
    QSpacerItem *verticalSpacer_7;
    QPushButton *quickUnlockButton;
    QPushButton *resetQuickUnlockButton;
    QSpacerItem *verticalSpacer_8;
    QSpacerItem *horizontalSpacer_6;
    QSpacerItem *verticalSpacer;
    QSpacerItem *horizontalSpacer_4;

    void setupUi(QWidget *DatabaseOpenWidget)
    {
        if (DatabaseOpenWidget->objectName().isEmpty())
            DatabaseOpenWidget->setObjectName(QString::fromUtf8("DatabaseOpenWidget"));
        DatabaseOpenWidget->resize(745, 544);
        verticalLayout = new QVBoxLayout(DatabaseOpenWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        messageWidget = new MessageWidget(DatabaseOpenWidget);
        messageWidget->setObjectName(QString::fromUtf8("messageWidget"));

        verticalLayout->addWidget(messageWidget);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_3 = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        formContainer = new QWidget(DatabaseOpenWidget);
        formContainer->setObjectName(QString::fromUtf8("formContainer"));
        verticalLayout_4 = new QVBoxLayout(formContainer);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_4->addItem(verticalSpacer_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(9);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        dbIconLabel = new QLabel(formContainer);
        dbIconLabel->setObjectName(QString::fromUtf8("dbIconLabel"));
        dbIconLabel->setMinimumSize(QSize(32, 32));
        dbIconLabel->setScaledContents(true);

        horizontalLayout->addWidget(dbIconLabel);

        labelHeadline = new QLabel(formContainer);
        labelHeadline->setObjectName(QString::fromUtf8("labelHeadline"));
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        font.setWeight(75);
        labelHeadline->setFont(font);

        horizontalLayout->addWidget(labelHeadline);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout_4->addLayout(horizontalLayout);

        fileNameLabel = new ElidedLabel(formContainer);
        fileNameLabel->setObjectName(QString::fromUtf8("fileNameLabel"));
        fileNameLabel->setText(QString::fromUtf8("filename.kdbx"));

        verticalLayout_4->addWidget(fileNameLabel);

        verticalSpacer_3 = new QSpacerItem(439, 13, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_4->addItem(verticalSpacer_3);

        centralStack = new QStackedWidget(formContainer);
        centralStack->setObjectName(QString::fromUtf8("centralStack"));
        centralStack->setMinimumSize(QSize(650, 0));
        centralStack->setFrameShape(QFrame::StyledPanel);
        centralStack->setLineWidth(1);
        mainPage = new QWidget();
        mainPage->setObjectName(QString::fromUtf8("mainPage"));
        verticalLayout_6 = new QVBoxLayout(mainPage);
        verticalLayout_6->setSpacing(5);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(30, 25, 30, 25);
        enterPasswordComponent = new QFrame(mainPage);
        enterPasswordComponent->setObjectName(QString::fromUtf8("enterPasswordComponent"));
        enterPasswordComponent->setLineWidth(0);
        enterPasswordComponentLayout = new QVBoxLayout(enterPasswordComponent);
        enterPasswordComponentLayout->setSpacing(10);
        enterPasswordComponentLayout->setObjectName(QString::fromUtf8("enterPasswordComponentLayout"));
        enterPasswordComponentLayout->setContentsMargins(0, 0, 0, 10);
        passwordLabel = new QLabel(enterPasswordComponent);
        passwordLabel->setObjectName(QString::fromUtf8("passwordLabel"));

        enterPasswordComponentLayout->addWidget(passwordLabel);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        editPassword = new PasswordWidget(enterPasswordComponent);
        editPassword->setObjectName(QString::fromUtf8("editPassword"));
        editPassword->setFocusPolicy(Qt::StrongFocus);

        verticalLayout_2->addWidget(editPassword);

        hardwareKeyProgress = new QProgressBar(enterPasswordComponent);
        hardwareKeyProgress->setObjectName(QString::fromUtf8("hardwareKeyProgress"));
        hardwareKeyProgress->setMaximumSize(QSize(16777215, 4));
        hardwareKeyProgress->setMinimum(0);
        hardwareKeyProgress->setMaximum(0);
        hardwareKeyProgress->setValue(-1);
        hardwareKeyProgress->setTextVisible(false);

        verticalLayout_2->addWidget(hardwareKeyProgress);


        enterPasswordComponentLayout->addLayout(verticalLayout_2);


        verticalLayout_6->addWidget(enterPasswordComponent);

        selectKeyFileComponent = new QFrame(mainPage);
        selectKeyFileComponent->setObjectName(QString::fromUtf8("selectKeyFileComponent"));
        selectKeyFileComponent->setFrameShape(QFrame::NoFrame);
        selectKeyFileComponent->setFrameShadow(QFrame::Plain);
        selectKeyFileComponent->setLineWidth(0);
        selectKeyFileComponentLayout = new QVBoxLayout(selectKeyFileComponent);
        selectKeyFileComponentLayout->setSpacing(10);
        selectKeyFileComponentLayout->setObjectName(QString::fromUtf8("selectKeyFileComponentLayout"));
        selectKeyFileComponentLayout->setContentsMargins(0, 0, 0, 10);
        selectKeyFileLabel = new QLabel(selectKeyFileComponent);
        selectKeyFileLabel->setObjectName(QString::fromUtf8("selectKeyFileLabel"));

        selectKeyFileComponentLayout->addWidget(selectKeyFileLabel);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        keyFileLineEdit = new PasswordWidget(selectKeyFileComponent);
        keyFileLineEdit->setObjectName(QString::fromUtf8("keyFileLineEdit"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(keyFileLineEdit->sizePolicy().hasHeightForWidth());
        keyFileLineEdit->setSizePolicy(sizePolicy);
        keyFileLineEdit->setFocusPolicy(Qt::StrongFocus);

        horizontalLayout_7->addWidget(keyFileLineEdit);

        buttonBrowseFile = new QPushButton(selectKeyFileComponent);
        buttonBrowseFile->setObjectName(QString::fromUtf8("buttonBrowseFile"));

        horizontalLayout_7->addWidget(buttonBrowseFile);

        horizontalLayout_7->setStretch(0, 1);

        selectKeyFileComponentLayout->addLayout(horizontalLayout_7);


        verticalLayout_6->addWidget(selectKeyFileComponent);

        addAdditionalKeysComponent = new QFrame(mainPage);
        addAdditionalKeysComponent->setObjectName(QString::fromUtf8("addAdditionalKeysComponent"));
        addAdditionalKeysComponent->setLineWidth(0);
        addAdditionalKeysComponentLayout = new QHBoxLayout(addAdditionalKeysComponent);
        addAdditionalKeysComponentLayout->setObjectName(QString::fromUtf8("addAdditionalKeysComponentLayout"));
        addAdditionalKeysComponentLayout->setSizeConstraint(QLayout::SetMinimumSize);
        addAdditionalKeysComponentLayout->setContentsMargins(0, 0, 0, 0);
        hardwareKeyComponent = new QFrame(addAdditionalKeysComponent);
        hardwareKeyComponent->setObjectName(QString::fromUtf8("hardwareKeyComponent"));
        hardwareKeyComponentLayout = new QHBoxLayout(hardwareKeyComponent);
        hardwareKeyComponentLayout->setSpacing(5);
        hardwareKeyComponentLayout->setObjectName(QString::fromUtf8("hardwareKeyComponentLayout"));
        hardwareKeyComponentLayout->setContentsMargins(0, 0, 0, 0);
        useHardwareKeyCheckBox = new QCheckBox(hardwareKeyComponent);
        useHardwareKeyCheckBox->setObjectName(QString::fromUtf8("useHardwareKeyCheckBox"));
        useHardwareKeyCheckBox->setText(QString::fromUtf8("Use Hardware Security Key [Serial: 11111111]"));

        hardwareKeyComponentLayout->addWidget(useHardwareKeyCheckBox);

        hardwareKeyCombo = new QComboBox(hardwareKeyComponent);
        hardwareKeyCombo->setObjectName(QString::fromUtf8("hardwareKeyCombo"));
        hardwareKeyCombo->setEnabled(false);
        hardwareKeyCombo->setMinimumSize(QSize(200, 0));
        hardwareKeyCombo->setMaximumSize(QSize(300, 16777215));
        hardwareKeyCombo->setEditable(false);

        hardwareKeyComponentLayout->addWidget(hardwareKeyCombo);


        addAdditionalKeysComponentLayout->addWidget(hardwareKeyComponent);

        noHardwareKeysFoundLabel = new QLabel(addAdditionalKeysComponent);
        noHardwareKeysFoundLabel->setObjectName(QString::fromUtf8("noHardwareKeysFoundLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(noHardwareKeysFoundLabel->sizePolicy().hasHeightForWidth());
        noHardwareKeysFoundLabel->setSizePolicy(sizePolicy1);
        noHardwareKeysFoundLabel->setMargin(1);

        addAdditionalKeysComponentLayout->addWidget(noHardwareKeysFoundLabel);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        addAdditionalKeysComponentLayout->addItem(horizontalSpacer);

        refreshHardwareKeys = new QPushButton(addAdditionalKeysComponent);
        refreshHardwareKeys->setObjectName(QString::fromUtf8("refreshHardwareKeys"));
        refreshHardwareKeys->setCursor(QCursor(Qt::PointingHandCursor));
        refreshHardwareKeys->setStyleSheet(QString::fromUtf8("QPushButton { background-color: transparent; border: none; } "));
        refreshHardwareKeys->setText(QString::fromUtf8(""));

        addAdditionalKeysComponentLayout->addWidget(refreshHardwareKeys);

        addKeyFileLinkLabel = new QLabel(addAdditionalKeysComponent);
        addKeyFileLinkLabel->setObjectName(QString::fromUtf8("addKeyFileLinkLabel"));
        addKeyFileLinkLabel->setCursor(QCursor(Qt::PointingHandCursor));
        addKeyFileLinkLabel->setFocusPolicy(Qt::TabFocus);
        addKeyFileLinkLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        addKeyFileLinkLabel->setMargin(1);
        addKeyFileLinkLabel->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

        addAdditionalKeysComponentLayout->addWidget(addKeyFileLinkLabel);


        verticalLayout_6->addWidget(addAdditionalKeysComponent);

        dialogButtonsLayout = new QHBoxLayout();
        dialogButtonsLayout->setSpacing(0);
        dialogButtonsLayout->setObjectName(QString::fromUtf8("dialogButtonsLayout"));
        dialogButtonsLayout->setContentsMargins(-1, 25, -1, 5);
        buttonBox = new QDialogButtonBox(mainPage);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setFocusPolicy(Qt::StrongFocus);
        buttonBox->setStandardButtons(QDialogButtonBox::Close|QDialogButtonBox::Ok);

        dialogButtonsLayout->addWidget(buttonBox, 0, Qt::AlignRight);


        verticalLayout_6->addLayout(dialogButtonsLayout);

        centralStack->addWidget(mainPage);
        quickUnlockPage = new QWidget();
        quickUnlockPage->setObjectName(QString::fromUtf8("quickUnlockPage"));
        horizontalLayout_2 = new QHBoxLayout(quickUnlockPage);
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(10, 10, 10, 10);
        horizontalSpacer_5 = new QSpacerItem(30, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_5);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalSpacer_7 = new QSpacerItem(0, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_7);

        quickUnlockButton = new QPushButton(quickUnlockPage);
        quickUnlockButton->setObjectName(QString::fromUtf8("quickUnlockButton"));
        quickUnlockButton->setMinimumSize(QSize(200, 60));
        QFont font1;
        font1.setPointSize(10);
        font1.setBold(true);
        font1.setWeight(75);
        quickUnlockButton->setFont(font1);

        verticalLayout_5->addWidget(quickUnlockButton);

        resetQuickUnlockButton = new QPushButton(quickUnlockPage);
        resetQuickUnlockButton->setObjectName(QString::fromUtf8("resetQuickUnlockButton"));

        verticalLayout_5->addWidget(resetQuickUnlockButton);

        verticalSpacer_8 = new QSpacerItem(0, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_8);


        horizontalLayout_2->addLayout(verticalLayout_5);

        horizontalSpacer_6 = new QSpacerItem(30, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_6);

        centralStack->addWidget(quickUnlockPage);

        verticalLayout_4->addWidget(centralStack);

        verticalSpacer = new QSpacerItem(20, 55, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);

        verticalLayout_4->addItem(verticalSpacer);

        verticalLayout_4->setStretch(0, 1);
        verticalLayout_4->setStretch(5, 2);

        horizontalLayout_3->addWidget(formContainer);

        horizontalSpacer_4 = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_4);

        horizontalLayout_3->setStretch(0, 1);
        horizontalLayout_3->setStretch(2, 1);

        verticalLayout->addLayout(horizontalLayout_3);

        QWidget::setTabOrder(quickUnlockButton, resetQuickUnlockButton);
        QWidget::setTabOrder(resetQuickUnlockButton, editPassword);
        QWidget::setTabOrder(editPassword, keyFileLineEdit);
        QWidget::setTabOrder(keyFileLineEdit, buttonBrowseFile);
        QWidget::setTabOrder(buttonBrowseFile, useHardwareKeyCheckBox);
        QWidget::setTabOrder(useHardwareKeyCheckBox, hardwareKeyCombo);
        QWidget::setTabOrder(hardwareKeyCombo, refreshHardwareKeys);
        QWidget::setTabOrder(refreshHardwareKeys, addKeyFileLinkLabel);
        QWidget::setTabOrder(addKeyFileLinkLabel, buttonBox);

        retranslateUi(DatabaseOpenWidget);

        centralStack->setCurrentIndex(0);
        quickUnlockButton->setDefault(true);


        QMetaObject::connectSlotsByName(DatabaseOpenWidget);
    } // setupUi

    void retranslateUi(QWidget *DatabaseOpenWidget)
    {
#if QT_CONFIG(accessibility)
        DatabaseOpenWidget->setAccessibleName(QCoreApplication::translate("DatabaseOpenWidget", "Unlock KeePassXC Database", nullptr));
#endif // QT_CONFIG(accessibility)
        dbIconLabel->setText(QString());
        labelHeadline->setText(QCoreApplication::translate("DatabaseOpenWidget", "Unlock KeePassXC Database", nullptr));
        passwordLabel->setText(QCoreApplication::translate("DatabaseOpenWidget", "Enter Password:", nullptr));
#if QT_CONFIG(accessibility)
        editPassword->setAccessibleName(QCoreApplication::translate("DatabaseOpenWidget", "Password field", nullptr));
#endif // QT_CONFIG(accessibility)
        selectKeyFileLabel->setText(QCoreApplication::translate("DatabaseOpenWidget", "Select Key File:", nullptr));
#if QT_CONFIG(accessibility)
        keyFileLineEdit->setAccessibleName(QCoreApplication::translate("DatabaseOpenWidget", "Key file to unlock the database", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        buttonBrowseFile->setToolTip(QCoreApplication::translate("DatabaseOpenWidget", "Browse for key file", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        buttonBrowseFile->setAccessibleName(QCoreApplication::translate("DatabaseOpenWidget", "Browse for key file", nullptr));
#endif // QT_CONFIG(accessibility)
        buttonBrowseFile->setText(QCoreApplication::translate("DatabaseOpenWidget", "Browse\342\200\246", nullptr));
#if QT_CONFIG(accessibility)
        hardwareKeyCombo->setAccessibleName(QCoreApplication::translate("DatabaseOpenWidget", "Hardware key slot selection", nullptr));
#endif // QT_CONFIG(accessibility)
        noHardwareKeysFoundLabel->setText(QCoreApplication::translate("DatabaseOpenWidget", "No hardware keys found.", nullptr));
#if QT_CONFIG(tooltip)
        refreshHardwareKeys->setToolTip(QCoreApplication::translate("DatabaseOpenWidget", "Refresh Hardware Keys", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        refreshHardwareKeys->setAccessibleName(QCoreApplication::translate("DatabaseOpenWidget", "Refresh Hardware Keys", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        addKeyFileLinkLabel->setToolTip(QCoreApplication::translate("DatabaseOpenWidget", "<p>In addition to a password, you can use a secret file to enhance the security of your database. This file can be generated in your database's security settings.</p><p>This is <strong>not</strong> your *.kdbx database file!</p>", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        addKeyFileLinkLabel->setAccessibleDescription(QCoreApplication::translate("DatabaseOpenWidget", "Click to add a key file.", nullptr));
#endif // QT_CONFIG(accessibility)
        addKeyFileLinkLabel->setText(QCoreApplication::translate("DatabaseOpenWidget", "<a href=\"#\" style=\"text-decoration: underline\">I have a key file</a>", nullptr));
        quickUnlockButton->setText(QCoreApplication::translate("DatabaseOpenWidget", "Unlock Database", nullptr));
        resetQuickUnlockButton->setText(QCoreApplication::translate("DatabaseOpenWidget", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DatabaseOpenWidget: public Ui_DatabaseOpenWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATABASEOPENWIDGET_H
