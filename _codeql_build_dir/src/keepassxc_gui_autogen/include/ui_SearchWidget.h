/********************************************************************************
** Form generated from reading UI file 'SearchWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SEARCHWIDGET_H
#define UI_SEARCHWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SearchWidget
{
public:
    QAction *searchIcon;
    QAction *helpIcon;
    QAction *saveIcon;
    QHBoxLayout *horizontalLayout;
    QLineEdit *searchEdit;

    void setupUi(QWidget *SearchWidget)
    {
        if (SearchWidget->objectName().isEmpty())
            SearchWidget->setObjectName(QString::fromUtf8("SearchWidget"));
        SearchWidget->resize(465, 34);
        searchIcon = new QAction(SearchWidget);
        searchIcon->setObjectName(QString::fromUtf8("searchIcon"));
        helpIcon = new QAction(SearchWidget);
        helpIcon->setObjectName(QString::fromUtf8("helpIcon"));
        saveIcon = new QAction(SearchWidget);
        saveIcon->setObjectName(QString::fromUtf8("saveIcon"));
        horizontalLayout = new QHBoxLayout(SearchWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(3, 0, 7, 0);
        searchEdit = new QLineEdit(SearchWidget);
        searchEdit->setObjectName(QString::fromUtf8("searchEdit"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(searchEdit->sizePolicy().hasHeightForWidth());
        searchEdit->setSizePolicy(sizePolicy);
        searchEdit->setMinimumSize(QSize(150, 0));
        searchEdit->setClearButtonEnabled(true);

        horizontalLayout->addWidget(searchEdit);


        retranslateUi(SearchWidget);

        QMetaObject::connectSlotsByName(SearchWidget);
    } // setupUi

    void retranslateUi(QWidget *SearchWidget)
    {
        searchIcon->setText(QCoreApplication::translate("SearchWidget", "Search", nullptr));
        helpIcon->setText(QCoreApplication::translate("SearchWidget", "Search Help", nullptr));
        saveIcon->setText(QCoreApplication::translate("SearchWidget", "Save Search", nullptr));
        searchEdit->setPlaceholderText(QString());
        (void)SearchWidget;
    } // retranslateUi

};

namespace Ui {
    class SearchWidget: public Ui_SearchWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SEARCHWIDGET_H
