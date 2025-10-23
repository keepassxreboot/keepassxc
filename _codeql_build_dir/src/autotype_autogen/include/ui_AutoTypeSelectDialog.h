/********************************************************************************
** Form generated from reading UI file 'AutoTypeSelectDialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTOTYPESELECTDIALOG_H
#define UI_AUTOTYPESELECTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "autotype/AutoTypeMatchView.h"

QT_BEGIN_NAMESPACE

class Ui_AutoTypeSelectDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label;
    QSpacerItem *horizontalSpacer_3;
    QToolButton *helpButton;
    AutoTypeMatchView *view;
    QWidget *buttonBox_2;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *searchCheckBox;
    QSpacerItem *horizontalSpacer;
    QLineEdit *search;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_2;
    QToolButton *action;
    QPushButton *cancelButton;

    void setupUi(QDialog *AutoTypeSelectDialog)
    {
        if (AutoTypeSelectDialog->objectName().isEmpty())
            AutoTypeSelectDialog->setObjectName(QString::fromUtf8("AutoTypeSelectDialog"));
        AutoTypeSelectDialog->resize(418, 303);
        verticalLayout = new QVBoxLayout(AutoTypeSelectDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(-1, 6, -1, 6);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        label = new QLabel(AutoTypeSelectDialog);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_3->addWidget(label);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        helpButton = new QToolButton(AutoTypeSelectDialog);
        helpButton->setObjectName(QString::fromUtf8("helpButton"));
        helpButton->setCursor(QCursor(Qt::PointingHandCursor));
        helpButton->setFocusPolicy(Qt::NoFocus);
        helpButton->setStyleSheet(QString::fromUtf8("QToolButton {\n"
"	border: none;\n"
"	background: none;\n"
"}"));
        helpButton->setText(QString::fromUtf8("?"));

        horizontalLayout_3->addWidget(helpButton);


        verticalLayout->addLayout(horizontalLayout_3);

        view = new AutoTypeMatchView(AutoTypeSelectDialog);
        view->setObjectName(QString::fromUtf8("view"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(view->sizePolicy().hasHeightForWidth());
        view->setSizePolicy(sizePolicy);
        view->setMinimumSize(QSize(0, 175));
        view->viewport()->setProperty("cursor", QVariant(QCursor(Qt::PointingHandCursor)));
        view->setTabKeyNavigation(false);
        view->setProperty("showDropIndicator", QVariant(false));
        view->setAlternatingRowColors(true);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSortingEnabled(true);
        view->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
        view->horizontalHeader()->setStretchLastSection(true);
        view->verticalHeader()->setVisible(false);

        verticalLayout->addWidget(view);

        buttonBox_2 = new QWidget(AutoTypeSelectDialog);
        buttonBox_2->setObjectName(QString::fromUtf8("buttonBox_2"));
        horizontalLayout_2 = new QHBoxLayout(buttonBox_2);
        horizontalLayout_2->setSpacing(10);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        searchCheckBox = new QCheckBox(buttonBox_2);
        searchCheckBox->setObjectName(QString::fromUtf8("searchCheckBox"));

        horizontalLayout_2->addWidget(searchCheckBox);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        verticalLayout->addWidget(buttonBox_2);

        search = new QLineEdit(AutoTypeSelectDialog);
        search->setObjectName(QString::fromUtf8("search"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(search->sizePolicy().hasHeightForWidth());
        search->setSizePolicy(sizePolicy1);
        search->setMinimumSize(QSize(400, 0));
        search->setClearButtonEnabled(true);

        verticalLayout->addWidget(search);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        action = new QToolButton(AutoTypeSelectDialog);
        action->setObjectName(QString::fromUtf8("action"));
        action->setPopupMode(QToolButton::MenuButtonPopup);

        horizontalLayout->addWidget(action);

        cancelButton = new QPushButton(AutoTypeSelectDialog);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));
        cancelButton->setAutoDefault(false);

        horizontalLayout->addWidget(cancelButton);


        verticalLayout->addLayout(horizontalLayout);

        QWidget::setTabOrder(view, searchCheckBox);
        QWidget::setTabOrder(searchCheckBox, search);

        retranslateUi(AutoTypeSelectDialog);

        QMetaObject::connectSlotsByName(AutoTypeSelectDialog);
    } // setupUi

    void retranslateUi(QDialog *AutoTypeSelectDialog)
    {
        AutoTypeSelectDialog->setWindowTitle(QCoreApplication::translate("AutoTypeSelectDialog", "Auto-Type - KeePassXC", nullptr));
        label->setText(QCoreApplication::translate("AutoTypeSelectDialog", "Double click a row to perform Auto-Type or find an entry using the search:", nullptr));
#if QT_CONFIG(tooltip)
        helpButton->setToolTip(QCoreApplication::translate("AutoTypeSelectDialog", "<p>The following shortcuts are available:<br/>\n"
"Ctrl+F - Focus search<br/>\n"
"Ctrl+1 - Type username<br/>\n"
"Ctrl+2 - Type password<br/>\n"
"Ctrl+3 - Type TOTP<br/>\n"
"Ctrl+4 - Type URL<br/>\n"
"Ctrl+5 - Use Virtual Keyboard (Windows Only)<br/>\n"
"Ctrl+Shift+1 - Copy username<br/>\n"
"Ctrl+Shift+2 - Copy password<br/>\n"
"Ctrl+Shift+3 - Copy TOTP<br/>\n"
"Ctrl+Shift+4 - Copy URL<br/>\n"
"</p>", nullptr));
#endif // QT_CONFIG(tooltip)
        searchCheckBox->setText(QCoreApplication::translate("AutoTypeSelectDialog", "Search all open databases", nullptr));
#if QT_CONFIG(tooltip)
        search->setToolTip(QCoreApplication::translate("AutoTypeSelectDialog", "You can use advanced search queries to find any entry in your open databases.", nullptr));
#endif // QT_CONFIG(tooltip)
        search->setPlaceholderText(QCoreApplication::translate("AutoTypeSelectDialog", "Search\342\200\246", nullptr));
        action->setText(QCoreApplication::translate("AutoTypeSelectDialog", "Type Sequence", nullptr));
        cancelButton->setText(QCoreApplication::translate("AutoTypeSelectDialog", "Cancel", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AutoTypeSelectDialog: public Ui_AutoTypeSelectDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTOTYPESELECTDIALOG_H
