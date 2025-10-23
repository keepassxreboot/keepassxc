/********************************************************************************
** Form generated from reading UI file 'EditGroupWidgetMain.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITGROUPWIDGETMAIN_H
#define UI_EDITGROUPWIDGETMAIN_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateTimeEdit>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditGroupWidgetMain
{
public:
    QWidget *container;
    QGridLayout *gridLayout;
    QCheckBox *expireCheck;
    QLineEdit *editName;
    QDateTimeEdit *expireDatePicker;
    QRadioButton *autoTypeSequenceInherit;
    QLabel *autotypeLabel;
    QLabel *searchLabel;
    QComboBox *autotypeComboBox;
    QVBoxLayout *verticalLayout;
    QLabel *labelNotes;
    QSpacerItem *verticalSpacer_2;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QLineEdit *autoTypeSequenceCustomEdit;
    QPlainTextEdit *editNotes;
    QLabel *labelName;
    QRadioButton *autoTypeSequenceCustomRadio;
    QComboBox *searchComboBox;
    QSpacerItem *verticalSpacer_4;

    void setupUi(QScrollArea *EditGroupWidgetMain)
    {
        if (EditGroupWidgetMain->objectName().isEmpty())
            EditGroupWidgetMain->setObjectName(QString::fromUtf8("EditGroupWidgetMain"));
        EditGroupWidgetMain->resize(539, 523);
        EditGroupWidgetMain->setFrameShape(QFrame::NoFrame);
        EditGroupWidgetMain->setFrameShadow(QFrame::Plain);
        EditGroupWidgetMain->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        EditGroupWidgetMain->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        EditGroupWidgetMain->setWidgetResizable(true);
        container = new QWidget();
        container->setObjectName(QString::fromUtf8("container"));
        container->setGeometry(QRect(0, 0, 539, 523));
        gridLayout = new QGridLayout(container);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(8);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        expireCheck = new QCheckBox(container);
        expireCheck->setObjectName(QString::fromUtf8("expireCheck"));

        gridLayout->addWidget(expireCheck, 3, 0, 1, 1);

        editName = new QLineEdit(container);
        editName->setObjectName(QString::fromUtf8("editName"));

        gridLayout->addWidget(editName, 0, 1, 1, 1);

        expireDatePicker = new QDateTimeEdit(container);
        expireDatePicker->setObjectName(QString::fromUtf8("expireDatePicker"));
        expireDatePicker->setEnabled(false);
        expireDatePicker->setCalendarPopup(true);

        gridLayout->addWidget(expireDatePicker, 3, 1, 1, 1);

        autoTypeSequenceInherit = new QRadioButton(container);
        autoTypeSequenceInherit->setObjectName(QString::fromUtf8("autoTypeSequenceInherit"));

        gridLayout->addWidget(autoTypeSequenceInherit, 6, 1, 1, 1);

        autotypeLabel = new QLabel(container);
        autotypeLabel->setObjectName(QString::fromUtf8("autotypeLabel"));
        autotypeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(autotypeLabel, 5, 0, 1, 1);

        searchLabel = new QLabel(container);
        searchLabel->setObjectName(QString::fromUtf8("searchLabel"));
        searchLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(searchLabel, 4, 0, 1, 1);

        autotypeComboBox = new QComboBox(container);
        autotypeComboBox->setObjectName(QString::fromUtf8("autotypeComboBox"));

        gridLayout->addWidget(autotypeComboBox, 5, 1, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        labelNotes = new QLabel(container);
        labelNotes->setObjectName(QString::fromUtf8("labelNotes"));
        labelNotes->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        verticalLayout->addWidget(labelNotes);

        verticalSpacer_2 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_2);


        gridLayout->addLayout(verticalLayout, 1, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_2 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        autoTypeSequenceCustomEdit = new QLineEdit(container);
        autoTypeSequenceCustomEdit->setObjectName(QString::fromUtf8("autoTypeSequenceCustomEdit"));
        autoTypeSequenceCustomEdit->setEnabled(false);

        horizontalLayout_2->addWidget(autoTypeSequenceCustomEdit);


        gridLayout->addLayout(horizontalLayout_2, 8, 1, 1, 1);

        editNotes = new QPlainTextEdit(container);
        editNotes->setObjectName(QString::fromUtf8("editNotes"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(editNotes->sizePolicy().hasHeightForWidth());
        editNotes->setSizePolicy(sizePolicy);
        editNotes->setMaximumSize(QSize(16777215, 120));

        gridLayout->addWidget(editNotes, 1, 1, 1, 1);

        labelName = new QLabel(container);
        labelName->setObjectName(QString::fromUtf8("labelName"));
        labelName->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(labelName, 0, 0, 1, 1);

        autoTypeSequenceCustomRadio = new QRadioButton(container);
        autoTypeSequenceCustomRadio->setObjectName(QString::fromUtf8("autoTypeSequenceCustomRadio"));

        gridLayout->addWidget(autoTypeSequenceCustomRadio, 7, 1, 1, 1);

        searchComboBox = new QComboBox(container);
        searchComboBox->setObjectName(QString::fromUtf8("searchComboBox"));

        gridLayout->addWidget(searchComboBox, 4, 1, 1, 1);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_4, 9, 0, 1, 1);

        gridLayout->setRowStretch(9, 1);
        gridLayout->setRowMinimumHeight(9, 1);
        EditGroupWidgetMain->setWidget(container);
        QWidget::setTabOrder(editName, editNotes);
        QWidget::setTabOrder(editNotes, expireCheck);
        QWidget::setTabOrder(expireCheck, expireDatePicker);
        QWidget::setTabOrder(expireDatePicker, searchComboBox);
        QWidget::setTabOrder(searchComboBox, autotypeComboBox);
        QWidget::setTabOrder(autotypeComboBox, autoTypeSequenceInherit);
        QWidget::setTabOrder(autoTypeSequenceInherit, autoTypeSequenceCustomRadio);
        QWidget::setTabOrder(autoTypeSequenceCustomRadio, autoTypeSequenceCustomEdit);

        retranslateUi(EditGroupWidgetMain);

        QMetaObject::connectSlotsByName(EditGroupWidgetMain);
    } // setupUi

    void retranslateUi(QScrollArea *EditGroupWidgetMain)
    {
        EditGroupWidgetMain->setWindowTitle(QCoreApplication::translate("EditGroupWidgetMain", "Edit Group", nullptr));
#if QT_CONFIG(accessibility)
        expireCheck->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Toggle expiration", nullptr));
#endif // QT_CONFIG(accessibility)
        expireCheck->setText(QCoreApplication::translate("EditGroupWidgetMain", "Expires:", nullptr));
#if QT_CONFIG(accessibility)
        editName->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Name field", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        expireDatePicker->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Expiration field", nullptr));
#endif // QT_CONFIG(accessibility)
        autoTypeSequenceInherit->setText(QCoreApplication::translate("EditGroupWidgetMain", "Use default Auto-Type sequence of parent group", nullptr));
        autotypeLabel->setText(QCoreApplication::translate("EditGroupWidgetMain", "Auto-Type:", nullptr));
        searchLabel->setText(QCoreApplication::translate("EditGroupWidgetMain", "Search:", nullptr));
#if QT_CONFIG(accessibility)
        autotypeComboBox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Auto-Type toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
        labelNotes->setText(QCoreApplication::translate("EditGroupWidgetMain", "Notes:", nullptr));
#if QT_CONFIG(accessibility)
        autoTypeSequenceCustomEdit->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Default auto-type sequence field", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        autoTypeSequenceCustomEdit->setAccessibleDescription(QString());
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        editNotes->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Notes field", nullptr));
#endif // QT_CONFIG(accessibility)
        labelName->setText(QCoreApplication::translate("EditGroupWidgetMain", "Name:", nullptr));
        autoTypeSequenceCustomRadio->setText(QCoreApplication::translate("EditGroupWidgetMain", "Set default Auto-Type sequence", nullptr));
#if QT_CONFIG(accessibility)
        searchComboBox->setAccessibleName(QCoreApplication::translate("EditGroupWidgetMain", "Search toggle for this and sub groups", nullptr));
#endif // QT_CONFIG(accessibility)
    } // retranslateUi

};

namespace Ui {
    class EditGroupWidgetMain: public Ui_EditGroupWidgetMain {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITGROUPWIDGETMAIN_H
