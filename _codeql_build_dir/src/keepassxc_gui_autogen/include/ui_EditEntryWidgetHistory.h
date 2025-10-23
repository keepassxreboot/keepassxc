/********************************************************************************
** Form generated from reading UI file 'EditEntryWidgetHistory.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYWIDGETHISTORY_H
#define UI_EDITENTRYWIDGETHISTORY_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditEntryWidgetHistory
{
public:
    QVBoxLayout *verticalLayout;
    QTreeView *historyView;
    QHBoxLayout *horizontalLayout;
    QPushButton *showButton;
    QPushButton *restoreButton;
    QPushButton *deleteButton;
    QPushButton *deleteAllButton;

    void setupUi(QWidget *EditEntryWidgetHistory)
    {
        if (EditEntryWidgetHistory->objectName().isEmpty())
            EditEntryWidgetHistory->setObjectName(QString::fromUtf8("EditEntryWidgetHistory"));
        EditEntryWidgetHistory->resize(400, 300);
        verticalLayout = new QVBoxLayout(EditEntryWidgetHistory);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        historyView = new QTreeView(EditEntryWidgetHistory);
        historyView->setObjectName(QString::fromUtf8("historyView"));
        historyView->setAlternatingRowColors(true);
        historyView->setSortingEnabled(true);
        historyView->header()->setDefaultSectionSize(160);

        verticalLayout->addWidget(historyView);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        showButton = new QPushButton(EditEntryWidgetHistory);
        showButton->setObjectName(QString::fromUtf8("showButton"));
        showButton->setEnabled(false);

        horizontalLayout->addWidget(showButton);

        restoreButton = new QPushButton(EditEntryWidgetHistory);
        restoreButton->setObjectName(QString::fromUtf8("restoreButton"));
        restoreButton->setEnabled(false);

        horizontalLayout->addWidget(restoreButton);

        deleteButton = new QPushButton(EditEntryWidgetHistory);
        deleteButton->setObjectName(QString::fromUtf8("deleteButton"));
        deleteButton->setEnabled(false);

        horizontalLayout->addWidget(deleteButton);

        deleteAllButton = new QPushButton(EditEntryWidgetHistory);
        deleteAllButton->setObjectName(QString::fromUtf8("deleteAllButton"));
        deleteAllButton->setEnabled(false);

        horizontalLayout->addWidget(deleteAllButton);


        verticalLayout->addLayout(horizontalLayout);

        QWidget::setTabOrder(historyView, showButton);
        QWidget::setTabOrder(showButton, restoreButton);
        QWidget::setTabOrder(restoreButton, deleteButton);
        QWidget::setTabOrder(deleteButton, deleteAllButton);

        retranslateUi(EditEntryWidgetHistory);

        QMetaObject::connectSlotsByName(EditEntryWidgetHistory);
    } // setupUi

    void retranslateUi(QWidget *EditEntryWidgetHistory)
    {
#if QT_CONFIG(accessibility)
        historyView->setAccessibleName(QCoreApplication::translate("EditEntryWidgetHistory", "Entry history selection", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        showButton->setToolTip(QCoreApplication::translate("EditEntryWidgetHistory", "Show entry at selected history state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        showButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetHistory", "Show entry at selected history state", nullptr));
#endif // QT_CONFIG(accessibility)
        showButton->setText(QCoreApplication::translate("EditEntryWidgetHistory", "Show", nullptr));
#if QT_CONFIG(tooltip)
        restoreButton->setToolTip(QCoreApplication::translate("EditEntryWidgetHistory", "Restore entry to selected history state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        restoreButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetHistory", "Restore entry to selected history state", nullptr));
#endif // QT_CONFIG(accessibility)
        restoreButton->setText(QCoreApplication::translate("EditEntryWidgetHistory", "Restore", nullptr));
#if QT_CONFIG(tooltip)
        deleteButton->setToolTip(QCoreApplication::translate("EditEntryWidgetHistory", "Delete selected history state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        deleteButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetHistory", "Delete selected history state", nullptr));
#endif // QT_CONFIG(accessibility)
        deleteButton->setText(QCoreApplication::translate("EditEntryWidgetHistory", "Delete", nullptr));
#if QT_CONFIG(tooltip)
        deleteAllButton->setToolTip(QCoreApplication::translate("EditEntryWidgetHistory", "Delete all history", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        deleteAllButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetHistory", "Delete all history", nullptr));
#endif // QT_CONFIG(accessibility)
        deleteAllButton->setText(QCoreApplication::translate("EditEntryWidgetHistory", "Delete all", nullptr));
        (void)EditEntryWidgetHistory;
    } // retranslateUi

};

namespace Ui {
    class EditEntryWidgetHistory: public Ui_EditEntryWidgetHistory {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYWIDGETHISTORY_H
