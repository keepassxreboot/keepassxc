/********************************************************************************
** Form generated from reading UI file 'EditEntryWidgetMain.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYWIDGETMAIN_H
#define UI_EDITENTRYWIDGETMAIN_H

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
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/PasswordWidget.h"
#include "gui/URLEdit.h"
#include "gui/tag/TagsEdit.h"

QT_BEGIN_NAMESPACE

class Ui_EditEntryWidgetMain
{
public:
    QWidget *container;
    QGridLayout *gridLayout;
    QLineEdit *titleEdit;
    QLabel *usernameLabel;
    PasswordWidget *passwordEdit;
    QVBoxLayout *verticalLayout_2;
    QPlainTextEdit *notesEdit;
    QLabel *titleLabel;
    QLabel *passwordLabel;
    QComboBox *usernameComboBox;
    TagsEdit *tagsList;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *expireCheck;
    QDateTimeEdit *expireDatePicker;
    QPushButton *expirePresets;
    QLabel *urlLabel;
    QHBoxLayout *horizontalLayout_6;
    URLEdit *urlEdit;
    QToolButton *fetchFaviconButton;
    QVBoxLayout *verticalLayout;
    QLabel *notesLabel;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer;
    QToolButton *revealNotesButton;
    QSpacerItem *verticalSpacer;
    QLabel *tagsLabel;
    QLabel *expireLabel;

    void setupUi(QScrollArea *EditEntryWidgetMain)
    {
        if (EditEntryWidgetMain->objectName().isEmpty())
            EditEntryWidgetMain->setObjectName(QString::fromUtf8("EditEntryWidgetMain"));
        EditEntryWidgetMain->resize(400, 523);
        EditEntryWidgetMain->setFrameShape(QFrame::NoFrame);
        EditEntryWidgetMain->setFrameShadow(QFrame::Plain);
        EditEntryWidgetMain->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        EditEntryWidgetMain->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        EditEntryWidgetMain->setWidgetResizable(true);
        container = new QWidget();
        container->setObjectName(QString::fromUtf8("container"));
        container->setGeometry(QRect(0, 0, 400, 523));
        gridLayout = new QGridLayout(container);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(10);
        gridLayout->setVerticalSpacing(8);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        titleEdit = new QLineEdit(container);
        titleEdit->setObjectName(QString::fromUtf8("titleEdit"));

        gridLayout->addWidget(titleEdit, 0, 1, 1, 1);

        usernameLabel = new QLabel(container);
        usernameLabel->setObjectName(QString::fromUtf8("usernameLabel"));
        usernameLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(usernameLabel, 1, 0, 1, 1);

        passwordEdit = new PasswordWidget(container);
        passwordEdit->setObjectName(QString::fromUtf8("passwordEdit"));
        passwordEdit->setFocusPolicy(Qt::StrongFocus);

        gridLayout->addWidget(passwordEdit, 2, 1, 1, 1);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        notesEdit = new QPlainTextEdit(container);
        notesEdit->setObjectName(QString::fromUtf8("notesEdit"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(notesEdit->sizePolicy().hasHeightForWidth());
        notesEdit->setSizePolicy(sizePolicy);
        notesEdit->setMinimumSize(QSize(0, 100));
        notesEdit->setTabStopDistance(10.000000000000000);

        verticalLayout_2->addWidget(notesEdit);


        gridLayout->addLayout(verticalLayout_2, 8, 1, 1, 1);

        titleLabel = new QLabel(container);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(titleLabel, 0, 0, 1, 1);

        passwordLabel = new QLabel(container);
        passwordLabel->setObjectName(QString::fromUtf8("passwordLabel"));
        passwordLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(passwordLabel, 2, 0, 1, 1);

        usernameComboBox = new QComboBox(container);
        usernameComboBox->setObjectName(QString::fromUtf8("usernameComboBox"));
        usernameComboBox->setFocusPolicy(Qt::StrongFocus);

        gridLayout->addWidget(usernameComboBox, 1, 1, 1, 1);

        tagsList = new TagsEdit(container);
        tagsList->setObjectName(QString::fromUtf8("tagsList"));
        tagsList->setFocusPolicy(Qt::StrongFocus);

        gridLayout->addWidget(tagsList, 5, 1, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(8);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        expireCheck = new QCheckBox(container);
        expireCheck->setObjectName(QString::fromUtf8("expireCheck"));

        horizontalLayout_2->addWidget(expireCheck);

        expireDatePicker = new QDateTimeEdit(container);
        expireDatePicker->setObjectName(QString::fromUtf8("expireDatePicker"));
        expireDatePicker->setEnabled(false);
        expireDatePicker->setCalendarPopup(true);

        horizontalLayout_2->addWidget(expireDatePicker);

        expirePresets = new QPushButton(container);
        expirePresets->setObjectName(QString::fromUtf8("expirePresets"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(expirePresets->sizePolicy().hasHeightForWidth());
        expirePresets->setSizePolicy(sizePolicy1);

        horizontalLayout_2->addWidget(expirePresets);

        horizontalLayout_2->setStretch(1, 1);

        gridLayout->addLayout(horizontalLayout_2, 7, 1, 1, 1);

        urlLabel = new QLabel(container);
        urlLabel->setObjectName(QString::fromUtf8("urlLabel"));
        urlLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(urlLabel, 3, 0, 1, 1);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(8);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        urlEdit = new URLEdit(container);
        urlEdit->setObjectName(QString::fromUtf8("urlEdit"));
        urlEdit->setPlaceholderText(QString::fromUtf8("https://example.com"));

        horizontalLayout_6->addWidget(urlEdit);

        fetchFaviconButton = new QToolButton(container);
        fetchFaviconButton->setObjectName(QString::fromUtf8("fetchFaviconButton"));

        horizontalLayout_6->addWidget(fetchFaviconButton);


        gridLayout->addLayout(horizontalLayout_6, 3, 1, 1, 1);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        notesLabel = new QLabel(container);
        notesLabel->setObjectName(QString::fromUtf8("notesLabel"));
        notesLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        verticalLayout->addWidget(notesLabel);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(-1, 6, -1, -1);
        horizontalSpacer = new QSpacerItem(5, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        revealNotesButton = new QToolButton(container);
        revealNotesButton->setObjectName(QString::fromUtf8("revealNotesButton"));
        revealNotesButton->setIconSize(QSize(14, 14));
        revealNotesButton->setCheckable(true);

        horizontalLayout_3->addWidget(revealNotesButton);


        verticalLayout->addLayout(horizontalLayout_3);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        gridLayout->addLayout(verticalLayout, 8, 0, 1, 1);

        tagsLabel = new QLabel(container);
        tagsLabel->setObjectName(QString::fromUtf8("tagsLabel"));
        tagsLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(tagsLabel, 5, 0, 1, 1);

        expireLabel = new QLabel(container);
        expireLabel->setObjectName(QString::fromUtf8("expireLabel"));
        expireLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(expireLabel, 7, 0, 1, 1);

        gridLayout->setColumnStretch(1, 1);
        EditEntryWidgetMain->setWidget(container);
#if QT_CONFIG(shortcut)
        usernameLabel->setBuddy(usernameComboBox);
        titleLabel->setBuddy(titleEdit);
        passwordLabel->setBuddy(passwordEdit);
        urlLabel->setBuddy(urlEdit);
        notesLabel->setBuddy(notesEdit);
        tagsLabel->setBuddy(tagsList);
        expireLabel->setBuddy(expireCheck);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(titleEdit, usernameComboBox);
        QWidget::setTabOrder(usernameComboBox, passwordEdit);
        QWidget::setTabOrder(passwordEdit, urlEdit);
        QWidget::setTabOrder(urlEdit, fetchFaviconButton);
        QWidget::setTabOrder(fetchFaviconButton, tagsList);
        QWidget::setTabOrder(tagsList, expireCheck);
        QWidget::setTabOrder(expireCheck, expireDatePicker);
        QWidget::setTabOrder(expireDatePicker, expirePresets);
        QWidget::setTabOrder(expirePresets, revealNotesButton);
        QWidget::setTabOrder(revealNotesButton, notesEdit);

        retranslateUi(EditEntryWidgetMain);

        QMetaObject::connectSlotsByName(EditEntryWidgetMain);
    } // setupUi

    void retranslateUi(QScrollArea *EditEntryWidgetMain)
    {
        EditEntryWidgetMain->setWindowTitle(QCoreApplication::translate("EditEntryWidgetMain", "Edit Entry", nullptr));
#if QT_CONFIG(accessibility)
        titleEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Title field", nullptr));
#endif // QT_CONFIG(accessibility)
        usernameLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "&Username:", nullptr));
#if QT_CONFIG(accessibility)
        passwordEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Password field", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        notesEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Notes field", nullptr));
#endif // QT_CONFIG(accessibility)
        titleLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "&Title:", nullptr));
        passwordLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "&Password:", nullptr));
#if QT_CONFIG(accessibility)
        usernameComboBox->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Username field", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        tagsList->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Tags list", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        expireCheck->setToolTip(QCoreApplication::translate("EditEntryWidgetMain", "Toggle expiration", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        expireCheck->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Toggle expiration", nullptr));
#endif // QT_CONFIG(accessibility)
        expireCheck->setText(QString());
#if QT_CONFIG(accessibility)
        expireDatePicker->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Expiration field", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        expirePresets->setToolTip(QCoreApplication::translate("EditEntryWidgetMain", "Expiration Presets", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        expirePresets->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Expiration presets", nullptr));
#endif // QT_CONFIG(accessibility)
        expirePresets->setText(QCoreApplication::translate("EditEntryWidgetMain", "Presets", nullptr));
        urlLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "UR&L:", nullptr));
#if QT_CONFIG(accessibility)
        urlEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Url field", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        fetchFaviconButton->setToolTip(QCoreApplication::translate("EditEntryWidgetMain", "Download favicon for URL", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        fetchFaviconButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Download favicon for URL", nullptr));
#endif // QT_CONFIG(accessibility)
        notesLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "&Notes:", nullptr));
#if QT_CONFIG(tooltip)
        revealNotesButton->setToolTip(QCoreApplication::translate("EditEntryWidgetMain", "Toggle notes visibility", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        revealNotesButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetMain", "Toggle notes visibility", nullptr));
#endif // QT_CONFIG(accessibility)
        tagsLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "T&ags:", nullptr));
        expireLabel->setText(QCoreApplication::translate("EditEntryWidgetMain", "&Expires:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EditEntryWidgetMain: public Ui_EditEntryWidgetMain {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYWIDGETMAIN_H
