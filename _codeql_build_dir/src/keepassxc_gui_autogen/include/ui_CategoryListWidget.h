/********************************************************************************
** Form generated from reading UI file 'CategoryListWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CATEGORYLISTWIDGET_H
#define UI_CATEGORYLISTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CategoryListWidget
{
public:
    QVBoxLayout *verticalLayout;
    QToolButton *scrollUp;
    QListWidget *categoryList;
    QToolButton *scrollDown;

    void setupUi(QWidget *CategoryListWidget)
    {
        if (CategoryListWidget->objectName().isEmpty())
            CategoryListWidget->setObjectName(QString::fromUtf8("CategoryListWidget"));
        CategoryListWidget->resize(256, 418);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CategoryListWidget->sizePolicy().hasHeightForWidth());
        CategoryListWidget->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(CategoryListWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        scrollUp = new QToolButton(CategoryListWidget);
        scrollUp->setObjectName(QString::fromUtf8("scrollUp"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(scrollUp->sizePolicy().hasHeightForWidth());
        scrollUp->setSizePolicy(sizePolicy1);
        scrollUp->setMaximumSize(QSize(16777215, 15));
        scrollUp->setText(QString::fromUtf8(""));
        scrollUp->setArrowType(Qt::UpArrow);

        verticalLayout->addWidget(scrollUp);

        categoryList = new QListWidget(CategoryListWidget);
        categoryList->setObjectName(QString::fromUtf8("categoryList"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(categoryList->sizePolicy().hasHeightForWidth());
        categoryList->setSizePolicy(sizePolicy2);
        categoryList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        categoryList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        categoryList->setMovement(QListView::Static);
        categoryList->setFlow(QListView::TopToBottom);
        categoryList->setProperty("isWrapping", QVariant(false));
        categoryList->setViewMode(QListView::IconMode);
        categoryList->setUniformItemSizes(true);
        categoryList->setWordWrap(true);

        verticalLayout->addWidget(categoryList);

        scrollDown = new QToolButton(CategoryListWidget);
        scrollDown->setObjectName(QString::fromUtf8("scrollDown"));
        sizePolicy1.setHeightForWidth(scrollDown->sizePolicy().hasHeightForWidth());
        scrollDown->setSizePolicy(sizePolicy1);
        scrollDown->setMaximumSize(QSize(16777215, 15));
        scrollDown->setText(QString::fromUtf8(""));
        scrollDown->setArrowType(Qt::DownArrow);

        verticalLayout->addWidget(scrollDown);

        QWidget::setTabOrder(scrollUp, categoryList);
        QWidget::setTabOrder(categoryList, scrollDown);

        retranslateUi(CategoryListWidget);

        QMetaObject::connectSlotsByName(CategoryListWidget);
    } // setupUi

    void retranslateUi(QWidget *CategoryListWidget)
    {
        (void)CategoryListWidget;
    } // retranslateUi

};

namespace Ui {
    class CategoryListWidget: public Ui_CategoryListWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CATEGORYLISTWIDGET_H
