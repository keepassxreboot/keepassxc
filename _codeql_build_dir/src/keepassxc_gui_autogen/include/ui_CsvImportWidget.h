/********************************************************************************
** Form generated from reading UI file 'CsvImportWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CSVIMPORTWIDGET_H
#define UI_CSVIMPORTWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CsvImportWidget
{
public:
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *parserOptionsLayout;
    QGroupBox *ColumnAssociation;
    QGridLayout *gridLayout;
    QComboBox *urlCombo;
    QComboBox *usernameCombo;
    QLabel *groupLabel;
    QLabel *notesLabel;
    QLabel *usernameLabel;
    QComboBox *lastModifiedCombo;
    QComboBox *iconCombo;
    QComboBox *totpCombo;
    QLabel *lastModifiedLabel;
    QComboBox *titleCombo;
    QComboBox *notesCombo;
    QLabel *passwordLabel;
    QSpacerItem *horizontalSpacer_2;
    QLabel *createdLabel;
    QComboBox *createdCombo;
    QLabel *urlLabel;
    QLabel *totpLabel;
    QComboBox *passwordCombo;
    QLabel *iconLabel;
    QLabel *titleLabel;
    QComboBox *groupCombo;
    QLabel *tagsLabel;
    QComboBox *tagsCombo;
    QGroupBox *Encoding;
    QFormLayout *formLayout;
    QLabel *labelCodec;
    QComboBox *comboBoxCodec;
    QLabel *labelTextQualifier;
    QComboBox *comboBoxTextQualifier;
    QLabel *labelFieldSeparator;
    QComboBox *comboBoxFieldSeparator;
    QLabel *labelComments;
    QComboBox *comboBoxComment;
    QLabel *labelSkipRows;
    QSpinBox *spinBoxSkip;
    QCheckBox *checkBoxBackslash;
    QCheckBox *checkBoxFieldNames;
    QSpacerItem *horizontalSpacer;
    QGroupBox *previewLayout;
    QVBoxLayout *verticalLayout;
    QLabel *labelSizeRowsCols;
    QTableView *tableViewFields;

    void setupUi(QWidget *CsvImportWidget)
    {
        if (CsvImportWidget->objectName().isEmpty())
            CsvImportWidget->setObjectName(QString::fromUtf8("CsvImportWidget"));
        CsvImportWidget->resize(820, 523);
        verticalLayout_2 = new QVBoxLayout(CsvImportWidget);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        parserOptionsLayout = new QHBoxLayout();
        parserOptionsLayout->setObjectName(QString::fromUtf8("parserOptionsLayout"));
        ColumnAssociation = new QGroupBox(CsvImportWidget);
        ColumnAssociation->setObjectName(QString::fromUtf8("ColumnAssociation"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        ColumnAssociation->setFont(font);
        gridLayout = new QGridLayout(ColumnAssociation);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        urlCombo = new QComboBox(ColumnAssociation);
        urlCombo->setObjectName(QString::fromUtf8("urlCombo"));
        urlCombo->setMaximumSize(QSize(200, 16777215));
        urlCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(urlCombo, 4, 1, 1, 1);

        usernameCombo = new QComboBox(ColumnAssociation);
        usernameCombo->setObjectName(QString::fromUtf8("usernameCombo"));
        usernameCombo->setMaximumSize(QSize(200, 16777215));
        usernameCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(usernameCombo, 2, 1, 1, 1);

        groupLabel = new QLabel(ColumnAssociation);
        groupLabel->setObjectName(QString::fromUtf8("groupLabel"));
        QFont font1;
        font1.setBold(false);
        font1.setWeight(50);
        groupLabel->setFont(font1);
        groupLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        groupLabel->setIndent(2);

        gridLayout->addWidget(groupLabel, 0, 0, 1, 1);

        notesLabel = new QLabel(ColumnAssociation);
        notesLabel->setObjectName(QString::fromUtf8("notesLabel"));
        notesLabel->setFont(font1);
        notesLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        notesLabel->setIndent(2);

        gridLayout->addWidget(notesLabel, 0, 3, 1, 1);

        usernameLabel = new QLabel(ColumnAssociation);
        usernameLabel->setObjectName(QString::fromUtf8("usernameLabel"));
        usernameLabel->setFont(font1);
        usernameLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        usernameLabel->setIndent(2);

        gridLayout->addWidget(usernameLabel, 2, 0, 1, 1);

        lastModifiedCombo = new QComboBox(ColumnAssociation);
        lastModifiedCombo->setObjectName(QString::fromUtf8("lastModifiedCombo"));
        lastModifiedCombo->setMaximumSize(QSize(200, 16777215));
        lastModifiedCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(lastModifiedCombo, 3, 4, 1, 1);

        iconCombo = new QComboBox(ColumnAssociation);
        iconCombo->setObjectName(QString::fromUtf8("iconCombo"));
        iconCombo->setMaximumSize(QSize(200, 16777215));
        iconCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(iconCombo, 2, 4, 1, 1);

        totpCombo = new QComboBox(ColumnAssociation);
        totpCombo->setObjectName(QString::fromUtf8("totpCombo"));
        totpCombo->setMaximumSize(QSize(200, 16777215));
        totpCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(totpCombo, 1, 4, 1, 1);

        lastModifiedLabel = new QLabel(ColumnAssociation);
        lastModifiedLabel->setObjectName(QString::fromUtf8("lastModifiedLabel"));
        lastModifiedLabel->setFont(font1);
        lastModifiedLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        lastModifiedLabel->setIndent(2);

        gridLayout->addWidget(lastModifiedLabel, 3, 3, 1, 1);

        titleCombo = new QComboBox(ColumnAssociation);
        titleCombo->setObjectName(QString::fromUtf8("titleCombo"));
        titleCombo->setMaximumSize(QSize(200, 16777215));
        titleCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(titleCombo, 1, 1, 1, 1);

        notesCombo = new QComboBox(ColumnAssociation);
        notesCombo->setObjectName(QString::fromUtf8("notesCombo"));
        notesCombo->setMaximumSize(QSize(200, 16777215));
        notesCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(notesCombo, 0, 4, 1, 1);

        passwordLabel = new QLabel(ColumnAssociation);
        passwordLabel->setObjectName(QString::fromUtf8("passwordLabel"));
        passwordLabel->setFont(font1);
        passwordLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        passwordLabel->setIndent(2);

        gridLayout->addWidget(passwordLabel, 3, 0, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(6, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        createdLabel = new QLabel(ColumnAssociation);
        createdLabel->setObjectName(QString::fromUtf8("createdLabel"));
        createdLabel->setFont(font1);
        createdLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        createdLabel->setIndent(2);

        gridLayout->addWidget(createdLabel, 4, 3, 1, 1);

        createdCombo = new QComboBox(ColumnAssociation);
        createdCombo->setObjectName(QString::fromUtf8("createdCombo"));
        createdCombo->setMaximumSize(QSize(200, 16777215));
        createdCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(createdCombo, 4, 4, 1, 1);

        urlLabel = new QLabel(ColumnAssociation);
        urlLabel->setObjectName(QString::fromUtf8("urlLabel"));
        urlLabel->setFont(font1);
        urlLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        urlLabel->setIndent(2);

        gridLayout->addWidget(urlLabel, 4, 0, 1, 1);

        totpLabel = new QLabel(ColumnAssociation);
        totpLabel->setObjectName(QString::fromUtf8("totpLabel"));
        totpLabel->setFont(font1);
        totpLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        totpLabel->setIndent(2);

        gridLayout->addWidget(totpLabel, 1, 3, 1, 1);

        passwordCombo = new QComboBox(ColumnAssociation);
        passwordCombo->setObjectName(QString::fromUtf8("passwordCombo"));
        passwordCombo->setMaximumSize(QSize(200, 16777215));
        passwordCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(passwordCombo, 3, 1, 1, 1);

        iconLabel = new QLabel(ColumnAssociation);
        iconLabel->setObjectName(QString::fromUtf8("iconLabel"));
        iconLabel->setFont(font1);
        iconLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        iconLabel->setIndent(2);

        gridLayout->addWidget(iconLabel, 2, 3, 1, 1);

        titleLabel = new QLabel(ColumnAssociation);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setFont(font1);
        titleLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        titleLabel->setIndent(2);

        gridLayout->addWidget(titleLabel, 1, 0, 1, 1);

        groupCombo = new QComboBox(ColumnAssociation);
        groupCombo->setObjectName(QString::fromUtf8("groupCombo"));
        groupCombo->setMaximumSize(QSize(200, 16777215));
        groupCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(groupCombo, 0, 1, 1, 1);

        tagsLabel = new QLabel(ColumnAssociation);
        tagsLabel->setObjectName(QString::fromUtf8("tagsLabel"));
        tagsLabel->setFont(font1);
        tagsLabel->setIndent(2);

        gridLayout->addWidget(tagsLabel, 5, 0, 1, 1);

        tagsCombo = new QComboBox(ColumnAssociation);
        tagsCombo->setObjectName(QString::fromUtf8("tagsCombo"));
        tagsCombo->setMaximumSize(QSize(200, 16777215));
        tagsCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        gridLayout->addWidget(tagsCombo, 5, 1, 1, 1);


        parserOptionsLayout->addWidget(ColumnAssociation);

        Encoding = new QGroupBox(CsvImportWidget);
        Encoding->setObjectName(QString::fromUtf8("Encoding"));
        Encoding->setFont(font);
        formLayout = new QFormLayout(Encoding);
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        labelCodec = new QLabel(Encoding);
        labelCodec->setObjectName(QString::fromUtf8("labelCodec"));
        labelCodec->setFont(font1);
        labelCodec->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(0, QFormLayout::LabelRole, labelCodec);

        comboBoxCodec = new QComboBox(Encoding);
        comboBoxCodec->addItem(QString::fromUtf8("UTF-8"));
        comboBoxCodec->addItem(QString::fromUtf8("Windows-1252"));
        comboBoxCodec->addItem(QString::fromUtf8("UTF-16"));
        comboBoxCodec->addItem(QString::fromUtf8("UTF-16LE"));
        comboBoxCodec->setObjectName(QString::fromUtf8("comboBoxCodec"));
        comboBoxCodec->setFont(font1);
        comboBoxCodec->setEditable(false);
        comboBoxCodec->setCurrentText(QString::fromUtf8("UTF-8"));

        formLayout->setWidget(0, QFormLayout::FieldRole, comboBoxCodec);

        labelTextQualifier = new QLabel(Encoding);
        labelTextQualifier->setObjectName(QString::fromUtf8("labelTextQualifier"));
        labelTextQualifier->setFont(font1);
        labelTextQualifier->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(1, QFormLayout::LabelRole, labelTextQualifier);

        comboBoxTextQualifier = new QComboBox(Encoding);
        comboBoxTextQualifier->addItem(QString::fromUtf8("\""));
        comboBoxTextQualifier->addItem(QString::fromUtf8("'"));
        comboBoxTextQualifier->addItem(QString::fromUtf8(":"));
        comboBoxTextQualifier->addItem(QString::fromUtf8("."));
        comboBoxTextQualifier->addItem(QString::fromUtf8("|"));
        comboBoxTextQualifier->setObjectName(QString::fromUtf8("comboBoxTextQualifier"));
        comboBoxTextQualifier->setFont(font1);
        comboBoxTextQualifier->setEditable(false);
        comboBoxTextQualifier->setCurrentText(QString::fromUtf8("\""));

        formLayout->setWidget(1, QFormLayout::FieldRole, comboBoxTextQualifier);

        labelFieldSeparator = new QLabel(Encoding);
        labelFieldSeparator->setObjectName(QString::fromUtf8("labelFieldSeparator"));
        labelFieldSeparator->setFont(font1);
        labelFieldSeparator->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(2, QFormLayout::LabelRole, labelFieldSeparator);

        comboBoxFieldSeparator = new QComboBox(Encoding);
        comboBoxFieldSeparator->addItem(QString::fromUtf8(","));
        comboBoxFieldSeparator->addItem(QString::fromUtf8(";"));
        comboBoxFieldSeparator->addItem(QString::fromUtf8("-"));
        comboBoxFieldSeparator->addItem(QString::fromUtf8(":"));
        comboBoxFieldSeparator->addItem(QString::fromUtf8("."));
        comboBoxFieldSeparator->addItem(QString::fromUtf8("TAB (\\t)"));
        comboBoxFieldSeparator->setObjectName(QString::fromUtf8("comboBoxFieldSeparator"));
        comboBoxFieldSeparator->setFont(font1);
        comboBoxFieldSeparator->setEditable(false);
        comboBoxFieldSeparator->setCurrentText(QString::fromUtf8(","));

        formLayout->setWidget(2, QFormLayout::FieldRole, comboBoxFieldSeparator);

        labelComments = new QLabel(Encoding);
        labelComments->setObjectName(QString::fromUtf8("labelComments"));
        labelComments->setFont(font1);
        labelComments->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(3, QFormLayout::LabelRole, labelComments);

        comboBoxComment = new QComboBox(Encoding);
        comboBoxComment->addItem(QString::fromUtf8("#"));
        comboBoxComment->addItem(QString::fromUtf8(";"));
        comboBoxComment->addItem(QString::fromUtf8(":"));
        comboBoxComment->addItem(QString::fromUtf8("@"));
        comboBoxComment->setObjectName(QString::fromUtf8("comboBoxComment"));
        comboBoxComment->setFont(font1);
        comboBoxComment->setEditable(false);
        comboBoxComment->setCurrentText(QString::fromUtf8("#"));

        formLayout->setWidget(3, QFormLayout::FieldRole, comboBoxComment);

        labelSkipRows = new QLabel(Encoding);
        labelSkipRows->setObjectName(QString::fromUtf8("labelSkipRows"));
        labelSkipRows->setFont(font1);
        labelSkipRows->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        formLayout->setWidget(4, QFormLayout::LabelRole, labelSkipRows);

        spinBoxSkip = new QSpinBox(Encoding);
        spinBoxSkip->setObjectName(QString::fromUtf8("spinBoxSkip"));
        spinBoxSkip->setFont(font1);

        formLayout->setWidget(4, QFormLayout::FieldRole, spinBoxSkip);

        checkBoxBackslash = new QCheckBox(Encoding);
        checkBoxBackslash->setObjectName(QString::fromUtf8("checkBoxBackslash"));
        checkBoxBackslash->setFont(font1);

        formLayout->setWidget(5, QFormLayout::SpanningRole, checkBoxBackslash);

        checkBoxFieldNames = new QCheckBox(Encoding);
        checkBoxFieldNames->setObjectName(QString::fromUtf8("checkBoxFieldNames"));
        checkBoxFieldNames->setFont(font1);
        checkBoxFieldNames->setChecked(true);

        formLayout->setWidget(6, QFormLayout::SpanningRole, checkBoxFieldNames);


        parserOptionsLayout->addWidget(Encoding);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        parserOptionsLayout->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(parserOptionsLayout);

        previewLayout = new QGroupBox(CsvImportWidget);
        previewLayout->setObjectName(QString::fromUtf8("previewLayout"));
        previewLayout->setFont(font);
        previewLayout->setCheckable(false);
        verticalLayout = new QVBoxLayout(previewLayout);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(-1, 3, -1, 0);
        labelSizeRowsCols = new QLabel(previewLayout);
        labelSizeRowsCols->setObjectName(QString::fromUtf8("labelSizeRowsCols"));
        labelSizeRowsCols->setFont(font1);

        verticalLayout->addWidget(labelSizeRowsCols);

        tableViewFields = new QTableView(previewLayout);
        tableViewFields->setObjectName(QString::fromUtf8("tableViewFields"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(tableViewFields->sizePolicy().hasHeightForWidth());
        tableViewFields->setSizePolicy(sizePolicy);
        tableViewFields->setMinimumSize(QSize(800, 300));
        tableViewFields->setFont(font1);
        tableViewFields->setCornerButtonEnabled(false);
        tableViewFields->horizontalHeader()->setVisible(true);

        verticalLayout->addWidget(tableViewFields);


        verticalLayout_2->addWidget(previewLayout);

        QWidget::setTabOrder(comboBoxCodec, comboBoxTextQualifier);
        QWidget::setTabOrder(comboBoxTextQualifier, comboBoxFieldSeparator);
        QWidget::setTabOrder(comboBoxFieldSeparator, comboBoxComment);
        QWidget::setTabOrder(comboBoxComment, tableViewFields);

        retranslateUi(CsvImportWidget);

        QMetaObject::connectSlotsByName(CsvImportWidget);
    } // setupUi

    void retranslateUi(QWidget *CsvImportWidget)
    {
        CsvImportWidget->setWindowTitle(QString());
        ColumnAssociation->setTitle(QCoreApplication::translate("CsvImportWidget", "Column Association", nullptr));
        groupLabel->setText(QCoreApplication::translate("CsvImportWidget", "Group", nullptr));
        notesLabel->setText(QCoreApplication::translate("CsvImportWidget", "Notes", nullptr));
        usernameLabel->setText(QCoreApplication::translate("CsvImportWidget", "Username", nullptr));
        lastModifiedLabel->setText(QCoreApplication::translate("CsvImportWidget", "Last Modified", nullptr));
        passwordLabel->setText(QCoreApplication::translate("CsvImportWidget", "Password", nullptr));
        createdLabel->setText(QCoreApplication::translate("CsvImportWidget", "Created", nullptr));
        urlLabel->setText(QCoreApplication::translate("CsvImportWidget", "URL", nullptr));
        totpLabel->setText(QCoreApplication::translate("CsvImportWidget", "TOTP", nullptr));
        iconLabel->setText(QCoreApplication::translate("CsvImportWidget", "Icon", nullptr));
        titleLabel->setText(QCoreApplication::translate("CsvImportWidget", "Title", nullptr));
        tagsLabel->setText(QCoreApplication::translate("CsvImportWidget", "Tags", nullptr));
        Encoding->setTitle(QCoreApplication::translate("CsvImportWidget", "Encoding", nullptr));
        labelCodec->setText(QCoreApplication::translate("CsvImportWidget", "Codec", nullptr));

#if QT_CONFIG(accessibility)
        comboBoxCodec->setAccessibleName(QCoreApplication::translate("CsvImportWidget", "Codec", nullptr));
#endif // QT_CONFIG(accessibility)
        labelTextQualifier->setText(QCoreApplication::translate("CsvImportWidget", "Text is qualified by", nullptr));

#if QT_CONFIG(accessibility)
        comboBoxTextQualifier->setAccessibleName(QCoreApplication::translate("CsvImportWidget", "Text qualification", nullptr));
#endif // QT_CONFIG(accessibility)
        labelFieldSeparator->setText(QCoreApplication::translate("CsvImportWidget", "Fields are separated by", nullptr));

#if QT_CONFIG(accessibility)
        comboBoxFieldSeparator->setAccessibleName(QCoreApplication::translate("CsvImportWidget", "Field separation", nullptr));
#endif // QT_CONFIG(accessibility)
        labelComments->setText(QCoreApplication::translate("CsvImportWidget", "Comments start with", nullptr));

#if QT_CONFIG(accessibility)
        comboBoxComment->setAccessibleName(QCoreApplication::translate("CsvImportWidget", "Comments start with", nullptr));
#endif // QT_CONFIG(accessibility)
        labelSkipRows->setText(QCoreApplication::translate("CsvImportWidget", "Header lines skipped", nullptr));
#if QT_CONFIG(accessibility)
        spinBoxSkip->setAccessibleName(QCoreApplication::translate("CsvImportWidget", "Number of header lines to discard", nullptr));
#endif // QT_CONFIG(accessibility)
        checkBoxBackslash->setText(QCoreApplication::translate("CsvImportWidget", "Consider '\\' an escape character", nullptr));
        checkBoxFieldNames->setText(QCoreApplication::translate("CsvImportWidget", "First line has field names", nullptr));
        previewLayout->setTitle(QCoreApplication::translate("CsvImportWidget", "Preview", nullptr));
        labelSizeRowsCols->setText(QCoreApplication::translate("CsvImportWidget", "size, rows, columns", nullptr));
#if QT_CONFIG(accessibility)
        tableViewFields->setAccessibleName(QCoreApplication::translate("CsvImportWidget", "CSV import preview", nullptr));
#endif // QT_CONFIG(accessibility)
    } // retranslateUi

};

namespace Ui {
    class CsvImportWidget: public Ui_CsvImportWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CSVIMPORTWIDGET_H
