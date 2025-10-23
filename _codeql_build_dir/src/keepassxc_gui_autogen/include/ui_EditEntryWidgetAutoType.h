/********************************************************************************
** Form generated from reading UI file 'EditEntryWidgetAutoType.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYWIDGETAUTOTYPE_H
#define UI_EDITENTRYWIDGETAUTOTYPE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "autotype/WindowSelectComboBox.h"

QT_BEGIN_NAMESPACE

class Ui_EditEntryWidgetAutoType
{
public:
    QVBoxLayout *verticalLayout_2;
    QCheckBox *enableButton;
    QSpacerItem *verticalSpacer_2;
    QRadioButton *inheritSequenceButton;
    QRadioButton *customSequenceButton;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QLineEdit *sequenceEdit;
    QToolButton *openHelpButton;
    QSpacerItem *verticalSpacer_4;
    QGroupBox *windowsBox;
    QHBoxLayout *horizontalLayout_5;
    QVBoxLayout *verticalLayout_3;
    QTreeView *assocView;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_3;
    QToolButton *assocAddButton;
    QToolButton *assocRemoveButton;
    QVBoxLayout *verticalLayout;
    QLabel *windowTitleLabel;
    WindowSelectComboBox *windowTitleCombo;
    QSpacerItem *verticalSpacer_3;
    QCheckBox *customWindowSequenceButton;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QLineEdit *windowSequenceEdit;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *EditEntryWidgetAutoType)
    {
        if (EditEntryWidgetAutoType->objectName().isEmpty())
            EditEntryWidgetAutoType->setObjectName(QString::fromUtf8("EditEntryWidgetAutoType"));
        EditEntryWidgetAutoType->resize(577, 434);
        verticalLayout_2 = new QVBoxLayout(EditEntryWidgetAutoType);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        enableButton = new QCheckBox(EditEntryWidgetAutoType);
        enableButton->setObjectName(QString::fromUtf8("enableButton"));

        verticalLayout_2->addWidget(enableButton);

        verticalSpacer_2 = new QSpacerItem(1, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_2->addItem(verticalSpacer_2);

        inheritSequenceButton = new QRadioButton(EditEntryWidgetAutoType);
        inheritSequenceButton->setObjectName(QString::fromUtf8("inheritSequenceButton"));

        verticalLayout_2->addWidget(inheritSequenceButton);

        customSequenceButton = new QRadioButton(EditEntryWidgetAutoType);
        customSequenceButton->setObjectName(QString::fromUtf8("customSequenceButton"));

        verticalLayout_2->addWidget(customSequenceButton);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        sequenceEdit = new QLineEdit(EditEntryWidgetAutoType);
        sequenceEdit->setObjectName(QString::fromUtf8("sequenceEdit"));
        sequenceEdit->setEnabled(false);

        horizontalLayout->addWidget(sequenceEdit);

        openHelpButton = new QToolButton(EditEntryWidgetAutoType);
        openHelpButton->setObjectName(QString::fromUtf8("openHelpButton"));
        openHelpButton->setEnabled(false);

        horizontalLayout->addWidget(openHelpButton);


        verticalLayout_2->addLayout(horizontalLayout);

        verticalSpacer_4 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_2->addItem(verticalSpacer_4);

        windowsBox = new QGroupBox(EditEntryWidgetAutoType);
        windowsBox->setObjectName(QString::fromUtf8("windowsBox"));
        horizontalLayout_5 = new QHBoxLayout(windowsBox);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        assocView = new QTreeView(windowsBox);
        assocView->setObjectName(QString::fromUtf8("assocView"));
        assocView->setRootIsDecorated(false);
        assocView->setUniformRowHeights(true);
        assocView->setSortingEnabled(false);
        assocView->header()->setVisible(false);

        verticalLayout_3->addWidget(assocView);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);

        assocAddButton = new QToolButton(windowsBox);
        assocAddButton->setObjectName(QString::fromUtf8("assocAddButton"));
        assocAddButton->setEnabled(false);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(assocAddButton->sizePolicy().hasHeightForWidth());
        assocAddButton->setSizePolicy(sizePolicy);
        assocAddButton->setMinimumSize(QSize(0, 25));

        horizontalLayout_4->addWidget(assocAddButton);

        assocRemoveButton = new QToolButton(windowsBox);
        assocRemoveButton->setObjectName(QString::fromUtf8("assocRemoveButton"));
        assocRemoveButton->setEnabled(false);
        sizePolicy.setHeightForWidth(assocRemoveButton->sizePolicy().hasHeightForWidth());
        assocRemoveButton->setSizePolicy(sizePolicy);
        assocRemoveButton->setMinimumSize(QSize(0, 25));

        horizontalLayout_4->addWidget(assocRemoveButton);

        horizontalLayout_4->setStretch(0, 2);
        horizontalLayout_4->setStretch(1, 1);
        horizontalLayout_4->setStretch(2, 1);

        verticalLayout_3->addLayout(horizontalLayout_4);


        horizontalLayout_5->addLayout(verticalLayout_3);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        windowTitleLabel = new QLabel(windowsBox);
        windowTitleLabel->setObjectName(QString::fromUtf8("windowTitleLabel"));

        verticalLayout->addWidget(windowTitleLabel);

        windowTitleCombo = new WindowSelectComboBox(windowsBox);
        windowTitleCombo->setObjectName(QString::fromUtf8("windowTitleCombo"));

        verticalLayout->addWidget(windowTitleCombo);

        verticalSpacer_3 = new QSpacerItem(1, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer_3);

        customWindowSequenceButton = new QCheckBox(windowsBox);
        customWindowSequenceButton->setObjectName(QString::fromUtf8("customWindowSequenceButton"));

        verticalLayout->addWidget(customWindowSequenceButton);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_2 = new QSpacerItem(20, 1, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        windowSequenceEdit = new QLineEdit(windowsBox);
        windowSequenceEdit->setObjectName(QString::fromUtf8("windowSequenceEdit"));
        windowSequenceEdit->setEnabled(false);

        horizontalLayout_2->addWidget(windowSequenceEdit);


        verticalLayout->addLayout(horizontalLayout_2);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        horizontalLayout_5->addLayout(verticalLayout);


        verticalLayout_2->addWidget(windowsBox);

        QWidget::setTabOrder(enableButton, inheritSequenceButton);
        QWidget::setTabOrder(inheritSequenceButton, customSequenceButton);
        QWidget::setTabOrder(customSequenceButton, sequenceEdit);
        QWidget::setTabOrder(sequenceEdit, openHelpButton);
        QWidget::setTabOrder(openHelpButton, assocView);
        QWidget::setTabOrder(assocView, windowTitleCombo);
        QWidget::setTabOrder(windowTitleCombo, customWindowSequenceButton);
        QWidget::setTabOrder(customWindowSequenceButton, windowSequenceEdit);
        QWidget::setTabOrder(windowSequenceEdit, assocAddButton);
        QWidget::setTabOrder(assocAddButton, assocRemoveButton);

        retranslateUi(EditEntryWidgetAutoType);

        QMetaObject::connectSlotsByName(EditEntryWidgetAutoType);
    } // setupUi

    void retranslateUi(QWidget *EditEntryWidgetAutoType)
    {
        enableButton->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "Enable Auto-Type for this entry", nullptr));
        inheritSequenceButton->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "Inherit default Auto-Type sequence from the group", nullptr));
        customSequenceButton->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "Use custom Auto-Type sequence:", nullptr));
#if QT_CONFIG(accessibility)
        sequenceEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Custom Auto-Type sequence", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        openHelpButton->setToolTip(QCoreApplication::translate("EditEntryWidgetAutoType", "Open Auto-Type help webpage", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        openHelpButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Open Auto-Type help webpage", nullptr));
#endif // QT_CONFIG(accessibility)
        openHelpButton->setText(QString());
        windowsBox->setTitle(QCoreApplication::translate("EditEntryWidgetAutoType", "Window Associations", nullptr));
#if QT_CONFIG(accessibility)
        assocView->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Existing window associations", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        assocAddButton->setToolTip(QCoreApplication::translate("EditEntryWidgetAutoType", "Add new window association", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        assocAddButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Add new window association", nullptr));
#endif // QT_CONFIG(accessibility)
        assocAddButton->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "+", "Add item"));
#if QT_CONFIG(tooltip)
        assocRemoveButton->setToolTip(QCoreApplication::translate("EditEntryWidgetAutoType", "Remove selected window association", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        assocRemoveButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Remove selected window association", nullptr));
#endif // QT_CONFIG(accessibility)
        assocRemoveButton->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "-", "Remove item"));
        windowTitleLabel->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "Window title:", nullptr));
#if QT_CONFIG(tooltip)
        windowTitleCombo->setToolTip(QCoreApplication::translate("EditEntryWidgetAutoType", "You can use an asterisk (*) to match everything", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        windowTitleCombo->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Set the window association title", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        windowTitleCombo->setAccessibleDescription(QCoreApplication::translate("EditEntryWidgetAutoType", "You can use an asterisk to match everything", nullptr));
#endif // QT_CONFIG(accessibility)
        customWindowSequenceButton->setText(QCoreApplication::translate("EditEntryWidgetAutoType", "Use a specific sequence for this association:", nullptr));
#if QT_CONFIG(accessibility)
        windowSequenceEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAutoType", "Custom Auto-Type sequence for this window", nullptr));
#endif // QT_CONFIG(accessibility)
        (void)EditEntryWidgetAutoType;
    } // retranslateUi

};

namespace Ui {
    class EditEntryWidgetAutoType: public Ui_EditEntryWidgetAutoType {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYWIDGETAUTOTYPE_H
