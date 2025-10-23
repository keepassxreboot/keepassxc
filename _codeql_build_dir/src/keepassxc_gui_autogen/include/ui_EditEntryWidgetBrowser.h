/********************************************************************************
** Form generated from reading UI file 'EditEntryWidgetBrowser.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYWIDGETBROWSER_H
#define UI_EDITENTRYWIDGETBROWSER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/MessageWidget.h"

QT_BEGIN_NAMESPACE

class Ui_EditEntryWidgetBrowser
{
public:
    QVBoxLayout *verticalLayout_1;
    QLabel *label;
    MessageWidget *messageWidget;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QCheckBox *hideEntryCheckbox;
    QCheckBox *skipAutoSubmitCheckbox;
    QCheckBox *onlyHttpAuthCheckbox;
    QCheckBox *notHttpAuthCheckbox;
    QGroupBox *groupBox_3;
    QHBoxLayout *horizontalLayout_1;
    QListView *additionalURLsView;
    QVBoxLayout *additionalURLsButtonLayout;
    QPushButton *addURLButton;
    QPushButton *removeURLButton;
    QPushButton *editURLButton;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *EditEntryWidgetBrowser)
    {
        if (EditEntryWidgetBrowser->objectName().isEmpty())
            EditEntryWidgetBrowser->setObjectName(QString::fromUtf8("EditEntryWidgetBrowser"));
        EditEntryWidgetBrowser->resize(374, 348);
        verticalLayout_1 = new QVBoxLayout(EditEntryWidgetBrowser);
        verticalLayout_1->setObjectName(QString::fromUtf8("verticalLayout_1"));
        verticalLayout_1->setContentsMargins(0, 0, 0, 0);
        label = new QLabel(EditEntryWidgetBrowser);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout_1->addWidget(label);

        messageWidget = new MessageWidget(EditEntryWidgetBrowser);
        messageWidget->setObjectName(QString::fromUtf8("messageWidget"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(messageWidget->sizePolicy().hasHeightForWidth());
        messageWidget->setSizePolicy(sizePolicy);

        verticalLayout_1->addWidget(messageWidget);

        groupBox_2 = new QGroupBox(EditEntryWidgetBrowser);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        hideEntryCheckbox = new QCheckBox(groupBox_2);
        hideEntryCheckbox->setObjectName(QString::fromUtf8("hideEntryCheckbox"));

        verticalLayout_2->addWidget(hideEntryCheckbox);

        skipAutoSubmitCheckbox = new QCheckBox(groupBox_2);
        skipAutoSubmitCheckbox->setObjectName(QString::fromUtf8("skipAutoSubmitCheckbox"));

        verticalLayout_2->addWidget(skipAutoSubmitCheckbox);

        onlyHttpAuthCheckbox = new QCheckBox(groupBox_2);
        onlyHttpAuthCheckbox->setObjectName(QString::fromUtf8("onlyHttpAuthCheckbox"));

        verticalLayout_2->addWidget(onlyHttpAuthCheckbox);

        notHttpAuthCheckbox = new QCheckBox(groupBox_2);
        notHttpAuthCheckbox->setObjectName(QString::fromUtf8("notHttpAuthCheckbox"));

        verticalLayout_2->addWidget(notHttpAuthCheckbox);


        verticalLayout_1->addWidget(groupBox_2);

        groupBox_3 = new QGroupBox(EditEntryWidgetBrowser);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        horizontalLayout_1 = new QHBoxLayout(groupBox_3);
        horizontalLayout_1->setObjectName(QString::fromUtf8("horizontalLayout_1"));
        additionalURLsView = new QListView(groupBox_3);
        additionalURLsView->setObjectName(QString::fromUtf8("additionalURLsView"));
        additionalURLsView->setMinimumSize(QSize(0, 0));
        additionalURLsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        additionalURLsView->setResizeMode(QListView::Adjust);

        horizontalLayout_1->addWidget(additionalURLsView);

        additionalURLsButtonLayout = new QVBoxLayout();
        additionalURLsButtonLayout->setObjectName(QString::fromUtf8("additionalURLsButtonLayout"));
        addURLButton = new QPushButton(groupBox_3);
        addURLButton->setObjectName(QString::fromUtf8("addURLButton"));

        additionalURLsButtonLayout->addWidget(addURLButton);

        removeURLButton = new QPushButton(groupBox_3);
        removeURLButton->setObjectName(QString::fromUtf8("removeURLButton"));
        removeURLButton->setEnabled(false);

        additionalURLsButtonLayout->addWidget(removeURLButton);

        editURLButton = new QPushButton(groupBox_3);
        editURLButton->setObjectName(QString::fromUtf8("editURLButton"));
        editURLButton->setEnabled(false);

        additionalURLsButtonLayout->addWidget(editURLButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        additionalURLsButtonLayout->addItem(verticalSpacer);


        horizontalLayout_1->addLayout(additionalURLsButtonLayout);


        verticalLayout_1->addWidget(groupBox_3);

        QWidget::setTabOrder(skipAutoSubmitCheckbox, onlyHttpAuthCheckbox);
        QWidget::setTabOrder(onlyHttpAuthCheckbox, additionalURLsView);
        QWidget::setTabOrder(additionalURLsView, addURLButton);
        QWidget::setTabOrder(addURLButton, removeURLButton);
        QWidget::setTabOrder(removeURLButton, editURLButton);

        retranslateUi(EditEntryWidgetBrowser);

        QMetaObject::connectSlotsByName(EditEntryWidgetBrowser);
    } // setupUi

    void retranslateUi(QWidget *EditEntryWidgetBrowser)
    {
        label->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "These settings affect the entry's behaviour with the browser extension.", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("EditEntryWidgetBrowser", "General", nullptr));
        hideEntryCheckbox->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Hide this entry from the browser extension", nullptr));
        skipAutoSubmitCheckbox->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Skip Auto-Submit for this entry", nullptr));
#if QT_CONFIG(tooltip)
        onlyHttpAuthCheckbox->setToolTip(QCoreApplication::translate("EditEntryWidgetBrowser", "Only send this entry to the browser for HTTP Auth dialogs. If enabled, normal login forms will not show this entry for selection.", nullptr));
#endif // QT_CONFIG(tooltip)
        onlyHttpAuthCheckbox->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Use this entry only with HTTP Basic Auth", nullptr));
#if QT_CONFIG(tooltip)
        notHttpAuthCheckbox->setToolTip(QCoreApplication::translate("EditEntryWidgetBrowser", "Do not send this entry to the browser for HTTP Auth dialogs. If enabled, HTTP Auth dialogs will not show this entry for selection.", nullptr));
#endif // QT_CONFIG(tooltip)
        notHttpAuthCheckbox->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Do not use this entry with HTTP Basic Auth", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("EditEntryWidgetBrowser", "Additional URLs", nullptr));
        addURLButton->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Add", nullptr));
        removeURLButton->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Remove", nullptr));
        editURLButton->setText(QCoreApplication::translate("EditEntryWidgetBrowser", "Edit", nullptr));
        (void)EditEntryWidgetBrowser;
    } // retranslateUi

};

namespace Ui {
    class EditEntryWidgetBrowser: public Ui_EditEntryWidgetBrowser {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYWIDGETBROWSER_H
