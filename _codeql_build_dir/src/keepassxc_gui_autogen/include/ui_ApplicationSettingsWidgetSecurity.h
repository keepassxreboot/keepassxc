/********************************************************************************
** Form generated from reading UI file 'ApplicationSettingsWidgetSecurity.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_APPLICATIONSETTINGSWIDGETSECURITY_H
#define UI_APPLICATIONSETTINGSWIDGETSECURITY_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ApplicationSettingsWidgetSecurity
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QSpinBox *lockDatabaseIdleSpinBox;
    QCheckBox *clearClipboardCheckBox;
    QCheckBox *clearSearchCheckBox;
    QSpinBox *clearSearchSpinBox;
    QSpinBox *clearClipboardSpinBox;
    QSpacerItem *horizontalSpacer;
    QCheckBox *lockDatabaseIdleCheckBox;
    QGroupBox *lockOptionsGroup;
    QVBoxLayout *verticalLayout_4;
    QCheckBox *quickUnlockCheckBox;
    QCheckBox *lockDatabaseOnScreenLockCheckBox;
    QCheckBox *lockDatabasesOnUserSwitchCheckBox;
    QCheckBox *lockDatabaseMinimizeCheckBox;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *passwordShowDotsCheckBox;
    QCheckBox *passwordsHiddenCheckBox;
    QCheckBox *passwordPreviewCleartextCheckBox;
    QCheckBox *hideTotpCheckBox;
    QCheckBox *hideNotesCheckBox;
    QGroupBox *privacy;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *fallbackToSearch;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QWidget *ApplicationSettingsWidgetSecurity)
    {
        if (ApplicationSettingsWidgetSecurity->objectName().isEmpty())
            ApplicationSettingsWidgetSecurity->setObjectName(QString::fromUtf8("ApplicationSettingsWidgetSecurity"));
        ApplicationSettingsWidgetSecurity->resize(364, 505);
        verticalLayout = new QVBoxLayout(ApplicationSettingsWidgetSecurity);
        verticalLayout->setSpacing(20);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(ApplicationSettingsWidgetSecurity);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        lockDatabaseIdleSpinBox = new QSpinBox(groupBox);
        lockDatabaseIdleSpinBox->setObjectName(QString::fromUtf8("lockDatabaseIdleSpinBox"));
        lockDatabaseIdleSpinBox->setEnabled(false);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(lockDatabaseIdleSpinBox->sizePolicy().hasHeightForWidth());
        lockDatabaseIdleSpinBox->setSizePolicy(sizePolicy);
        lockDatabaseIdleSpinBox->setMinimum(10);
        lockDatabaseIdleSpinBox->setMaximum(43200);
        lockDatabaseIdleSpinBox->setValue(240);

        gridLayout->addWidget(lockDatabaseIdleSpinBox, 1, 1, 1, 1);

        clearClipboardCheckBox = new QCheckBox(groupBox);
        clearClipboardCheckBox->setObjectName(QString::fromUtf8("clearClipboardCheckBox"));

        gridLayout->addWidget(clearClipboardCheckBox, 0, 0, 1, 1);

        clearSearchCheckBox = new QCheckBox(groupBox);
        clearSearchCheckBox->setObjectName(QString::fromUtf8("clearSearchCheckBox"));

        gridLayout->addWidget(clearSearchCheckBox, 2, 0, 1, 1);

        clearSearchSpinBox = new QSpinBox(groupBox);
        clearSearchSpinBox->setObjectName(QString::fromUtf8("clearSearchSpinBox"));
        clearSearchSpinBox->setEnabled(false);
        sizePolicy.setHeightForWidth(clearSearchSpinBox->sizePolicy().hasHeightForWidth());
        clearSearchSpinBox->setSizePolicy(sizePolicy);
        clearSearchSpinBox->setMinimum(1);
        clearSearchSpinBox->setMaximum(1440);
        clearSearchSpinBox->setValue(5);
        clearSearchSpinBox->setDisplayIntegerBase(10);

        gridLayout->addWidget(clearSearchSpinBox, 2, 1, 1, 1);

        clearClipboardSpinBox = new QSpinBox(groupBox);
        clearClipboardSpinBox->setObjectName(QString::fromUtf8("clearClipboardSpinBox"));
        clearClipboardSpinBox->setEnabled(false);
        sizePolicy.setHeightForWidth(clearClipboardSpinBox->sizePolicy().hasHeightForWidth());
        clearClipboardSpinBox->setSizePolicy(sizePolicy);
        clearClipboardSpinBox->setMinimum(1);
        clearClipboardSpinBox->setMaximum(999);
        clearClipboardSpinBox->setValue(10);

        gridLayout->addWidget(clearClipboardSpinBox, 0, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 0, 2, 1, 1);

        lockDatabaseIdleCheckBox = new QCheckBox(groupBox);
        lockDatabaseIdleCheckBox->setObjectName(QString::fromUtf8("lockDatabaseIdleCheckBox"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(lockDatabaseIdleCheckBox->sizePolicy().hasHeightForWidth());
        lockDatabaseIdleCheckBox->setSizePolicy(sizePolicy1);

        gridLayout->addWidget(lockDatabaseIdleCheckBox, 1, 0, 1, 1);

        gridLayout->setColumnStretch(2, 1);

        verticalLayout->addWidget(groupBox);

        lockOptionsGroup = new QGroupBox(ApplicationSettingsWidgetSecurity);
        lockOptionsGroup->setObjectName(QString::fromUtf8("lockOptionsGroup"));
        verticalLayout_4 = new QVBoxLayout(lockOptionsGroup);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        quickUnlockCheckBox = new QCheckBox(lockOptionsGroup);
        quickUnlockCheckBox->setObjectName(QString::fromUtf8("quickUnlockCheckBox"));

        verticalLayout_4->addWidget(quickUnlockCheckBox);

        lockDatabaseOnScreenLockCheckBox = new QCheckBox(lockOptionsGroup);
        lockDatabaseOnScreenLockCheckBox->setObjectName(QString::fromUtf8("lockDatabaseOnScreenLockCheckBox"));

        verticalLayout_4->addWidget(lockDatabaseOnScreenLockCheckBox);

        lockDatabasesOnUserSwitchCheckBox = new QCheckBox(lockOptionsGroup);
        lockDatabasesOnUserSwitchCheckBox->setObjectName(QString::fromUtf8("lockDatabasesOnUserSwitchCheckBox"));

        verticalLayout_4->addWidget(lockDatabasesOnUserSwitchCheckBox);

        lockDatabaseMinimizeCheckBox = new QCheckBox(lockOptionsGroup);
        lockDatabaseMinimizeCheckBox->setObjectName(QString::fromUtf8("lockDatabaseMinimizeCheckBox"));

        verticalLayout_4->addWidget(lockDatabaseMinimizeCheckBox);


        verticalLayout->addWidget(lockOptionsGroup);

        groupBox_2 = new QGroupBox(ApplicationSettingsWidgetSecurity);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        passwordShowDotsCheckBox = new QCheckBox(groupBox_2);
        passwordShowDotsCheckBox->setObjectName(QString::fromUtf8("passwordShowDotsCheckBox"));

        verticalLayout_2->addWidget(passwordShowDotsCheckBox);

        passwordsHiddenCheckBox = new QCheckBox(groupBox_2);
        passwordsHiddenCheckBox->setObjectName(QString::fromUtf8("passwordsHiddenCheckBox"));

        verticalLayout_2->addWidget(passwordsHiddenCheckBox);

        passwordPreviewCleartextCheckBox = new QCheckBox(groupBox_2);
        passwordPreviewCleartextCheckBox->setObjectName(QString::fromUtf8("passwordPreviewCleartextCheckBox"));

        verticalLayout_2->addWidget(passwordPreviewCleartextCheckBox);

        hideTotpCheckBox = new QCheckBox(groupBox_2);
        hideTotpCheckBox->setObjectName(QString::fromUtf8("hideTotpCheckBox"));

        verticalLayout_2->addWidget(hideTotpCheckBox);

        hideNotesCheckBox = new QCheckBox(groupBox_2);
        hideNotesCheckBox->setObjectName(QString::fromUtf8("hideNotesCheckBox"));

        verticalLayout_2->addWidget(hideNotesCheckBox);


        verticalLayout->addWidget(groupBox_2);

        privacy = new QGroupBox(ApplicationSettingsWidgetSecurity);
        privacy->setObjectName(QString::fromUtf8("privacy"));
        verticalLayout_3 = new QVBoxLayout(privacy);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        fallbackToSearch = new QCheckBox(privacy);
        fallbackToSearch->setObjectName(QString::fromUtf8("fallbackToSearch"));

        verticalLayout_3->addWidget(fallbackToSearch);


        verticalLayout->addWidget(privacy);

        verticalSpacer_2 = new QSpacerItem(20, 30, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);

        QWidget::setTabOrder(clearClipboardCheckBox, clearClipboardSpinBox);
        QWidget::setTabOrder(clearClipboardSpinBox, lockDatabaseIdleCheckBox);
        QWidget::setTabOrder(lockDatabaseIdleCheckBox, lockDatabaseIdleSpinBox);
        QWidget::setTabOrder(lockDatabaseIdleSpinBox, clearSearchCheckBox);
        QWidget::setTabOrder(clearSearchCheckBox, clearSearchSpinBox);
        QWidget::setTabOrder(clearSearchSpinBox, quickUnlockCheckBox);
        QWidget::setTabOrder(quickUnlockCheckBox, lockDatabaseOnScreenLockCheckBox);
        QWidget::setTabOrder(lockDatabaseOnScreenLockCheckBox, lockDatabasesOnUserSwitchCheckBox);
        QWidget::setTabOrder(lockDatabasesOnUserSwitchCheckBox, lockDatabaseMinimizeCheckBox);
        QWidget::setTabOrder(lockDatabaseMinimizeCheckBox, passwordShowDotsCheckBox);
        QWidget::setTabOrder(passwordShowDotsCheckBox, passwordsHiddenCheckBox);
        QWidget::setTabOrder(passwordsHiddenCheckBox, passwordPreviewCleartextCheckBox);
        QWidget::setTabOrder(passwordPreviewCleartextCheckBox, hideTotpCheckBox);
        QWidget::setTabOrder(hideTotpCheckBox, hideNotesCheckBox);
        QWidget::setTabOrder(hideNotesCheckBox, fallbackToSearch);

        retranslateUi(ApplicationSettingsWidgetSecurity);

        QMetaObject::connectSlotsByName(ApplicationSettingsWidgetSecurity);
    } // setupUi

    void retranslateUi(QWidget *ApplicationSettingsWidgetSecurity)
    {
        groupBox->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Timeouts", nullptr));
#if QT_CONFIG(accessibility)
        lockDatabaseIdleSpinBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Database lock timeout seconds", nullptr));
#endif // QT_CONFIG(accessibility)
        lockDatabaseIdleSpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", " sec", "Seconds"));
        clearClipboardCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Clear clipboard after", nullptr));
        clearSearchCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Clear search query after", nullptr));
        clearSearchSpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", " min", "Minutes"));
#if QT_CONFIG(accessibility)
        clearClipboardSpinBox->setAccessibleName(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Clipboard clear seconds", nullptr));
#endif // QT_CONFIG(accessibility)
        clearClipboardSpinBox->setSuffix(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", " sec", "Seconds"));
        lockDatabaseIdleCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Lock databases after inactivity of", nullptr));
        lockOptionsGroup->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Lock Options", nullptr));
        quickUnlockCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Enable database quick unlock (Touch ID / Windows Hello)", nullptr));
        lockDatabaseOnScreenLockCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Lock databases when session is locked or lid is closed", nullptr));
        lockDatabasesOnUserSwitchCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Lock databases when switching user", nullptr));
        lockDatabaseMinimizeCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Lock databases after minimizing the window", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Convenience", nullptr));
        passwordShowDotsCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Use placeholder for empty password fields", nullptr));
        passwordsHiddenCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Hide passwords when editing them", nullptr));
        passwordPreviewCleartextCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Hide passwords in the entry preview panel", nullptr));
        hideTotpCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Hide TOTP in the entry preview panel", nullptr));
        hideNotesCheckBox->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Hide notes in the entry preview panel", nullptr));
        privacy->setTitle(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Privacy", nullptr));
        fallbackToSearch->setText(QCoreApplication::translate("ApplicationSettingsWidgetSecurity", "Use DuckDuckGo service to download website icons", nullptr));
        (void)ApplicationSettingsWidgetSecurity;
    } // retranslateUi

};

namespace Ui {
    class ApplicationSettingsWidgetSecurity: public Ui_ApplicationSettingsWidgetSecurity {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_APPLICATIONSETTINGSWIDGETSECURITY_H
