/********************************************************************************
** Form generated from reading UI file 'NewDatabaseWizardPage.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NEWDATABASEWIZARDPAGE_H
#define UI_NEWDATABASEWIZARDPAGE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QWizardPage>

QT_BEGIN_NAMESPACE

class Ui_NewDatabaseWizardPage
{
public:
    QVBoxLayout *verticalLayout;
    QScrollArea *pageContent;
    QWidget *content;

    void setupUi(QWizardPage *NewDatabaseWizardPage)
    {
        if (NewDatabaseWizardPage->objectName().isEmpty())
            NewDatabaseWizardPage->setObjectName(QString::fromUtf8("NewDatabaseWizardPage"));
        NewDatabaseWizardPage->resize(578, 410);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(NewDatabaseWizardPage->sizePolicy().hasHeightForWidth());
        NewDatabaseWizardPage->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(NewDatabaseWizardPage);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        pageContent = new QScrollArea(NewDatabaseWizardPage);
        pageContent->setObjectName(QString::fromUtf8("pageContent"));
        QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(pageContent->sizePolicy().hasHeightForWidth());
        pageContent->setSizePolicy(sizePolicy1);
        pageContent->setMinimumSize(QSize(560, 300));
        pageContent->setStyleSheet(QString::fromUtf8("QScrollArea { background: transparent; }\n"
"QScrollArea > QWidget > QWidget { background: transparent; }\n"
"QScrollArea > QWidget > QScrollBar { background: 1; }"));
        pageContent->setFrameShape(QFrame::NoFrame);
        pageContent->setFrameShadow(QFrame::Plain);
        pageContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        pageContent->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        pageContent->setWidgetResizable(true);
        content = new QWidget();
        content->setObjectName(QString::fromUtf8("content"));
        content->setGeometry(QRect(0, 0, 560, 392));
        pageContent->setWidget(content);

        verticalLayout->addWidget(pageContent);

        verticalLayout->setStretch(0, 1);

        retranslateUi(NewDatabaseWizardPage);

        QMetaObject::connectSlotsByName(NewDatabaseWizardPage);
    } // setupUi

    void retranslateUi(QWizardPage *NewDatabaseWizardPage)
    {
        NewDatabaseWizardPage->setWindowTitle(QCoreApplication::translate("NewDatabaseWizardPage", "WizardPage", nullptr));
        NewDatabaseWizardPage->setTitle(QCoreApplication::translate("NewDatabaseWizardPage", "Encryption Settings", nullptr));
        NewDatabaseWizardPage->setSubTitle(QCoreApplication::translate("NewDatabaseWizardPage", "Here you can adjust the database encryption settings. Don't worry, you can change them later in the database settings.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class NewDatabaseWizardPage: public Ui_NewDatabaseWizardPage {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NEWDATABASEWIZARDPAGE_H
