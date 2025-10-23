/********************************************************************************
** Form generated from reading UI file 'EditGroupWidgetBrowser.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITGROUPWIDGETBROWSER_H
#define UI_EDITGROUPWIDGETBROWSER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditGroupWidgetBrowser
{
public:
    QVBoxLayout *verticalLayout_1;
    QLabel *label;
    QGridLayout *gridLayout;
    QLabel *browserIntegrationHideEntriesLabel;
    QComboBox *browserIntegrationHideEntriesComboBox;
    QLabel *browserIntegrationSkipAutoSubmitLabel;
    QComboBox *browserIntegrationSkipAutoSubmitComboBox;
    QLabel *browserIntegrationOnlyHttpAuthLabel;
    QComboBox *browserIntegrationOnlyHttpAuthComboBox;
    QLabel *browserIntegrationNotHttpAuthLabel;
    QComboBox *browserIntegrationNotHttpAuthComboBox;
    QLabel *browserIntegrationOmitWwwLabel;
    QComboBox *browserIntegrationOmitWwwCombobox;
    QLabel *browserIntegrationRestrictKey;
    QComboBox *browserIntegrationRestrictKeyCombobox;
    QSpacerItem *verticalSpacer_1;

    void setupUi(QWidget *EditGroupWidgetBrowser)
    {
        if (EditGroupWidgetBrowser->objectName().isEmpty())
            EditGroupWidgetBrowser->setObjectName(QString::fromUtf8("EditGroupWidgetBrowser"));
        EditGroupWidgetBrowser->resize(539, 523);
        verticalLayout_1 = new QVBoxLayout(EditGroupWidgetBrowser);
        verticalLayout_1->setObjectName(QString::fromUtf8("verticalLayout_1"));
        verticalLayout_1->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(EditGroupWidgetBrowser);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout_1->addWidget(label);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(8);
        gridLayout->setContentsMargins(0, 10, 0, 0);
        browserIntegrationHideEntriesLabel = new QLabel(EditGroupWidgetBrowser);
        browserIntegrationHideEntriesLabel->setObjectName(QString::fromUtf8("browserIntegrationHideEntriesLabel"));
        browserIntegrationHideEntriesLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(browserIntegrationHideEntriesLabel, 0, 0, 1, 1);

        browserIntegrationHideEntriesComboBox = new QComboBox(EditGroupWidgetBrowser);
        browserIntegrationHideEntriesComboBox->setObjectName(QString::fromUtf8("browserIntegrationHideEntriesComboBox"));

        gridLayout->addWidget(browserIntegrationHideEntriesComboBox, 0, 1, 1, 1);

        browserIntegrationSkipAutoSubmitLabel = new QLabel(EditGroupWidgetBrowser);
        browserIntegrationSkipAutoSubmitLabel->setObjectName(QString::fromUtf8("browserIntegrationSkipAutoSubmitLabel"));
        browserIntegrationSkipAutoSubmitLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(browserIntegrationSkipAutoSubmitLabel, 1, 0, 1, 1);

        browserIntegrationSkipAutoSubmitComboBox = new QComboBox(EditGroupWidgetBrowser);
        browserIntegrationSkipAutoSubmitComboBox->setObjectName(QString::fromUtf8("browserIntegrationSkipAutoSubmitComboBox"));

        gridLayout->addWidget(browserIntegrationSkipAutoSubmitComboBox, 1, 1, 1, 1);

        browserIntegrationOnlyHttpAuthLabel = new QLabel(EditGroupWidgetBrowser);
        browserIntegrationOnlyHttpAuthLabel->setObjectName(QString::fromUtf8("browserIntegrationOnlyHttpAuthLabel"));
        browserIntegrationOnlyHttpAuthLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(browserIntegrationOnlyHttpAuthLabel, 2, 0, 1, 1);

        browserIntegrationOnlyHttpAuthComboBox = new QComboBox(EditGroupWidgetBrowser);
        browserIntegrationOnlyHttpAuthComboBox->setObjectName(QString::fromUtf8("browserIntegrationOnlyHttpAuthComboBox"));

        gridLayout->addWidget(browserIntegrationOnlyHttpAuthComboBox, 2, 1, 1, 1);

        browserIntegrationNotHttpAuthLabel = new QLabel(EditGroupWidgetBrowser);
        browserIntegrationNotHttpAuthLabel->setObjectName(QString::fromUtf8("browserIntegrationNotHttpAuthLabel"));
        browserIntegrationNotHttpAuthLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(browserIntegrationNotHttpAuthLabel, 3, 0, 1, 1);

        browserIntegrationNotHttpAuthComboBox = new QComboBox(EditGroupWidgetBrowser);
        browserIntegrationNotHttpAuthComboBox->setObjectName(QString::fromUtf8("browserIntegrationNotHttpAuthComboBox"));

        gridLayout->addWidget(browserIntegrationNotHttpAuthComboBox, 3, 1, 1, 1);

        browserIntegrationOmitWwwLabel = new QLabel(EditGroupWidgetBrowser);
        browserIntegrationOmitWwwLabel->setObjectName(QString::fromUtf8("browserIntegrationOmitWwwLabel"));
        browserIntegrationOmitWwwLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(browserIntegrationOmitWwwLabel, 4, 0, 1, 1);

        browserIntegrationOmitWwwCombobox = new QComboBox(EditGroupWidgetBrowser);
        browserIntegrationOmitWwwCombobox->setObjectName(QString::fromUtf8("browserIntegrationOmitWwwCombobox"));

        gridLayout->addWidget(browserIntegrationOmitWwwCombobox, 4, 1, 1, 1);

        browserIntegrationRestrictKey = new QLabel(EditGroupWidgetBrowser);
        browserIntegrationRestrictKey->setObjectName(QString::fromUtf8("browserIntegrationRestrictKey"));
        browserIntegrationRestrictKey->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(browserIntegrationRestrictKey, 5, 0, 1, 1);

        browserIntegrationRestrictKeyCombobox = new QComboBox(EditGroupWidgetBrowser);
        browserIntegrationRestrictKeyCombobox->setObjectName(QString::fromUtf8("browserIntegrationRestrictKeyCombobox"));

        gridLayout->addWidget(browserIntegrationRestrictKeyCombobox, 5, 1, 1, 1);

        verticalSpacer_1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_1, 6, 0, 1, 1);

        gridLayout->setRowStretch(3, 1);
        gridLayout->setColumnStretch(1, 1);
        gridLayout->setRowMinimumHeight(3, 1);

        verticalLayout_1->addLayout(gridLayout);

        QWidget::setTabOrder(browserIntegrationHideEntriesComboBox, browserIntegrationSkipAutoSubmitComboBox);
        QWidget::setTabOrder(browserIntegrationSkipAutoSubmitComboBox, browserIntegrationOnlyHttpAuthComboBox);
        QWidget::setTabOrder(browserIntegrationOnlyHttpAuthComboBox, browserIntegrationNotHttpAuthComboBox);
        QWidget::setTabOrder(browserIntegrationNotHttpAuthComboBox, browserIntegrationOmitWwwCombobox);
        QWidget::setTabOrder(browserIntegrationOmitWwwCombobox, browserIntegrationRestrictKeyCombobox);

        retranslateUi(EditGroupWidgetBrowser);

        QMetaObject::connectSlotsByName(EditGroupWidgetBrowser);
    } // setupUi

    void retranslateUi(QWidget *EditGroupWidgetBrowser)
    {
        label->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "These settings affect to the group's behaviour with the browser extension.", nullptr));
        browserIntegrationHideEntriesLabel->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "Hide entries from browser extension:", nullptr));
#if QT_CONFIG(accessibility)
        browserIntegrationHideEntriesComboBox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetBrowser", "Hide entries from browser extension toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        browserIntegrationSkipAutoSubmitLabel->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "Skip Auto-Submit for entries:", nullptr));
#if QT_CONFIG(accessibility)
        browserIntegrationSkipAutoSubmitComboBox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetBrowser", "Skip Auto-Submit toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        browserIntegrationOnlyHttpAuthLabel->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "Use entries only with HTTP Basic Auth:", nullptr));
#if QT_CONFIG(accessibility)
        browserIntegrationOnlyHttpAuthComboBox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetBrowser", "Only HTTP Auth toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        browserIntegrationNotHttpAuthLabel->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "Do not use entries with HTTP Basic Auth:", nullptr));
#if QT_CONFIG(accessibility)
        browserIntegrationNotHttpAuthComboBox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetBrowser", "Do not use HTTP Auth toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        browserIntegrationOmitWwwLabel->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "Omit WWW subdomain from matching:", nullptr));
#if QT_CONFIG(accessibility)
        browserIntegrationOmitWwwCombobox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetBrowser", "Omit WWW subdomain from matching toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        browserIntegrationRestrictKey->setText(QCoreApplication::translate("EditGroupWidgetBrowser", "Restrict matching to given browser key:", nullptr));
#if QT_CONFIG(accessibility)
        browserIntegrationRestrictKeyCombobox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetBrowser", "Restrict matching to given browser key toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        (void)EditGroupWidgetBrowser;
    } // retranslateUi

};

namespace Ui {
    class EditGroupWidgetBrowser: public Ui_EditGroupWidgetBrowser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITGROUPWIDGETBROWSER_H
