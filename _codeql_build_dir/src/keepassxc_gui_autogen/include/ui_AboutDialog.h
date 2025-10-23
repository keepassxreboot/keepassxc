/********************************************************************************
** Form generated from reading UI file 'AboutDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout;
    QLabel *iconLabel;
    QLabel *nameLabel;
    QTabWidget *tabWidget;
    QWidget *aboutTab;
    QVBoxLayout *verticalLayout_3;
    QLabel *label_2;
    QLabel *label_5;
    QSpacerItem *verticalSpacer_3;
    QLabel *label_3;
    QSpacerItem *verticalSpacer_2;
    QLabel *label_8;
    QLabel *maintainers;
    QLabel *label_7;
    QSpacerItem *verticalSpacer;
    QWidget *contribTab;
    QVBoxLayout *verticalLayout;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_5;
    QLabel *contributors;
    QLabel *seeContributions;
    QWidget *debugTab;
    QVBoxLayout *verticalLayout_4;
    QLabel *label;
    QPlainTextEdit *debugInfo;
    QPushButton *copyToClipboard;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName(QString::fromUtf8("AboutDialog"));
        AboutDialog->resize(510, 443);
        verticalLayout_2 = new QVBoxLayout(AboutDialog);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(-1, 15, -1, 20);
        iconLabel = new QLabel(AboutDialog);
        iconLabel->setObjectName(QString::fromUtf8("iconLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(iconLabel->sizePolicy().hasHeightForWidth());
        iconLabel->setSizePolicy(sizePolicy);
        iconLabel->setMinimumSize(QSize(48, 48));
        iconLabel->setMaximumSize(QSize(48, 48));

        horizontalLayout->addWidget(iconLabel, 0, Qt::AlignVCenter);

        nameLabel = new QLabel(AboutDialog);
        nameLabel->setObjectName(QString::fromUtf8("nameLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(nameLabel->sizePolicy().hasHeightForWidth());
        nameLabel->setSizePolicy(sizePolicy1);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        nameLabel->setFont(font);
        nameLabel->setText(QString::fromUtf8("<span style=\"font-size: 20pt\"> KeePassXC ${VERSION}</span>"));
        nameLabel->setMargin(0);
        nameLabel->setIndent(11);

        horizontalLayout->addWidget(nameLabel, 0, Qt::AlignVCenter);


        verticalLayout_2->addLayout(horizontalLayout);

        tabWidget = new QTabWidget(AboutDialog);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        aboutTab = new QWidget();
        aboutTab->setObjectName(QString::fromUtf8("aboutTab"));
        verticalLayout_3 = new QVBoxLayout(aboutTab);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        label_2 = new QLabel(aboutTab);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy2);
        label_2->setText(QString::fromUtf8("Website: <a href=\"https://keepassxc.org/\" style=\"text-decoration: underline\">https://keepassxc.org</a>"));
        label_2->setOpenExternalLinks(true);

        verticalLayout_3->addWidget(label_2);

        label_5 = new QLabel(aboutTab);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setOpenExternalLinks(true);

        verticalLayout_3->addWidget(label_5);

        verticalSpacer_3 = new QSpacerItem(20, 5, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_3->addItem(verticalSpacer_3);

        label_3 = new QLabel(aboutTab);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy3);
        label_3->setWordWrap(true);

        verticalLayout_3->addWidget(label_3);

        verticalSpacer_2 = new QSpacerItem(0, 5, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_3->addItem(verticalSpacer_2);

        label_8 = new QLabel(aboutTab);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        verticalLayout_3->addWidget(label_8);

        maintainers = new QLabel(aboutTab);
        maintainers->setObjectName(QString::fromUtf8("maintainers"));
        sizePolicy2.setHeightForWidth(maintainers->sizePolicy().hasHeightForWidth());
        maintainers->setSizePolicy(sizePolicy2);
        maintainers->setText(QString::fromUtf8(""));
        maintainers->setOpenExternalLinks(true);
        maintainers->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

        verticalLayout_3->addWidget(maintainers);

        label_7 = new QLabel(aboutTab);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        sizePolicy2.setHeightForWidth(label_7->sizePolicy().hasHeightForWidth());
        label_7->setSizePolicy(sizePolicy2);
        label_7->setWordWrap(true);
        label_7->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

        verticalLayout_3->addWidget(label_7);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);

        tabWidget->addTab(aboutTab, QString());
        contribTab = new QWidget();
        contribTab->setObjectName(QString::fromUtf8("contribTab"));
        verticalLayout = new QVBoxLayout(contribTab);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        scrollArea = new QScrollArea(contribTab);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 466, 242));
        verticalLayout_5 = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        contributors = new QLabel(scrollAreaWidgetContents);
        contributors->setObjectName(QString::fromUtf8("contributors"));
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(contributors->sizePolicy().hasHeightForWidth());
        contributors->setSizePolicy(sizePolicy4);
        contributors->setCursor(QCursor(Qt::IBeamCursor));
        contributors->setText(QString::fromUtf8(""));
        contributors->setTextFormat(Qt::AutoText);
        contributors->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        contributors->setWordWrap(true);
        contributors->setMargin(5);
        contributors->setTextInteractionFlags(Qt::TextBrowserInteraction);

        verticalLayout_5->addWidget(contributors);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout->addWidget(scrollArea);

        seeContributions = new QLabel(contribTab);
        seeContributions->setObjectName(QString::fromUtf8("seeContributions"));
        seeContributions->setAlignment(Qt::AlignCenter);
        seeContributions->setOpenExternalLinks(true);

        verticalLayout->addWidget(seeContributions);

        tabWidget->addTab(contribTab, QString());
        debugTab = new QWidget();
        debugTab->setObjectName(QString::fromUtf8("debugTab"));
        verticalLayout_4 = new QVBoxLayout(debugTab);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        label = new QLabel(debugTab);
        label->setObjectName(QString::fromUtf8("label"));
        sizePolicy2.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy2);

        verticalLayout_4->addWidget(label);

        debugInfo = new QPlainTextEdit(debugTab);
        debugInfo->setObjectName(QString::fromUtf8("debugInfo"));
        debugInfo->setReadOnly(true);
        debugInfo->setPlainText(QString::fromUtf8(""));
        debugInfo->setTextInteractionFlags(Qt::TextBrowserInteraction);

        verticalLayout_4->addWidget(debugInfo);

        copyToClipboard = new QPushButton(debugTab);
        copyToClipboard->setObjectName(QString::fromUtf8("copyToClipboard"));

        verticalLayout_4->addWidget(copyToClipboard);

        tabWidget->addTab(debugTab, QString());

        verticalLayout_2->addWidget(tabWidget);

        buttonBox = new QDialogButtonBox(AboutDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        verticalLayout_2->addWidget(buttonBox);

        verticalLayout_2->setStretch(1, 1);
        QWidget::setTabOrder(tabWidget, scrollArea);
        QWidget::setTabOrder(scrollArea, debugInfo);
        QWidget::setTabOrder(debugInfo, copyToClipboard);

        retranslateUi(AboutDialog);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QDialog *AboutDialog)
    {
        AboutDialog->setWindowTitle(QCoreApplication::translate("AboutDialog", "About KeePassXC", nullptr));
        label_5->setText(QCoreApplication::translate("AboutDialog", "Report bugs at: <a href=\"https://github.com/keepassxreboot/keepassxc/issues\" style=\"text-decoration: underline;\">https://github.com</a>", nullptr));
        label_3->setText(QCoreApplication::translate("AboutDialog", "KeePassXC is distributed under the terms of the GNU General Public License (GPL) version 2 or (at your option) version 3.", nullptr));
        label_8->setText(QCoreApplication::translate("AboutDialog", "Project Maintainers:", nullptr));
        label_7->setText(QCoreApplication::translate("AboutDialog", "Special thanks from the KeePassXC team go to debfx for creating the original KeePassX.", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(aboutTab), QCoreApplication::translate("AboutDialog", "About", nullptr));
        seeContributions->setText(QCoreApplication::translate("AboutDialog", "<a href=\"https://github.com/keepassxreboot/keepassxc/graphs/contributors\">See Contributions on GitHub</a>", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(contribTab), QCoreApplication::translate("AboutDialog", "Contributors", nullptr));
        label->setText(QCoreApplication::translate("AboutDialog", "Include the following information whenever you report a bug:", nullptr));
        copyToClipboard->setText(QCoreApplication::translate("AboutDialog", "Copy to clipboard", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(debugTab), QCoreApplication::translate("AboutDialog", "Debug Info", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
