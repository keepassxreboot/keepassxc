/********************************************************************************
** Form generated from reading UI file 'ReportsWidgetStatistics.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPORTSWIDGETSTATISTICS_H
#define UI_REPORTSWIDGETSTATISTICS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReportsWidgetStatistics
{
public:
    QVBoxLayout *verticalLayout;
    QTableView *statisticsTableView;
    QLabel *tipLabel;

    void setupUi(QWidget *ReportsWidgetStatistics)
    {
        if (ReportsWidgetStatistics->objectName().isEmpty())
            ReportsWidgetStatistics->setObjectName(QString::fromUtf8("ReportsWidgetStatistics"));
        ReportsWidgetStatistics->resize(397, 379);
        verticalLayout = new QVBoxLayout(ReportsWidgetStatistics);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        statisticsTableView = new QTableView(ReportsWidgetStatistics);
        statisticsTableView->setObjectName(QString::fromUtf8("statisticsTableView"));
        statisticsTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        statisticsTableView->setProperty("showDropIndicator", QVariant(false));
        statisticsTableView->setAlternatingRowColors(true);
        statisticsTableView->setTextElideMode(Qt::ElideMiddle);
        statisticsTableView->setSortingEnabled(false);
        statisticsTableView->horizontalHeader()->setVisible(false);
        statisticsTableView->horizontalHeader()->setStretchLastSection(true);
        statisticsTableView->verticalHeader()->setVisible(false);

        verticalLayout->addWidget(statisticsTableView);

        tipLabel = new QLabel(ReportsWidgetStatistics);
        tipLabel->setObjectName(QString::fromUtf8("tipLabel"));
        QFont font;
        font.setItalic(true);
        tipLabel->setFont(font);

        verticalLayout->addWidget(tipLabel);


        retranslateUi(ReportsWidgetStatistics);

        QMetaObject::connectSlotsByName(ReportsWidgetStatistics);
    } // setupUi

    void retranslateUi(QWidget *ReportsWidgetStatistics)
    {
        tipLabel->setText(QCoreApplication::translate("ReportsWidgetStatistics", "Hover over lines with error icons for further information.", nullptr));
        (void)ReportsWidgetStatistics;
    } // retranslateUi

};

namespace Ui {
    class ReportsWidgetStatistics: public Ui_ReportsWidgetStatistics {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPORTSWIDGETSTATISTICS_H
