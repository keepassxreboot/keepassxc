/********************************************************************************
** Form generated from reading UI file 'WelcomeWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WELCOMEWIDGET_H
#define UI_WELCOMEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_WelcomeWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QVBoxLayout *verticalLayout_2;
    QLabel *iconLabel;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer_3;
    QLabel *welcomeLabel;
    QLabel *startLabel;
    QSpacerItem *verticalSpacer;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *buttonNewDatabase;
    QPushButton *buttonOpenDatabase;
    QPushButton *buttonImport;
    QSpacerItem *verticalSpacer_2;
    QLabel *recentLabel;
    QListWidget *recentListWidget;

    void setupUi(QWidget *WelcomeWidget)
    {
        if (WelcomeWidget->objectName().isEmpty())
            WelcomeWidget->setObjectName(QString::fromUtf8("WelcomeWidget"));
        WelcomeWidget->resize(450, 419);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(WelcomeWidget->sizePolicy().hasHeightForWidth());
        WelcomeWidget->setSizePolicy(sizePolicy);
        WelcomeWidget->setMinimumSize(QSize(450, 0));
        verticalLayout = new QVBoxLayout(WelcomeWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        iconLabel = new QLabel(WelcomeWidget);
        iconLabel->setObjectName(QString::fromUtf8("iconLabel"));
        sizePolicy.setHeightForWidth(iconLabel->sizePolicy().hasHeightForWidth());
        iconLabel->setSizePolicy(sizePolicy);
        iconLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        verticalLayout_2->addWidget(iconLabel);


        horizontalLayout->addLayout(verticalLayout_2);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer_3 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer_3);

        welcomeLabel = new QLabel(WelcomeWidget);
        welcomeLabel->setObjectName(QString::fromUtf8("welcomeLabel"));
        welcomeLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(welcomeLabel);

        startLabel = new QLabel(WelcomeWidget);
        startLabel->setObjectName(QString::fromUtf8("startLabel"));
        startLabel->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(startLabel);

        verticalSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        buttonNewDatabase = new QPushButton(WelcomeWidget);
        buttonNewDatabase->setObjectName(QString::fromUtf8("buttonNewDatabase"));

        horizontalLayout_2->addWidget(buttonNewDatabase);

        buttonOpenDatabase = new QPushButton(WelcomeWidget);
        buttonOpenDatabase->setObjectName(QString::fromUtf8("buttonOpenDatabase"));

        horizontalLayout_2->addWidget(buttonOpenDatabase);

        buttonImport = new QPushButton(WelcomeWidget);
        buttonImport->setObjectName(QString::fromUtf8("buttonImport"));

        horizontalLayout_2->addWidget(buttonImport);


        verticalLayout->addLayout(horizontalLayout_2);

        verticalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer_2);

        recentLabel = new QLabel(WelcomeWidget);
        recentLabel->setObjectName(QString::fromUtf8("recentLabel"));

        verticalLayout->addWidget(recentLabel);

        recentListWidget = new QListWidget(WelcomeWidget);
        recentListWidget->setObjectName(QString::fromUtf8("recentListWidget"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(recentListWidget->sizePolicy().hasHeightForWidth());
        recentListWidget->setSizePolicy(sizePolicy1);
        recentListWidget->setMinimumSize(QSize(0, 0));
        recentListWidget->setMaximumSize(QSize(16777215, 110));

        verticalLayout->addWidget(recentListWidget);

        QWidget::setTabOrder(buttonImport, recentListWidget);

        retranslateUi(WelcomeWidget);

        QMetaObject::connectSlotsByName(WelcomeWidget);
    } // setupUi

    void retranslateUi(QWidget *WelcomeWidget)
    {
        startLabel->setText(QCoreApplication::translate("WelcomeWidget", "Start storing your passwords securely in a KeePassXC database", nullptr));
        buttonNewDatabase->setText(QCoreApplication::translate("WelcomeWidget", "Create Database", nullptr));
        buttonOpenDatabase->setText(QCoreApplication::translate("WelcomeWidget", "Open Database", nullptr));
        buttonImport->setText(QCoreApplication::translate("WelcomeWidget", "Import File", nullptr));
        recentLabel->setText(QCoreApplication::translate("WelcomeWidget", "Recent databases", nullptr));
#if QT_CONFIG(accessibility)
        recentListWidget->setAccessibleName(QCoreApplication::translate("WelcomeWidget", "Open a recent database", nullptr));
#endif // QT_CONFIG(accessibility)
        (void)WelcomeWidget;
    } // retranslateUi

};

namespace Ui {
    class WelcomeWidget: public Ui_WelcomeWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WELCOMEWIDGET_H
