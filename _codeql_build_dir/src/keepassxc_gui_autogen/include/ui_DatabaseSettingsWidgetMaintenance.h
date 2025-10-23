/********************************************************************************
** Form generated from reading UI file 'DatabaseSettingsWidgetMaintenance.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATABASESETTINGSWIDGETMAINTENANCE_H
#define UI_DATABASESETTINGSWIDGETMAINTENANCE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QListView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DatabaseSettingsWidgetMaintenance
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_1;
    QListView *customIconsView;
    QHBoxLayout *customIconButtonsHorizontalLayout;
    QPushButton *deleteButton;
    QPushButton *purgeButton;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *DatabaseSettingsWidgetMaintenance)
    {
        if (DatabaseSettingsWidgetMaintenance->objectName().isEmpty())
            DatabaseSettingsWidgetMaintenance->setObjectName(QString::fromUtf8("DatabaseSettingsWidgetMaintenance"));
        DatabaseSettingsWidgetMaintenance->resize(669, 395);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DatabaseSettingsWidgetMaintenance->sizePolicy().hasHeightForWidth());
        DatabaseSettingsWidgetMaintenance->setSizePolicy(sizePolicy);
        DatabaseSettingsWidgetMaintenance->setMinimumSize(QSize(450, 0));
        verticalLayout = new QVBoxLayout(DatabaseSettingsWidgetMaintenance);
        verticalLayout->setSpacing(20);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(DatabaseSettingsWidgetMaintenance);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout_1 = new QVBoxLayout(groupBox);
        verticalLayout_1->setObjectName(QString::fromUtf8("verticalLayout_1"));
        customIconsView = new QListView(groupBox);
        customIconsView->setObjectName(QString::fromUtf8("customIconsView"));
        customIconsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        customIconsView->setSelectionMode(QAbstractItemView::MultiSelection);
        customIconsView->setMovement(QListView::Static);
        customIconsView->setFlow(QListView::LeftToRight);
        customIconsView->setProperty("isWrapping", QVariant(true));
        customIconsView->setResizeMode(QListView::Adjust);
        customIconsView->setSpacing(4);
        customIconsView->setViewMode(QListView::ListMode);

        verticalLayout_1->addWidget(customIconsView);

        customIconButtonsHorizontalLayout = new QHBoxLayout();
        customIconButtonsHorizontalLayout->setObjectName(QString::fromUtf8("customIconButtonsHorizontalLayout"));
        deleteButton = new QPushButton(groupBox);
        deleteButton->setObjectName(QString::fromUtf8("deleteButton"));

        customIconButtonsHorizontalLayout->addWidget(deleteButton);

        purgeButton = new QPushButton(groupBox);
        purgeButton->setObjectName(QString::fromUtf8("purgeButton"));

        customIconButtonsHorizontalLayout->addWidget(purgeButton);


        verticalLayout_1->addLayout(customIconButtonsHorizontalLayout);


        verticalLayout->addWidget(groupBox);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);


        retranslateUi(DatabaseSettingsWidgetMaintenance);

        QMetaObject::connectSlotsByName(DatabaseSettingsWidgetMaintenance);
    } // setupUi

    void retranslateUi(QWidget *DatabaseSettingsWidgetMaintenance)
    {
        groupBox->setTitle(QCoreApplication::translate("DatabaseSettingsWidgetMaintenance", "Manage Custom Icons", nullptr));
        deleteButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetMaintenance", "Delete selected icon(s)", nullptr));
#if QT_CONFIG(tooltip)
        purgeButton->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetMaintenance", "Delete all custom icons not in use by any entry or group", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        purgeButton->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetMaintenance", "Delete all custom icons not in use by any entry or group", nullptr));
#endif // QT_CONFIG(accessibility)
        purgeButton->setText(QCoreApplication::translate("DatabaseSettingsWidgetMaintenance", "Purge unused icons", nullptr));
        (void)DatabaseSettingsWidgetMaintenance;
    } // retranslateUi

};

namespace Ui {
    class DatabaseSettingsWidgetMaintenance: public Ui_DatabaseSettingsWidgetMaintenance {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATABASESETTINGSWIDGETMAINTENANCE_H
