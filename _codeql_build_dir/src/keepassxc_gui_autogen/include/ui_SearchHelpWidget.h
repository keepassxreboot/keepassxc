/********************************************************************************
** Form generated from reading UI file 'SearchHelpWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SEARCHHELPWIDGET_H
#define UI_SEARCHHELPWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_SearchHelpWidget
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label_2;
    QLabel *label_24;
    QHBoxLayout *horizontalLayout_2;
    QGroupBox *groupBox_2;
    QFormLayout *formLayout;
    QLabel *label_4;
    QLabel *label_7;
    QLabel *label_8;
    QLabel *label_25;
    QLabel *label_15;
    QLabel *label_6;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label_3;
    QLabel *label_11;
    QLabel *label;
    QLabel *label_13;
    QLabel *label_17;
    QLabel *label_5;
    QLabel *label_12;
    QLabel *label_14;
    QLabel *label_16;
    QHBoxLayout *horizontalLayout;
    QGroupBox *groupBox_4;
    QFormLayout *formLayout_2;
    QLabel *label_18;
    QLabel *label_19;
    QLabel *label_20;
    QLabel *label_21;
    QLabel *label_22;
    QLabel *label_23;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_9;
    QLabel *label_10;
    QLabel *label_26;

    void setupUi(QFrame *SearchHelpWidget)
    {
        if (SearchHelpWidget->objectName().isEmpty())
            SearchHelpWidget->setObjectName(QString::fromUtf8("SearchHelpWidget"));
        SearchHelpWidget->resize(397, 264);
        SearchHelpWidget->setAutoFillBackground(false);
        SearchHelpWidget->setFrameShape(QFrame::StyledPanel);
        verticalLayout = new QVBoxLayout(SearchHelpWidget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label_2 = new QLabel(SearchHelpWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        label_2->setFont(font);
        label_2->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(label_2);

        label_24 = new QLabel(SearchHelpWidget);
        label_24->setObjectName(QString::fromUtf8("label_24"));
        sizePolicy.setHeightForWidth(label_24->sizePolicy().hasHeightForWidth());
        label_24->setSizePolicy(sizePolicy);
        label_24->setFont(font);
        label_24->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(label_24);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        groupBox_2 = new QGroupBox(SearchHelpWidget);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        QFont font1;
        font1.setBold(false);
        font1.setWeight(50);
        groupBox_2->setFont(font1);
        formLayout = new QFormLayout(groupBox_2);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setMinimumSize(QSize(10, 0));
        label_4->setFont(font);
        label_4->setText(QString::fromUtf8("!"));
        label_4->setScaledContents(false);
        label_4->setAlignment(Qt::AlignCenter);

        formLayout->setWidget(0, QFormLayout::LabelRole, label_4);

        label_7 = new QLabel(groupBox_2);
        label_7->setObjectName(QString::fromUtf8("label_7"));

        formLayout->setWidget(0, QFormLayout::FieldRole, label_7);

        label_8 = new QLabel(groupBox_2);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        formLayout->setWidget(1, QFormLayout::FieldRole, label_8);

        label_25 = new QLabel(groupBox_2);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setMinimumSize(QSize(10, 0));
        label_25->setFont(font);
        label_25->setText(QString::fromUtf8("*"));
        label_25->setAlignment(Qt::AlignCenter);

        formLayout->setWidget(2, QFormLayout::LabelRole, label_25);

        label_15 = new QLabel(groupBox_2);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        formLayout->setWidget(2, QFormLayout::FieldRole, label_15);

        label_6 = new QLabel(groupBox_2);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setMinimumSize(QSize(10, 0));
        label_6->setFont(font);
        label_6->setText(QString::fromUtf8("+"));
        label_6->setAlignment(Qt::AlignCenter);

        formLayout->setWidget(1, QFormLayout::LabelRole, label_6);


        horizontalLayout_2->addWidget(groupBox_2);

        groupBox = new QGroupBox(SearchHelpWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setFont(font1);
        groupBox->setFlat(false);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        label_3->setText(QString::fromUtf8("username (u)"));

        gridLayout->addWidget(label_3, 1, 0, 1, 1);

        label_11 = new QLabel(groupBox);
        label_11->setObjectName(QString::fromUtf8("label_11"));
        label_11->setText(QString::fromUtf8("password (p, pw)"));

        gridLayout->addWidget(label_11, 2, 0, 1, 1);

        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        label->setText(QString::fromUtf8("title (t)"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        label_13 = new QLabel(groupBox);
        label_13->setObjectName(QString::fromUtf8("label_13"));
        label_13->setText(QString::fromUtf8("url"));

        gridLayout->addWidget(label_13, 3, 0, 1, 1);

        label_17 = new QLabel(groupBox);
        label_17->setObjectName(QString::fromUtf8("label_17"));
        label_17->setText(QString::fromUtf8("uuid"));

        gridLayout->addWidget(label_17, 4, 0, 1, 1);

        label_5 = new QLabel(groupBox);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setText(QString::fromUtf8("notes (n)"));

        gridLayout->addWidget(label_5, 0, 1, 1, 1);

        label_12 = new QLabel(groupBox);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setText(QString::fromUtf8("attribute (attr)"));

        gridLayout->addWidget(label_12, 1, 1, 1, 1);

        label_14 = new QLabel(groupBox);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setText(QString::fromUtf8("attachment (attach)"));

        gridLayout->addWidget(label_14, 2, 1, 1, 1);

        label_16 = new QLabel(groupBox);
        label_16->setObjectName(QString::fromUtf8("label_16"));
        label_16->setText(QString::fromUtf8("group (g)"));

        gridLayout->addWidget(label_16, 3, 1, 1, 1);


        horizontalLayout_2->addWidget(groupBox);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        groupBox_4 = new QGroupBox(SearchHelpWidget);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        formLayout_2 = new QFormLayout(groupBox_4);
        formLayout_2->setObjectName(QString::fromUtf8("formLayout_2"));
        label_18 = new QLabel(groupBox_4);
        label_18->setObjectName(QString::fromUtf8("label_18"));
        label_18->setMinimumSize(QSize(10, 0));
        label_18->setFont(font);
        label_18->setText(QString::fromUtf8("*"));
        label_18->setScaledContents(false);
        label_18->setAlignment(Qt::AlignCenter);

        formLayout_2->setWidget(0, QFormLayout::LabelRole, label_18);

        label_19 = new QLabel(groupBox_4);
        label_19->setObjectName(QString::fromUtf8("label_19"));
        label_19->setMinimumSize(QSize(10, 0));
        label_19->setFont(font);
        label_19->setText(QString::fromUtf8("?"));
        label_19->setAlignment(Qt::AlignCenter);

        formLayout_2->setWidget(1, QFormLayout::LabelRole, label_19);

        label_20 = new QLabel(groupBox_4);
        label_20->setObjectName(QString::fromUtf8("label_20"));
        label_20->setMinimumSize(QSize(10, 0));
        label_20->setFont(font);
        label_20->setText(QString::fromUtf8("|"));
        label_20->setAlignment(Qt::AlignCenter);

        formLayout_2->setWidget(2, QFormLayout::LabelRole, label_20);

        label_21 = new QLabel(groupBox_4);
        label_21->setObjectName(QString::fromUtf8("label_21"));

        formLayout_2->setWidget(0, QFormLayout::FieldRole, label_21);

        label_22 = new QLabel(groupBox_4);
        label_22->setObjectName(QString::fromUtf8("label_22"));

        formLayout_2->setWidget(1, QFormLayout::FieldRole, label_22);

        label_23 = new QLabel(groupBox_4);
        label_23->setObjectName(QString::fromUtf8("label_23"));

        formLayout_2->setWidget(2, QFormLayout::FieldRole, label_23);


        horizontalLayout->addWidget(groupBox_4);

        groupBox_3 = new QGroupBox(SearchHelpWidget);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_2 = new QVBoxLayout(groupBox_3);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        label_9 = new QLabel(groupBox_3);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_9->sizePolicy().hasHeightForWidth());
        label_9->setSizePolicy(sizePolicy1);
        label_9->setText(QString::fromUtf8("user:name1 url:google"));

        verticalLayout_2->addWidget(label_9);

        label_10 = new QLabel(groupBox_3);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        sizePolicy1.setHeightForWidth(label_10->sizePolicy().hasHeightForWidth());
        label_10->setSizePolicy(sizePolicy1);
        label_10->setText(QString::fromUtf8("user:\"name1|name2\""));

        verticalLayout_2->addWidget(label_10);

        label_26 = new QLabel(groupBox_3);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        sizePolicy1.setHeightForWidth(label_26->sizePolicy().hasHeightForWidth());
        label_26->setSizePolicy(sizePolicy1);
        label_26->setText(QString::fromUtf8("+user:name1 *notes:\"secret \\d\""));

        verticalLayout_2->addWidget(label_26);


        horizontalLayout->addWidget(groupBox_3);


        verticalLayout->addLayout(horizontalLayout);


        retranslateUi(SearchHelpWidget);

        QMetaObject::connectSlotsByName(SearchHelpWidget);
    } // setupUi

    void retranslateUi(QFrame *SearchHelpWidget)
    {
        SearchHelpWidget->setWindowTitle(QCoreApplication::translate("SearchHelpWidget", "Search Help", nullptr));
        label_2->setText(QCoreApplication::translate("SearchHelpWidget", "Search terms are as follows: [modifiers][field:][\"]term[\"]", nullptr));
        label_24->setText(QCoreApplication::translate("SearchHelpWidget", "Every search term must match (ie, logical AND)", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("SearchHelpWidget", "Modifiers", nullptr));
        label_7->setText(QCoreApplication::translate("SearchHelpWidget", "exclude term from results", nullptr));
        label_8->setText(QCoreApplication::translate("SearchHelpWidget", "match term exactly", nullptr));
        label_15->setText(QCoreApplication::translate("SearchHelpWidget", "use regex in term", nullptr));
        groupBox->setTitle(QCoreApplication::translate("SearchHelpWidget", "Fields", nullptr));
        groupBox_4->setTitle(QCoreApplication::translate("SearchHelpWidget", "Term Wildcards", nullptr));
        label_21->setText(QCoreApplication::translate("SearchHelpWidget", "match anything", nullptr));
        label_22->setText(QCoreApplication::translate("SearchHelpWidget", "match one", nullptr));
        label_23->setText(QCoreApplication::translate("SearchHelpWidget", "logical OR", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("SearchHelpWidget", "Examples", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SearchHelpWidget: public Ui_SearchHelpWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SEARCHHELPWIDGET_H
