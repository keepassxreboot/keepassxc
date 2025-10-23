/********************************************************************************
** Form generated from reading UI file 'KeyComponentWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_KEYCOMPONENTWIDGET_H
#define UI_KEYCOMPONENTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_KeyComponentWidget
{
public:
    QVBoxLayout *verticalLayout_5;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_4;
    QStackedWidget *stackedWidget;
    QWidget *addPage;
    QVBoxLayout *verticalLayout;
    QLabel *componentDescription;
    QPushButton *addButton;
    QWidget *editPage;
    QVBoxLayout *verticalLayout_2;
    QWidget *componentWidgetContainer;
    QVBoxLayout *componentWidgetLayout;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QPushButton *cancelButton;
    QWidget *changeOrRemovePage;
    QVBoxLayout *verticalLayout_3;
    QLabel *changeOrRemoveLabel;
    QHBoxLayout *horizontalLayout;
    QPushButton *changeButton;
    QPushButton *removeButton;

    void setupUi(QWidget *KeyComponentWidget)
    {
        if (KeyComponentWidget->objectName().isEmpty())
            KeyComponentWidget->setObjectName(QString::fromUtf8("KeyComponentWidget"));
        KeyComponentWidget->resize(354, 106);
        verticalLayout_5 = new QVBoxLayout(KeyComponentWidget);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout_5->setContentsMargins(0, 0, 0, 0);
        groupBox = new QGroupBox(KeyComponentWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy);
        groupBox->setStyleSheet(QString::fromUtf8("QGroupBox { font-weight: bold; }"));
        verticalLayout_4 = new QVBoxLayout(groupBox);
        verticalLayout_4->setSpacing(15);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        stackedWidget = new QStackedWidget(groupBox);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        addPage = new QWidget();
        addPage->setObjectName(QString::fromUtf8("addPage"));
        sizePolicy.setHeightForWidth(addPage->sizePolicy().hasHeightForWidth());
        addPage->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(addPage);
        verticalLayout->setSpacing(10);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        componentDescription = new QLabel(addPage);
        componentDescription->setObjectName(QString::fromUtf8("componentDescription"));
        componentDescription->setTextFormat(Qt::RichText);
        componentDescription->setOpenExternalLinks(true);
        componentDescription->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard|Qt::LinksAccessibleByMouse);

        verticalLayout->addWidget(componentDescription);

        addButton = new QPushButton(addPage);
        addButton->setObjectName(QString::fromUtf8("addButton"));
        addButton->setText(QString::fromUtf8("Add Key Component"));

        verticalLayout->addWidget(addButton);

        stackedWidget->addWidget(addPage);
        editPage = new QWidget();
        editPage->setObjectName(QString::fromUtf8("editPage"));
        verticalLayout_2 = new QVBoxLayout(editPage);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        componentWidgetContainer = new QWidget(editPage);
        componentWidgetContainer->setObjectName(QString::fromUtf8("componentWidgetContainer"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(componentWidgetContainer->sizePolicy().hasHeightForWidth());
        componentWidgetContainer->setSizePolicy(sizePolicy1);
        componentWidgetLayout = new QVBoxLayout(componentWidgetContainer);
        componentWidgetLayout->setObjectName(QString::fromUtf8("componentWidgetLayout"));
        componentWidgetLayout->setSizeConstraint(QLayout::SetMinimumSize);
        componentWidgetLayout->setContentsMargins(0, 0, 0, 0);

        verticalLayout_2->addWidget(componentWidgetContainer);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        cancelButton = new QPushButton(editPage);
        cancelButton->setObjectName(QString::fromUtf8("cancelButton"));

        horizontalLayout_2->addWidget(cancelButton);


        verticalLayout_2->addLayout(horizontalLayout_2);

        stackedWidget->addWidget(editPage);
        changeOrRemovePage = new QWidget();
        changeOrRemovePage->setObjectName(QString::fromUtf8("changeOrRemovePage"));
        sizePolicy.setHeightForWidth(changeOrRemovePage->sizePolicy().hasHeightForWidth());
        changeOrRemovePage->setSizePolicy(sizePolicy);
        verticalLayout_3 = new QVBoxLayout(changeOrRemovePage);
        verticalLayout_3->setSpacing(10);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        changeOrRemoveLabel = new QLabel(changeOrRemovePage);
        changeOrRemoveLabel->setObjectName(QString::fromUtf8("changeOrRemoveLabel"));

        verticalLayout_3->addWidget(changeOrRemoveLabel);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        changeButton = new QPushButton(changeOrRemovePage);
        changeButton->setObjectName(QString::fromUtf8("changeButton"));
        changeButton->setText(QString::fromUtf8("Change Key Component"));

        horizontalLayout->addWidget(changeButton);

        removeButton = new QPushButton(changeOrRemovePage);
        removeButton->setObjectName(QString::fromUtf8("removeButton"));
        removeButton->setText(QString::fromUtf8("Remove Key Component"));

        horizontalLayout->addWidget(removeButton);


        verticalLayout_3->addLayout(horizontalLayout);

        stackedWidget->addWidget(changeOrRemovePage);

        verticalLayout_4->addWidget(stackedWidget);


        verticalLayout_5->addWidget(groupBox);


        retranslateUi(KeyComponentWidget);

        stackedWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(KeyComponentWidget);
    } // setupUi

    void retranslateUi(QWidget *KeyComponentWidget)
    {
        groupBox->setTitle(QCoreApplication::translate("KeyComponentWidget", "Key Component", nullptr));
        componentDescription->setText(QCoreApplication::translate("KeyComponentWidget", "Key Component Description", nullptr));
        cancelButton->setText(QCoreApplication::translate("KeyComponentWidget", "Cancel", nullptr));
        changeOrRemoveLabel->setText(QCoreApplication::translate("KeyComponentWidget", "Key Component set, click to change or remove", nullptr));
        (void)KeyComponentWidget;
    } // retranslateUi

};

namespace Ui {
    class KeyComponentWidget: public Ui_KeyComponentWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_KEYCOMPONENTWIDGET_H
