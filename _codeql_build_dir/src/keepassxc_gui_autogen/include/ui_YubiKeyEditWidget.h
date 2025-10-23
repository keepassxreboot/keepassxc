/********************************************************************************
** Form generated from reading UI file 'YubiKeyEditWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_YUBIKEYEDITWIDGET_H
#define UI_YUBIKEYEDITWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_YubiKeyEditWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QVBoxLayout *verticalLayout_2;
    QComboBox *comboChallengeResponse;
    QProgressBar *yubikeyProgress;
    QVBoxLayout *verticalLayout_3;
    QPushButton *refreshHardwareKeys;
    QSpacerItem *verticalSpacer_2;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *YubiKeyEditWidget)
    {
        if (YubiKeyEditWidget->objectName().isEmpty())
            YubiKeyEditWidget->setObjectName(QString::fromUtf8("YubiKeyEditWidget"));
        YubiKeyEditWidget->resize(381, 64);
        verticalLayout = new QVBoxLayout(YubiKeyEditWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        comboChallengeResponse = new QComboBox(YubiKeyEditWidget);
        comboChallengeResponse->setObjectName(QString::fromUtf8("comboChallengeResponse"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(comboChallengeResponse->sizePolicy().hasHeightForWidth());
        comboChallengeResponse->setSizePolicy(sizePolicy);

        verticalLayout_2->addWidget(comboChallengeResponse);

        yubikeyProgress = new QProgressBar(YubiKeyEditWidget);
        yubikeyProgress->setObjectName(QString::fromUtf8("yubikeyProgress"));
        yubikeyProgress->setMaximumSize(QSize(16777215, 2));
        yubikeyProgress->setMaximum(0);
        yubikeyProgress->setValue(-1);
        yubikeyProgress->setTextVisible(false);

        verticalLayout_2->addWidget(yubikeyProgress);


        horizontalLayout->addLayout(verticalLayout_2);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        refreshHardwareKeys = new QPushButton(YubiKeyEditWidget);
        refreshHardwareKeys->setObjectName(QString::fromUtf8("refreshHardwareKeys"));

        verticalLayout_3->addWidget(refreshHardwareKeys);

        verticalSpacer_2 = new QSpacerItem(0, 2, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_3->addItem(verticalSpacer_2);


        horizontalLayout->addLayout(verticalLayout_3);


        verticalLayout->addLayout(horizontalLayout);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(YubiKeyEditWidget);

        QMetaObject::connectSlotsByName(YubiKeyEditWidget);
    } // setupUi

    void retranslateUi(QWidget *YubiKeyEditWidget)
    {
#if QT_CONFIG(accessibility)
        comboChallengeResponse->setAccessibleName(QCoreApplication::translate("YubiKeyEditWidget", "Hardware key slot selection", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        refreshHardwareKeys->setToolTip(QCoreApplication::translate("YubiKeyEditWidget", "Refresh hardware keys", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        refreshHardwareKeys->setAccessibleName(QCoreApplication::translate("YubiKeyEditWidget", "Refresh hardware keys", nullptr));
#endif // QT_CONFIG(accessibility)
        refreshHardwareKeys->setText(QString());
        (void)YubiKeyEditWidget;
    } // retranslateUi

};

namespace Ui {
    class YubiKeyEditWidget: public Ui_YubiKeyEditWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_YUBIKEYEDITWIDGET_H
