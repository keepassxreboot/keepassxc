/********************************************************************************
** Form generated from reading UI file 'EditWidgetIcons.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITWIDGETICONS_H
#define UI_EDITWIDGETICONS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditWidgetIcons
{
public:
    QVBoxLayout *verticalLayout;
    QRadioButton *defaultIconsRadio;
    QListView *defaultIconsView;
    QSpacerItem *verticalSpacer;
    QRadioButton *customIconsRadio;
    QListView *customIconsView;
    QHBoxLayout *customIconButtonsHorizontalLayout;
    QPushButton *addButton;
    QSpacerItem *horizontalSpacer_2;
    QLineEdit *faviconURL;
    QPushButton *faviconButton;
    QHBoxLayout *applyToChildrenHorizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *applyIconToPushButton;

    void setupUi(QWidget *EditWidgetIcons)
    {
        if (EditWidgetIcons->objectName().isEmpty())
            EditWidgetIcons->setObjectName(QString::fromUtf8("EditWidgetIcons"));
        EditWidgetIcons->resize(437, 316);
        verticalLayout = new QVBoxLayout(EditWidgetIcons);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        defaultIconsRadio = new QRadioButton(EditWidgetIcons);
        defaultIconsRadio->setObjectName(QString::fromUtf8("defaultIconsRadio"));

        verticalLayout->addWidget(defaultIconsRadio);

        defaultIconsView = new QListView(EditWidgetIcons);
        defaultIconsView->setObjectName(QString::fromUtf8("defaultIconsView"));
        defaultIconsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        defaultIconsView->setMovement(QListView::Static);
        defaultIconsView->setFlow(QListView::LeftToRight);
        defaultIconsView->setProperty("isWrapping", QVariant(true));
        defaultIconsView->setResizeMode(QListView::Adjust);
        defaultIconsView->setSpacing(4);
        defaultIconsView->setViewMode(QListView::ListMode);

        verticalLayout->addWidget(defaultIconsView);

        verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer);

        customIconsRadio = new QRadioButton(EditWidgetIcons);
        customIconsRadio->setObjectName(QString::fromUtf8("customIconsRadio"));

        verticalLayout->addWidget(customIconsRadio);

        customIconsView = new QListView(EditWidgetIcons);
        customIconsView->setObjectName(QString::fromUtf8("customIconsView"));
        customIconsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        customIconsView->setMovement(QListView::Static);
        customIconsView->setFlow(QListView::LeftToRight);
        customIconsView->setProperty("isWrapping", QVariant(true));
        customIconsView->setResizeMode(QListView::Adjust);
        customIconsView->setSpacing(4);
        customIconsView->setViewMode(QListView::ListMode);

        verticalLayout->addWidget(customIconsView);

        customIconButtonsHorizontalLayout = new QHBoxLayout();
        customIconButtonsHorizontalLayout->setObjectName(QString::fromUtf8("customIconButtonsHorizontalLayout"));
        addButton = new QPushButton(EditWidgetIcons);
        addButton->setObjectName(QString::fromUtf8("addButton"));

        customIconButtonsHorizontalLayout->addWidget(addButton);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        customIconButtonsHorizontalLayout->addItem(horizontalSpacer_2);

        faviconURL = new QLineEdit(EditWidgetIcons);
        faviconURL->setObjectName(QString::fromUtf8("faviconURL"));
        faviconURL->setMinimumSize(QSize(200, 0));

        customIconButtonsHorizontalLayout->addWidget(faviconURL);

        faviconButton = new QPushButton(EditWidgetIcons);
        faviconButton->setObjectName(QString::fromUtf8("faviconButton"));

        customIconButtonsHorizontalLayout->addWidget(faviconButton);


        verticalLayout->addLayout(customIconButtonsHorizontalLayout);

        applyToChildrenHorizontalLayout = new QHBoxLayout();
        applyToChildrenHorizontalLayout->setObjectName(QString::fromUtf8("applyToChildrenHorizontalLayout"));
        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        applyToChildrenHorizontalLayout->addItem(horizontalSpacer);

        applyIconToPushButton = new QPushButton(EditWidgetIcons);
        applyIconToPushButton->setObjectName(QString::fromUtf8("applyIconToPushButton"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(applyIconToPushButton->sizePolicy().hasHeightForWidth());
        applyIconToPushButton->setSizePolicy(sizePolicy);
        applyIconToPushButton->setStyleSheet(QString::fromUtf8("padding: 4px 10px"));

        applyToChildrenHorizontalLayout->addWidget(applyIconToPushButton);


        verticalLayout->addLayout(applyToChildrenHorizontalLayout);

        QWidget::setTabOrder(defaultIconsRadio, defaultIconsView);
        QWidget::setTabOrder(defaultIconsView, customIconsRadio);
        QWidget::setTabOrder(customIconsRadio, customIconsView);
        QWidget::setTabOrder(customIconsView, addButton);
        QWidget::setTabOrder(addButton, faviconURL);
        QWidget::setTabOrder(faviconURL, faviconButton);
        QWidget::setTabOrder(faviconButton, applyIconToPushButton);

        retranslateUi(EditWidgetIcons);

        QMetaObject::connectSlotsByName(EditWidgetIcons);
    } // setupUi

    void retranslateUi(QWidget *EditWidgetIcons)
    {
        defaultIconsRadio->setText(QCoreApplication::translate("EditWidgetIcons", "Use default icon", nullptr));
        customIconsRadio->setText(QCoreApplication::translate("EditWidgetIcons", "Use custom icon", nullptr));
        addButton->setText(QCoreApplication::translate("EditWidgetIcons", "Choose icon\342\200\246", nullptr));
#if QT_CONFIG(tooltip)
        faviconURL->setToolTip(QCoreApplication::translate("EditWidgetIcons", "Set the URL to use to search for a favicon", nullptr));
#endif // QT_CONFIG(tooltip)
        faviconURL->setPlaceholderText(QCoreApplication::translate("EditWidgetIcons", "Favicon URL", nullptr));
#if QT_CONFIG(tooltip)
        faviconButton->setToolTip(QCoreApplication::translate("EditWidgetIcons", "Download favicon for URL", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        faviconButton->setAccessibleName(QCoreApplication::translate("EditWidgetIcons", "Download favicon for URL", nullptr));
#endif // QT_CONFIG(accessibility)
        faviconButton->setText(QCoreApplication::translate("EditWidgetIcons", "Download favicon", nullptr));
#if QT_CONFIG(tooltip)
        applyIconToPushButton->setToolTip(QCoreApplication::translate("EditWidgetIcons", "Apply selected icon to subgroups and entries", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        applyIconToPushButton->setAccessibleName(QCoreApplication::translate("EditWidgetIcons", "Apply selected icon to subgroups and entries", nullptr));
#endif // QT_CONFIG(accessibility)
        applyIconToPushButton->setText(QCoreApplication::translate("EditWidgetIcons", "Apply icon to\342\200\246", nullptr));
        (void)EditWidgetIcons;
    } // retranslateUi

};

namespace Ui {
    class EditWidgetIcons: public Ui_EditWidgetIcons {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITWIDGETICONS_H
