/********************************************************************************
** Form generated from reading UI file 'ReportsWidgetHealthcheck.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPORTSWIDGETHEALTHCHECK_H
#define UI_REPORTSWIDGETHEALTHCHECK_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReportsWidgetHealthcheck
{
public:
    QVBoxLayout *verticalLayout;
    QTableView *healthcheckTableView;
    QCheckBox *showExpired;
    QCheckBox *showExcluded;
    QLabel *tipLabel;

    void setupUi(QWidget *ReportsWidgetHealthcheck)
    {
        if (ReportsWidgetHealthcheck->objectName().isEmpty())
            ReportsWidgetHealthcheck->setObjectName(QString::fromUtf8("ReportsWidgetHealthcheck"));
        ReportsWidgetHealthcheck->resize(505, 379);
        verticalLayout = new QVBoxLayout(ReportsWidgetHealthcheck);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        healthcheckTableView = new QTableView(ReportsWidgetHealthcheck);
        healthcheckTableView->setObjectName(QString::fromUtf8("healthcheckTableView"));
        healthcheckTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        healthcheckTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        healthcheckTableView->setProperty("showDropIndicator", QVariant(false));
        healthcheckTableView->setAlternatingRowColors(true);
        healthcheckTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        healthcheckTableView->setTextElideMode(Qt::ElideMiddle);
        healthcheckTableView->setSortingEnabled(true);
        healthcheckTableView->horizontalHeader()->setStretchLastSection(true);
        healthcheckTableView->verticalHeader()->setVisible(false);

        verticalLayout->addWidget(healthcheckTableView);

        showExpired = new QCheckBox(ReportsWidgetHealthcheck);
        showExpired->setObjectName(QString::fromUtf8("showExpired"));

        verticalLayout->addWidget(showExpired);

        showExcluded = new QCheckBox(ReportsWidgetHealthcheck);
        showExcluded->setObjectName(QString::fromUtf8("showExcluded"));

        verticalLayout->addWidget(showExcluded);

        tipLabel = new QLabel(ReportsWidgetHealthcheck);
        tipLabel->setObjectName(QString::fromUtf8("tipLabel"));
        QFont font;
        font.setItalic(true);
        tipLabel->setFont(font);

        verticalLayout->addWidget(tipLabel);

        QWidget::setTabOrder(healthcheckTableView, showExcluded);

        retranslateUi(ReportsWidgetHealthcheck);

        QMetaObject::connectSlotsByName(ReportsWidgetHealthcheck);
    } // setupUi

    void retranslateUi(QWidget *ReportsWidgetHealthcheck)
    {
        showExpired->setText(QCoreApplication::translate("ReportsWidgetHealthcheck", "Show expired entries", nullptr));
        showExcluded->setText(QCoreApplication::translate("ReportsWidgetHealthcheck", "Show entries that have been excluded from reports", nullptr));
        tipLabel->setText(QCoreApplication::translate("ReportsWidgetHealthcheck", "Hover over reason to show additional details. Double-click entries to edit.", nullptr));
        (void)ReportsWidgetHealthcheck;
    } // retranslateUi

};

namespace Ui {
    class ReportsWidgetHealthcheck: public Ui_ReportsWidgetHealthcheck {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPORTSWIDGETHEALTHCHECK_H
