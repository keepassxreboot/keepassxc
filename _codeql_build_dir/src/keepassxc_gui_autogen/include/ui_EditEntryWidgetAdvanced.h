/********************************************************************************
** Form generated from reading UI file 'EditEntryWidgetAdvanced.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYWIDGETADVANCED_H
#define UI_EDITENTRYWIDGETADVANCED_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/entry/EditEntryWidget_p.h"
#include "gui/entry/EntryAttachmentsWidget.h"

QT_BEGIN_NAMESPACE

class Ui_EditEntryWidgetAdvanced
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *attributesBox;
    QHBoxLayout *horizontalLayout_3;
    QSplitter *attributesSplitter;
    AttributesListView *attributesView;
    QPlainTextEdit *attributesEdit;
    QVBoxLayout *attributesButtonLayout;
    QPushButton *addAttributeButton;
    QPushButton *removeAttributeButton;
    QPushButton *editAttributeButton;
    QSpacerItem *verticalSpacer;
    QCheckBox *protectAttributeButton;
    QPushButton *revealAttributeButton;
    QGroupBox *attachmentsBox;
    QHBoxLayout *horizontalLayout_2;
    EntryAttachmentsWidget *attachmentsWidget;
    QCheckBox *excludeReportsCheckBox;
    QWidget *colorsBox;
    QHBoxLayout *horizontalLayout_4;
    QCheckBox *fgColorCheckBox;
    QPushButton *fgColorButton;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *bgColorCheckBox;
    QPushButton *bgColorButton;
    QSpacerItem *horizontalSpacer;

    void setupUi(QWidget *EditEntryWidgetAdvanced)
    {
        if (EditEntryWidgetAdvanced->objectName().isEmpty())
            EditEntryWidgetAdvanced->setObjectName(QString::fromUtf8("EditEntryWidgetAdvanced"));
        EditEntryWidgetAdvanced->resize(532, 469);
        verticalLayout = new QVBoxLayout(EditEntryWidgetAdvanced);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        attributesBox = new QGroupBox(EditEntryWidgetAdvanced);
        attributesBox->setObjectName(QString::fromUtf8("attributesBox"));
        horizontalLayout_3 = new QHBoxLayout(attributesBox);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        attributesSplitter = new QSplitter(attributesBox);
        attributesSplitter->setObjectName(QString::fromUtf8("attributesSplitter"));
        attributesSplitter->setOrientation(Qt::Horizontal);
        attributesSplitter->setChildrenCollapsible(false);
        attributesView = new AttributesListView(attributesSplitter);
        attributesView->setObjectName(QString::fromUtf8("attributesView"));
        attributesView->setMinimumSize(QSize(0, 0));
        attributesView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        attributesView->setResizeMode(QListView::Adjust);
        attributesSplitter->addWidget(attributesView);
        attributesEdit = new QPlainTextEdit(attributesSplitter);
        attributesEdit->setObjectName(QString::fromUtf8("attributesEdit"));
        attributesEdit->setEnabled(false);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(attributesEdit->sizePolicy().hasHeightForWidth());
        attributesEdit->setSizePolicy(sizePolicy);
        attributesEdit->setMinimumSize(QSize(170, 0));
        attributesSplitter->addWidget(attributesEdit);

        horizontalLayout_3->addWidget(attributesSplitter);

        attributesButtonLayout = new QVBoxLayout();
        attributesButtonLayout->setObjectName(QString::fromUtf8("attributesButtonLayout"));
        addAttributeButton = new QPushButton(attributesBox);
        addAttributeButton->setObjectName(QString::fromUtf8("addAttributeButton"));

        attributesButtonLayout->addWidget(addAttributeButton);

        removeAttributeButton = new QPushButton(attributesBox);
        removeAttributeButton->setObjectName(QString::fromUtf8("removeAttributeButton"));
        removeAttributeButton->setEnabled(false);

        attributesButtonLayout->addWidget(removeAttributeButton);

        editAttributeButton = new QPushButton(attributesBox);
        editAttributeButton->setObjectName(QString::fromUtf8("editAttributeButton"));
        editAttributeButton->setEnabled(false);

        attributesButtonLayout->addWidget(editAttributeButton);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        attributesButtonLayout->addItem(verticalSpacer);

        protectAttributeButton = new QCheckBox(attributesBox);
        protectAttributeButton->setObjectName(QString::fromUtf8("protectAttributeButton"));
        protectAttributeButton->setEnabled(true);
        protectAttributeButton->setStyleSheet(QString::fromUtf8("margin-left:50%;margin-right:50%"));
        protectAttributeButton->setCheckable(true);

        attributesButtonLayout->addWidget(protectAttributeButton);

        revealAttributeButton = new QPushButton(attributesBox);
        revealAttributeButton->setObjectName(QString::fromUtf8("revealAttributeButton"));
        revealAttributeButton->setEnabled(false);
        revealAttributeButton->setCheckable(false);

        attributesButtonLayout->addWidget(revealAttributeButton);


        horizontalLayout_3->addLayout(attributesButtonLayout);


        verticalLayout->addWidget(attributesBox);

        attachmentsBox = new QGroupBox(EditEntryWidgetAdvanced);
        attachmentsBox->setObjectName(QString::fromUtf8("attachmentsBox"));
        horizontalLayout_2 = new QHBoxLayout(attachmentsBox);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        attachmentsWidget = new EntryAttachmentsWidget(attachmentsBox);
        attachmentsWidget->setObjectName(QString::fromUtf8("attachmentsWidget"));

        horizontalLayout_2->addWidget(attachmentsWidget);


        verticalLayout->addWidget(attachmentsBox);

        excludeReportsCheckBox = new QCheckBox(EditEntryWidgetAdvanced);
        excludeReportsCheckBox->setObjectName(QString::fromUtf8("excludeReportsCheckBox"));

        verticalLayout->addWidget(excludeReportsCheckBox);

        colorsBox = new QWidget(EditEntryWidgetAdvanced);
        colorsBox->setObjectName(QString::fromUtf8("colorsBox"));
        horizontalLayout_4 = new QHBoxLayout(colorsBox);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, 0, 0);
        fgColorCheckBox = new QCheckBox(colorsBox);
        fgColorCheckBox->setObjectName(QString::fromUtf8("fgColorCheckBox"));

        horizontalLayout_4->addWidget(fgColorCheckBox);

        fgColorButton = new QPushButton(colorsBox);
        fgColorButton->setObjectName(QString::fromUtf8("fgColorButton"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(fgColorButton->sizePolicy().hasHeightForWidth());
        fgColorButton->setSizePolicy(sizePolicy1);
        fgColorButton->setMaximumSize(QSize(25, 25));

        horizontalLayout_4->addWidget(fgColorButton);

        horizontalSpacer_2 = new QSpacerItem(30, 20, QSizePolicy::Maximum, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_2);

        bgColorCheckBox = new QCheckBox(colorsBox);
        bgColorCheckBox->setObjectName(QString::fromUtf8("bgColorCheckBox"));

        horizontalLayout_4->addWidget(bgColorCheckBox);

        bgColorButton = new QPushButton(colorsBox);
        bgColorButton->setObjectName(QString::fromUtf8("bgColorButton"));
        sizePolicy1.setHeightForWidth(bgColorButton->sizePolicy().hasHeightForWidth());
        bgColorButton->setSizePolicy(sizePolicy1);
        bgColorButton->setMaximumSize(QSize(25, 25));
        bgColorButton->setFlat(false);

        horizontalLayout_4->addWidget(bgColorButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);


        verticalLayout->addWidget(colorsBox);

        QWidget::setTabOrder(attributesView, attributesEdit);
        QWidget::setTabOrder(attributesEdit, addAttributeButton);
        QWidget::setTabOrder(addAttributeButton, removeAttributeButton);
        QWidget::setTabOrder(removeAttributeButton, editAttributeButton);
        QWidget::setTabOrder(editAttributeButton, protectAttributeButton);
        QWidget::setTabOrder(protectAttributeButton, revealAttributeButton);
        QWidget::setTabOrder(revealAttributeButton, excludeReportsCheckBox);
        QWidget::setTabOrder(excludeReportsCheckBox, fgColorCheckBox);
        QWidget::setTabOrder(fgColorCheckBox, fgColorButton);
        QWidget::setTabOrder(fgColorButton, bgColorCheckBox);
        QWidget::setTabOrder(bgColorCheckBox, bgColorButton);

        retranslateUi(EditEntryWidgetAdvanced);

        QMetaObject::connectSlotsByName(EditEntryWidgetAdvanced);
    } // setupUi

    void retranslateUi(QWidget *EditEntryWidgetAdvanced)
    {
        attributesBox->setTitle(QCoreApplication::translate("EditEntryWidgetAdvanced", "Additional attributes", nullptr));
#if QT_CONFIG(accessibility)
        attributesView->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Attribute selection", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        attributesEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Attribute value", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        addAttributeButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Add a new attribute", nullptr));
#endif // QT_CONFIG(accessibility)
        addAttributeButton->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Add", nullptr));
#if QT_CONFIG(accessibility)
        removeAttributeButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Remove selected attribute", nullptr));
#endif // QT_CONFIG(accessibility)
        removeAttributeButton->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Remove", nullptr));
#if QT_CONFIG(accessibility)
        editAttributeButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Edit attribute name", nullptr));
#endif // QT_CONFIG(accessibility)
        editAttributeButton->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Edit Name", nullptr));
#if QT_CONFIG(accessibility)
        protectAttributeButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Toggle attribute protection", nullptr));
#endif // QT_CONFIG(accessibility)
        protectAttributeButton->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Protect", nullptr));
#if QT_CONFIG(accessibility)
        revealAttributeButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Show a protected attribute", nullptr));
#endif // QT_CONFIG(accessibility)
        revealAttributeButton->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Reveal", nullptr));
        attachmentsBox->setTitle(QCoreApplication::translate("EditEntryWidgetAdvanced", "Attachments", nullptr));
#if QT_CONFIG(tooltip)
        excludeReportsCheckBox->setToolTip(QCoreApplication::translate("EditEntryWidgetAdvanced", "If checked, the entry will not appear in reports like Health Check and HIBP even if it doesn't match the quality requirements.", nullptr));
#endif // QT_CONFIG(tooltip)
        excludeReportsCheckBox->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Exclude from database reports", nullptr));
        fgColorCheckBox->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Foreground Color:", nullptr));
#if QT_CONFIG(accessibility)
        fgColorButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Foreground color selection", nullptr));
#endif // QT_CONFIG(accessibility)
        fgColorButton->setText(QString());
        bgColorCheckBox->setText(QCoreApplication::translate("EditEntryWidgetAdvanced", "Background Color:", nullptr));
#if QT_CONFIG(accessibility)
        bgColorButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetAdvanced", "Background color selection", nullptr));
#endif // QT_CONFIG(accessibility)
        bgColorButton->setText(QString());
        (void)EditEntryWidgetAdvanced;
    } // retranslateUi

};

namespace Ui {
    class EditEntryWidgetAdvanced: public Ui_EditEntryWidgetAdvanced {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYWIDGETADVANCED_H
