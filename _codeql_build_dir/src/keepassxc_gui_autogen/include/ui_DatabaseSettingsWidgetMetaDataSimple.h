/********************************************************************************
** Form generated from reading UI file 'DatabaseSettingsWidgetMetaDataSimple.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATABASESETTINGSWIDGETMETADATASIMPLE_H
#define UI_DATABASESETTINGSWIDGETMETADATASIMPLE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DatabaseSettingsWidgetMetaDataSimple
{
public:
    QFormLayout *formLayout;
    QLabel *databaseNameLabel;
    QLineEdit *databaseName;
    QLabel *databaseDescriptionLabel;
    QLineEdit *databaseDescription;

    void setupUi(QWidget *DatabaseSettingsWidgetMetaDataSimple)
    {
        if (DatabaseSettingsWidgetMetaDataSimple->objectName().isEmpty())
            DatabaseSettingsWidgetMetaDataSimple->setObjectName(QString::fromUtf8("DatabaseSettingsWidgetMetaDataSimple"));
        DatabaseSettingsWidgetMetaDataSimple->resize(450, 86);
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DatabaseSettingsWidgetMetaDataSimple->sizePolicy().hasHeightForWidth());
        DatabaseSettingsWidgetMetaDataSimple->setSizePolicy(sizePolicy);
        DatabaseSettingsWidgetMetaDataSimple->setMinimumSize(QSize(450, 0));
        formLayout = new QFormLayout(DatabaseSettingsWidgetMetaDataSimple);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        databaseNameLabel = new QLabel(DatabaseSettingsWidgetMetaDataSimple);
        databaseNameLabel->setObjectName(QString::fromUtf8("databaseNameLabel"));

        formLayout->setWidget(0, QFormLayout::LabelRole, databaseNameLabel);

        databaseName = new QLineEdit(DatabaseSettingsWidgetMetaDataSimple);
        databaseName->setObjectName(QString::fromUtf8("databaseName"));

        formLayout->setWidget(0, QFormLayout::FieldRole, databaseName);

        databaseDescriptionLabel = new QLabel(DatabaseSettingsWidgetMetaDataSimple);
        databaseDescriptionLabel->setObjectName(QString::fromUtf8("databaseDescriptionLabel"));

        formLayout->setWidget(1, QFormLayout::LabelRole, databaseDescriptionLabel);

        databaseDescription = new QLineEdit(DatabaseSettingsWidgetMetaDataSimple);
        databaseDescription->setObjectName(QString::fromUtf8("databaseDescription"));

        formLayout->setWidget(1, QFormLayout::FieldRole, databaseDescription);


        retranslateUi(DatabaseSettingsWidgetMetaDataSimple);

        QMetaObject::connectSlotsByName(DatabaseSettingsWidgetMetaDataSimple);
    } // setupUi

    void retranslateUi(QWidget *DatabaseSettingsWidgetMetaDataSimple)
    {
        databaseNameLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetMetaDataSimple", "Database Name:", nullptr));
#if QT_CONFIG(accessibility)
        databaseName->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetMetaDataSimple", "Database name field", nullptr));
#endif // QT_CONFIG(accessibility)
        databaseDescriptionLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetMetaDataSimple", "Description:", nullptr));
#if QT_CONFIG(accessibility)
        databaseDescription->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetMetaDataSimple", "Database description field", nullptr));
#endif // QT_CONFIG(accessibility)
        (void)DatabaseSettingsWidgetMetaDataSimple;
    } // retranslateUi

};

namespace Ui {
    class DatabaseSettingsWidgetMetaDataSimple: public Ui_DatabaseSettingsWidgetMetaDataSimple {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATABASESETTINGSWIDGETMETADATASIMPLE_H
