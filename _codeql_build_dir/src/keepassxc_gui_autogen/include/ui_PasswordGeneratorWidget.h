/********************************************************************************
** Form generated from reading UI file 'PasswordGeneratorWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PASSWORDGENERATORWIDGET_H
#define UI_PASSWORDGENERATORWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/PasswordWidget.h"

QT_BEGIN_NAMESPACE

class Ui_PasswordGeneratorWidget
{
public:
    QVBoxLayout *verticalLayout_2;
    QGridLayout *passwordFieldLayout;
    QHBoxLayout *passwordStrengthTextLayout;
    QLabel *strengthLabel;
    QSpacerItem *horizontalSpacer_2;
    QLabel *passwordLengthLabel;
    QSpacerItem *horizontalSpacer_6;
    QLabel *entropyLabel;
    PasswordWidget *editNewPassword;
    QProgressBar *entropyProgressBar;
    QPushButton *buttonGenerate;
    QPushButton *buttonCopy;
    QTabWidget *tabWidget;
    QWidget *passwordWidget;
    QGridLayout *_2;
    QHBoxLayout *passwordLengthSliderLayout;
    QLabel *labelLength;
    QSlider *sliderLength;
    QSpinBox *spinBoxLength;
    QPushButton *buttonAdvancedMode;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QVBoxLayout *verticalLayout_4;
    QGridLayout *characterButtons;
    QPushButton *checkBoxSpecialChars;
    QPushButton *checkBoxQuotes;
    QPushButton *checkBoxPunctuation;
    QPushButton *checkBoxDashes;
    QPushButton *checkBoxUpper;
    QPushButton *checkBoxNumbers;
    QPushButton *checkBoxLower;
    QPushButton *checkBoxMath;
    QPushButton *checkBoxExtASCII;
    QPushButton *checkBoxBraces;
    QSpacerItem *horizontalSpacer_3;
    QWidget *advancedContainer;
    QVBoxLayout *verticalLayout_3;
    QGridLayout *gridLayout;
    QLabel *labelExcludedChars;
    QLineEdit *editAdditionalChars;
    QPushButton *buttonAddHex;
    QLineEdit *editExcludedChars;
    QLabel *label;
    QCheckBox *checkBoxExcludeAlike;
    QCheckBox *checkBoxEnsureEvery;
    QSpacerItem *horizontalSpacer_8;
    QWidget *dicewareWidget;
    QGridLayout *gridLayout_2;
    QGridLayout *gridLayout_3;
    QLabel *labelWordList;
    QHBoxLayout *horizontalLayout_9;
    QLineEdit *editWordSeparator;
    QSpacerItem *horizontalSpacer_7;
    QLabel *labelWordCount;
    QLabel *wordCaseLabel;
    QHBoxLayout *horizontalLayout_10;
    QComboBox *comboBoxWordList;
    QPushButton *buttonDeleteWordList;
    QPushButton *buttonAddWordList;
    QHBoxLayout *horizontalLayout_3;
    QSlider *sliderWordCount;
    QSpinBox *spinBoxWordCount;
    QHBoxLayout *horizontalLayout_6;
    QComboBox *wordCaseComboBox;
    QSpacerItem *horizontalSpacer_4;
    QLabel *labelWordSeparator;
    QLabel *labelWordListWarning;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *buttonClose;
    QPushButton *buttonApply;
    QSpacerItem *verticalSpacer;
    QButtonGroup *optionButtons;

    void setupUi(QWidget *PasswordGeneratorWidget)
    {
        if (PasswordGeneratorWidget->objectName().isEmpty())
            PasswordGeneratorWidget->setObjectName(QString::fromUtf8("PasswordGeneratorWidget"));
        PasswordGeneratorWidget->resize(729, 433);
        verticalLayout_2 = new QVBoxLayout(PasswordGeneratorWidget);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        passwordFieldLayout = new QGridLayout();
        passwordFieldLayout->setObjectName(QString::fromUtf8("passwordFieldLayout"));
        passwordStrengthTextLayout = new QHBoxLayout();
        passwordStrengthTextLayout->setObjectName(QString::fromUtf8("passwordStrengthTextLayout"));
        strengthLabel = new QLabel(PasswordGeneratorWidget);
        strengthLabel->setObjectName(QString::fromUtf8("strengthLabel"));
        strengthLabel->setMinimumSize(QSize(70, 0));
        strengthLabel->setMaximumSize(QSize(16777215, 30));
        strengthLabel->setTextFormat(Qt::PlainText);
        strengthLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        strengthLabel->setMargin(3);

        passwordStrengthTextLayout->addWidget(strengthLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        passwordStrengthTextLayout->addItem(horizontalSpacer_2);

        passwordLengthLabel = new QLabel(PasswordGeneratorWidget);
        passwordLengthLabel->setObjectName(QString::fromUtf8("passwordLengthLabel"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(passwordLengthLabel->sizePolicy().hasHeightForWidth());
        passwordLengthLabel->setSizePolicy(sizePolicy);
        passwordLengthLabel->setMinimumSize(QSize(70, 0));
        passwordLengthLabel->setMaximumSize(QSize(16777215, 30));
        passwordLengthLabel->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);
        passwordLengthLabel->setMargin(3);

        passwordStrengthTextLayout->addWidget(passwordLengthLabel);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        passwordStrengthTextLayout->addItem(horizontalSpacer_6);

        entropyLabel = new QLabel(PasswordGeneratorWidget);
        entropyLabel->setObjectName(QString::fromUtf8("entropyLabel"));
        sizePolicy.setHeightForWidth(entropyLabel->sizePolicy().hasHeightForWidth());
        entropyLabel->setSizePolicy(sizePolicy);
        entropyLabel->setMinimumSize(QSize(70, 0));
        entropyLabel->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);
        entropyLabel->setMargin(3);

        passwordStrengthTextLayout->addWidget(entropyLabel);


        passwordFieldLayout->addLayout(passwordStrengthTextLayout, 2, 0, 1, 1);

        editNewPassword = new PasswordWidget(PasswordGeneratorWidget);
        editNewPassword->setObjectName(QString::fromUtf8("editNewPassword"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(editNewPassword->sizePolicy().hasHeightForWidth());
        editNewPassword->setSizePolicy(sizePolicy1);
        editNewPassword->setMinimumSize(QSize(450, 0));
        editNewPassword->setFocusPolicy(Qt::StrongFocus);

        passwordFieldLayout->addWidget(editNewPassword, 0, 0, 1, 1);

        entropyProgressBar = new QProgressBar(PasswordGeneratorWidget);
        entropyProgressBar->setObjectName(QString::fromUtf8("entropyProgressBar"));
        entropyProgressBar->setMinimumSize(QSize(50, 5));
        entropyProgressBar->setMaximumSize(QSize(16777215, 5));
        entropyProgressBar->setStyleSheet(QString::fromUtf8("QProgressBar {\n"
"	border: none;\n"
"	height: 2px;\n"
"    font-size: 1px;\n"
"	background-color: transparent;\n"
"    padding: 0 1px;\n"
"}\n"
"QProgressBar::chunk {\n"
"	background-color: #c0392b;\n"
"	border-radius: 2px;\n"
"}"));
        entropyProgressBar->setMaximum(200);
        entropyProgressBar->setValue(100);
        entropyProgressBar->setTextVisible(false);
        entropyProgressBar->setOrientation(Qt::Horizontal);
        entropyProgressBar->setInvertedAppearance(false);
        entropyProgressBar->setTextDirection(QProgressBar::TopToBottom);

        passwordFieldLayout->addWidget(entropyProgressBar, 1, 0, 1, 1);

        buttonGenerate = new QPushButton(PasswordGeneratorWidget);
        buttonGenerate->setObjectName(QString::fromUtf8("buttonGenerate"));
        buttonGenerate->setFocusPolicy(Qt::TabFocus);
#if QT_CONFIG(shortcut)
        buttonGenerate->setShortcut(QString::fromUtf8("Ctrl+R"));
#endif // QT_CONFIG(shortcut)

        passwordFieldLayout->addWidget(buttonGenerate, 0, 1, 1, 1);

        buttonCopy = new QPushButton(PasswordGeneratorWidget);
        buttonCopy->setObjectName(QString::fromUtf8("buttonCopy"));
        buttonCopy->setFocusPolicy(Qt::TabFocus);
#if QT_CONFIG(shortcut)
        buttonCopy->setShortcut(QString::fromUtf8("Ctrl+C"));
#endif // QT_CONFIG(shortcut)

        passwordFieldLayout->addWidget(buttonCopy, 0, 2, 1, 1);


        verticalLayout_2->addLayout(passwordFieldLayout);

        tabWidget = new QTabWidget(PasswordGeneratorWidget);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Minimum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());
        tabWidget->setSizePolicy(sizePolicy2);
        tabWidget->setTabPosition(QTabWidget::North);
        passwordWidget = new QWidget();
        passwordWidget->setObjectName(QString::fromUtf8("passwordWidget"));
        _2 = new QGridLayout(passwordWidget);
        _2->setObjectName(QString::fromUtf8("_2"));
        _2->setVerticalSpacing(10);
        passwordLengthSliderLayout = new QHBoxLayout();
        passwordLengthSliderLayout->setSpacing(15);
        passwordLengthSliderLayout->setObjectName(QString::fromUtf8("passwordLengthSliderLayout"));
        passwordLengthSliderLayout->setSizeConstraint(QLayout::SetMinimumSize);
        passwordLengthSliderLayout->setContentsMargins(-1, 0, -1, -1);
        labelLength = new QLabel(passwordWidget);
        labelLength->setObjectName(QString::fromUtf8("labelLength"));

        passwordLengthSliderLayout->addWidget(labelLength);

        sliderLength = new QSlider(passwordWidget);
        sliderLength->setObjectName(QString::fromUtf8("sliderLength"));
        sliderLength->setMinimum(1);
        sliderLength->setMaximum(128);
        sliderLength->setSliderPosition(20);
        sliderLength->setOrientation(Qt::Horizontal);
        sliderLength->setTickPosition(QSlider::TicksBelow);
        sliderLength->setTickInterval(8);

        passwordLengthSliderLayout->addWidget(sliderLength);

        spinBoxLength = new QSpinBox(passwordWidget);
        spinBoxLength->setObjectName(QString::fromUtf8("spinBoxLength"));
        spinBoxLength->setMinimum(1);
        spinBoxLength->setMaximum(999);
        spinBoxLength->setValue(20);

        passwordLengthSliderLayout->addWidget(spinBoxLength);

        buttonAdvancedMode = new QPushButton(passwordWidget);
        optionButtons = new QButtonGroup(PasswordGeneratorWidget);
        optionButtons->setObjectName(QString::fromUtf8("optionButtons"));
        optionButtons->setExclusive(false);
        optionButtons->addButton(buttonAdvancedMode);
        buttonAdvancedMode->setObjectName(QString::fromUtf8("buttonAdvancedMode"));
        buttonAdvancedMode->setFocusPolicy(Qt::TabFocus);
        buttonAdvancedMode->setCheckable(true);

        passwordLengthSliderLayout->addWidget(buttonAdvancedMode);


        _2->addLayout(passwordLengthSliderLayout, 0, 0, 1, 1);

        groupBox = new QGroupBox(passwordWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setMinimumSize(QSize(580, 0));
        horizontalLayout = new QHBoxLayout(groupBox);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        characterButtons = new QGridLayout();
        characterButtons->setObjectName(QString::fromUtf8("characterButtons"));
        characterButtons->setContentsMargins(-1, -1, -1, 6);
        checkBoxSpecialChars = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxSpecialChars);
        checkBoxSpecialChars->setObjectName(QString::fromUtf8("checkBoxSpecialChars"));
        checkBoxSpecialChars->setEnabled(true);
        checkBoxSpecialChars->setFocusPolicy(Qt::TabFocus);
        checkBoxSpecialChars->setText(QString::fromUtf8("/ * + && \342\200\246"));
        checkBoxSpecialChars->setCheckable(true);

        characterButtons->addWidget(checkBoxSpecialChars, 0, 3, 1, 1);

        checkBoxQuotes = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxQuotes);
        checkBoxQuotes->setObjectName(QString::fromUtf8("checkBoxQuotes"));
        checkBoxQuotes->setFocusPolicy(Qt::TabFocus);
        checkBoxQuotes->setText(QString::fromUtf8("\" '"));
        checkBoxQuotes->setCheckable(true);

        characterButtons->addWidget(checkBoxQuotes, 1, 1, 1, 1);

        checkBoxPunctuation = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxPunctuation);
        checkBoxPunctuation->setObjectName(QString::fromUtf8("checkBoxPunctuation"));
        checkBoxPunctuation->setFocusPolicy(Qt::TabFocus);
        checkBoxPunctuation->setText(QString::fromUtf8(". , : ;"));
        checkBoxPunctuation->setCheckable(true);

        characterButtons->addWidget(checkBoxPunctuation, 1, 0, 1, 1);

        checkBoxDashes = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxDashes);
        checkBoxDashes->setObjectName(QString::fromUtf8("checkBoxDashes"));
        checkBoxDashes->setFocusPolicy(Qt::TabFocus);
        checkBoxDashes->setText(QString::fromUtf8("\\ / | _ -"));
        checkBoxDashes->setCheckable(true);

        characterButtons->addWidget(checkBoxDashes, 1, 2, 1, 1);

        checkBoxUpper = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxUpper);
        checkBoxUpper->setObjectName(QString::fromUtf8("checkBoxUpper"));
        checkBoxUpper->setFocusPolicy(Qt::TabFocus);
        checkBoxUpper->setText(QString::fromUtf8("A-Z"));
        checkBoxUpper->setCheckable(true);

        characterButtons->addWidget(checkBoxUpper, 0, 0, 1, 1);

        checkBoxNumbers = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxNumbers);
        checkBoxNumbers->setObjectName(QString::fromUtf8("checkBoxNumbers"));
        checkBoxNumbers->setFocusPolicy(Qt::TabFocus);
        checkBoxNumbers->setText(QString::fromUtf8("0-9"));
        checkBoxNumbers->setCheckable(true);

        characterButtons->addWidget(checkBoxNumbers, 0, 2, 1, 1);

        checkBoxLower = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxLower);
        checkBoxLower->setObjectName(QString::fromUtf8("checkBoxLower"));
        checkBoxLower->setFocusPolicy(Qt::TabFocus);
        checkBoxLower->setText(QString::fromUtf8("a-z"));
        checkBoxLower->setCheckable(true);

        characterButtons->addWidget(checkBoxLower, 0, 1, 1, 1);

        checkBoxMath = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxMath);
        checkBoxMath->setObjectName(QString::fromUtf8("checkBoxMath"));
        checkBoxMath->setFocusPolicy(Qt::TabFocus);
        checkBoxMath->setText(QString::fromUtf8("< > * + ! ? ="));
        checkBoxMath->setCheckable(true);

        characterButtons->addWidget(checkBoxMath, 1, 3, 1, 1);

        checkBoxExtASCII = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxExtASCII);
        checkBoxExtASCII->setObjectName(QString::fromUtf8("checkBoxExtASCII"));
        checkBoxExtASCII->setFocusPolicy(Qt::TabFocus);
        checkBoxExtASCII->setCheckable(true);

        characterButtons->addWidget(checkBoxExtASCII, 0, 4, 1, 1);

        checkBoxBraces = new QPushButton(groupBox);
        optionButtons->addButton(checkBoxBraces);
        checkBoxBraces->setObjectName(QString::fromUtf8("checkBoxBraces"));
        checkBoxBraces->setFocusPolicy(Qt::TabFocus);
        checkBoxBraces->setText(QString::fromUtf8("{ [ ( ) ] }"));
        checkBoxBraces->setCheckable(true);

        characterButtons->addWidget(checkBoxBraces, 1, 4, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        characterButtons->addItem(horizontalSpacer_3, 0, 5, 1, 1);


        verticalLayout_4->addLayout(characterButtons);

        advancedContainer = new QWidget(groupBox);
        advancedContainer->setObjectName(QString::fromUtf8("advancedContainer"));
        verticalLayout_3 = new QVBoxLayout(advancedContainer);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(-1, -1, -1, 6);
        labelExcludedChars = new QLabel(advancedContainer);
        labelExcludedChars->setObjectName(QString::fromUtf8("labelExcludedChars"));

        gridLayout->addWidget(labelExcludedChars, 1, 0, 1, 1);

        editAdditionalChars = new QLineEdit(advancedContainer);
        editAdditionalChars->setObjectName(QString::fromUtf8("editAdditionalChars"));
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(editAdditionalChars->sizePolicy().hasHeightForWidth());
        editAdditionalChars->setSizePolicy(sizePolicy3);
        editAdditionalChars->setMinimumSize(QSize(375, 0));
        editAdditionalChars->setClearButtonEnabled(true);

        gridLayout->addWidget(editAdditionalChars, 0, 1, 1, 1);

        buttonAddHex = new QPushButton(advancedContainer);
        buttonAddHex->setObjectName(QString::fromUtf8("buttonAddHex"));
        buttonAddHex->setFocusPolicy(Qt::TabFocus);

        gridLayout->addWidget(buttonAddHex, 1, 2, 1, 1);

        editExcludedChars = new QLineEdit(advancedContainer);
        editExcludedChars->setObjectName(QString::fromUtf8("editExcludedChars"));
        QSizePolicy sizePolicy4(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(editExcludedChars->sizePolicy().hasHeightForWidth());
        editExcludedChars->setSizePolicy(sizePolicy4);
        editExcludedChars->setMinimumSize(QSize(375, 0));
        editExcludedChars->setClearButtonEnabled(true);

        gridLayout->addWidget(editExcludedChars, 1, 1, 1, 1);

        label = new QLabel(advancedContainer);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);


        verticalLayout_3->addLayout(gridLayout);

        checkBoxExcludeAlike = new QCheckBox(advancedContainer);
        optionButtons->addButton(checkBoxExcludeAlike);
        checkBoxExcludeAlike->setObjectName(QString::fromUtf8("checkBoxExcludeAlike"));

        verticalLayout_3->addWidget(checkBoxExcludeAlike);

        checkBoxEnsureEvery = new QCheckBox(advancedContainer);
        optionButtons->addButton(checkBoxEnsureEvery);
        checkBoxEnsureEvery->setObjectName(QString::fromUtf8("checkBoxEnsureEvery"));

        verticalLayout_3->addWidget(checkBoxEnsureEvery);


        verticalLayout_4->addWidget(advancedContainer);


        horizontalLayout->addLayout(verticalLayout_4);

        horizontalSpacer_8 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);


        _2->addWidget(groupBox, 1, 0, 1, 1);

        tabWidget->addTab(passwordWidget, QString());
        dicewareWidget = new QWidget();
        dicewareWidget->setObjectName(QString::fromUtf8("dicewareWidget"));
        gridLayout_2 = new QGridLayout(dicewareWidget);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_3 = new QGridLayout();
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        labelWordList = new QLabel(dicewareWidget);
        labelWordList->setObjectName(QString::fromUtf8("labelWordList"));
        QSizePolicy sizePolicy5(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(labelWordList->sizePolicy().hasHeightForWidth());
        labelWordList->setSizePolicy(sizePolicy5);

        gridLayout_3->addWidget(labelWordList, 1, 1, 1, 1, Qt::AlignRight);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        editWordSeparator = new QLineEdit(dicewareWidget);
        editWordSeparator->setObjectName(QString::fromUtf8("editWordSeparator"));
        sizePolicy3.setHeightForWidth(editWordSeparator->sizePolicy().hasHeightForWidth());
        editWordSeparator->setSizePolicy(sizePolicy3);
        editWordSeparator->setMinimumSize(QSize(20, 0));

        horizontalLayout_9->addWidget(editWordSeparator);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_7);


        gridLayout_3->addLayout(horizontalLayout_9, 3, 2, 1, 1);

        labelWordCount = new QLabel(dicewareWidget);
        labelWordCount->setObjectName(QString::fromUtf8("labelWordCount"));

        gridLayout_3->addWidget(labelWordCount, 2, 1, 1, 1, Qt::AlignRight);

        wordCaseLabel = new QLabel(dicewareWidget);
        wordCaseLabel->setObjectName(QString::fromUtf8("wordCaseLabel"));

        gridLayout_3->addWidget(wordCaseLabel, 4, 1, 1, 1, Qt::AlignRight);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        comboBoxWordList = new QComboBox(dicewareWidget);
        comboBoxWordList->setObjectName(QString::fromUtf8("comboBoxWordList"));
        QSizePolicy sizePolicy6(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(comboBoxWordList->sizePolicy().hasHeightForWidth());
        comboBoxWordList->setSizePolicy(sizePolicy6);

        horizontalLayout_10->addWidget(comboBoxWordList);

        buttonDeleteWordList = new QPushButton(dicewareWidget);
        buttonDeleteWordList->setObjectName(QString::fromUtf8("buttonDeleteWordList"));
        buttonDeleteWordList->setFocusPolicy(Qt::TabFocus);

        horizontalLayout_10->addWidget(buttonDeleteWordList);

        buttonAddWordList = new QPushButton(dicewareWidget);
        buttonAddWordList->setObjectName(QString::fromUtf8("buttonAddWordList"));
        buttonAddWordList->setFocusPolicy(Qt::TabFocus);

        horizontalLayout_10->addWidget(buttonAddWordList);


        gridLayout_3->addLayout(horizontalLayout_10, 1, 2, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setSizeConstraint(QLayout::SetMinimumSize);
        sliderWordCount = new QSlider(dicewareWidget);
        sliderWordCount->setObjectName(QString::fromUtf8("sliderWordCount"));
        sliderWordCount->setMinimum(1);
        sliderWordCount->setMaximum(40);
        sliderWordCount->setValue(6);
        sliderWordCount->setSliderPosition(6);
        sliderWordCount->setOrientation(Qt::Horizontal);
        sliderWordCount->setTickPosition(QSlider::TicksBelow);
        sliderWordCount->setTickInterval(8);

        horizontalLayout_3->addWidget(sliderWordCount);

        spinBoxWordCount = new QSpinBox(dicewareWidget);
        spinBoxWordCount->setObjectName(QString::fromUtf8("spinBoxWordCount"));
        spinBoxWordCount->setMinimum(1);
        spinBoxWordCount->setMaximum(100);
        spinBoxWordCount->setValue(6);

        horizontalLayout_3->addWidget(spinBoxWordCount);


        gridLayout_3->addLayout(horizontalLayout_3, 2, 2, 1, 1);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        wordCaseComboBox = new QComboBox(dicewareWidget);
        wordCaseComboBox->setObjectName(QString::fromUtf8("wordCaseComboBox"));

        horizontalLayout_6->addWidget(wordCaseComboBox);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_4);


        gridLayout_3->addLayout(horizontalLayout_6, 4, 2, 1, 1);

        labelWordSeparator = new QLabel(dicewareWidget);
        labelWordSeparator->setObjectName(QString::fromUtf8("labelWordSeparator"));

        gridLayout_3->addWidget(labelWordSeparator, 3, 1, 1, 1, Qt::AlignRight);

        labelWordListWarning = new QLabel(dicewareWidget);
        labelWordListWarning->setObjectName(QString::fromUtf8("labelWordListWarning"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        labelWordListWarning->setFont(font);

        gridLayout_3->addWidget(labelWordListWarning, 0, 2, 1, 1);


        gridLayout_2->addLayout(gridLayout_3, 0, 0, 1, 1);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(verticalSpacer_3, 1, 0, 1, 1);

        tabWidget->addTab(dicewareWidget, QString());

        verticalLayout_2->addWidget(tabWidget);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalLayout_4->setContentsMargins(0, 0, -1, -1);
        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_5);

        buttonClose = new QPushButton(PasswordGeneratorWidget);
        buttonClose->setObjectName(QString::fromUtf8("buttonClose"));

        horizontalLayout_4->addWidget(buttonClose);

        buttonApply = new QPushButton(PasswordGeneratorWidget);
        buttonApply->setObjectName(QString::fromUtf8("buttonApply"));

        horizontalLayout_4->addWidget(buttonApply);


        verticalLayout_2->addLayout(horizontalLayout_4);

        verticalSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

#if QT_CONFIG(shortcut)
        labelLength->setBuddy(spinBoxLength);
        labelWordCount->setBuddy(spinBoxLength);
#endif // QT_CONFIG(shortcut)
        QWidget::setTabOrder(editNewPassword, buttonGenerate);
        QWidget::setTabOrder(buttonGenerate, buttonCopy);
        QWidget::setTabOrder(buttonCopy, tabWidget);
        QWidget::setTabOrder(tabWidget, sliderLength);
        QWidget::setTabOrder(sliderLength, spinBoxLength);
        QWidget::setTabOrder(spinBoxLength, buttonAdvancedMode);
        QWidget::setTabOrder(buttonAdvancedMode, checkBoxUpper);
        QWidget::setTabOrder(checkBoxUpper, checkBoxLower);
        QWidget::setTabOrder(checkBoxLower, checkBoxNumbers);
        QWidget::setTabOrder(checkBoxNumbers, checkBoxSpecialChars);
        QWidget::setTabOrder(checkBoxSpecialChars, checkBoxExtASCII);
        QWidget::setTabOrder(checkBoxExtASCII, checkBoxPunctuation);
        QWidget::setTabOrder(checkBoxPunctuation, checkBoxQuotes);
        QWidget::setTabOrder(checkBoxQuotes, checkBoxDashes);
        QWidget::setTabOrder(checkBoxDashes, checkBoxMath);
        QWidget::setTabOrder(checkBoxMath, checkBoxBraces);
        QWidget::setTabOrder(checkBoxBraces, editAdditionalChars);
        QWidget::setTabOrder(editAdditionalChars, editExcludedChars);
        QWidget::setTabOrder(editExcludedChars, buttonAddHex);
        QWidget::setTabOrder(buttonAddHex, checkBoxExcludeAlike);
        QWidget::setTabOrder(checkBoxExcludeAlike, checkBoxEnsureEvery);
        QWidget::setTabOrder(checkBoxEnsureEvery, comboBoxWordList);
        QWidget::setTabOrder(comboBoxWordList, buttonDeleteWordList);
        QWidget::setTabOrder(buttonDeleteWordList, buttonAddWordList);
        QWidget::setTabOrder(buttonAddWordList, sliderWordCount);
        QWidget::setTabOrder(sliderWordCount, spinBoxWordCount);
        QWidget::setTabOrder(spinBoxWordCount, editWordSeparator);
        QWidget::setTabOrder(editWordSeparator, wordCaseComboBox);
        QWidget::setTabOrder(wordCaseComboBox, buttonClose);
        QWidget::setTabOrder(buttonClose, buttonApply);

        retranslateUi(PasswordGeneratorWidget);

        tabWidget->setCurrentIndex(0);
        buttonApply->setDefault(true);


        QMetaObject::connectSlotsByName(PasswordGeneratorWidget);
    } // setupUi

    void retranslateUi(QWidget *PasswordGeneratorWidget)
    {
        PasswordGeneratorWidget->setWindowTitle(QCoreApplication::translate("PasswordGeneratorWidget", "Generate Password", nullptr));
        strengthLabel->setText(QCoreApplication::translate("PasswordGeneratorWidget", "strength", "Password strength"));
        passwordLengthLabel->setText(QCoreApplication::translate("PasswordGeneratorWidget", "passwordLength", nullptr));
        entropyLabel->setText(QCoreApplication::translate("PasswordGeneratorWidget", "entropy", nullptr));
#if QT_CONFIG(accessibility)
        editNewPassword->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Generated password", nullptr));
#endif // QT_CONFIG(accessibility)
        entropyProgressBar->setFormat(QCoreApplication::translate("PasswordGeneratorWidget", "%p%", nullptr));
#if QT_CONFIG(accessibility)
        buttonGenerate->setAccessibleDescription(QCoreApplication::translate("PasswordGeneratorWidget", "Regenerate password", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        buttonCopy->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Copy password", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        buttonCopy->setAccessibleDescription(QCoreApplication::translate("PasswordGeneratorWidget", "Copy password", nullptr));
#endif // QT_CONFIG(accessibility)
        labelLength->setText(QCoreApplication::translate("PasswordGeneratorWidget", "&Length:", nullptr));
#if QT_CONFIG(accessibility)
        sliderLength->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Password length", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(accessibility)
        spinBoxLength->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Password length", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        buttonAdvancedMode->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Switch to advanced mode", nullptr));
#endif // QT_CONFIG(tooltip)
        buttonAdvancedMode->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Advanced", nullptr));
        groupBox->setTitle(QCoreApplication::translate("PasswordGeneratorWidget", "Character Types", nullptr));
#if QT_CONFIG(tooltip)
        checkBoxSpecialChars->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Special characters", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxSpecialChars->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Special characters", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxQuotes->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Quotes", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxQuotes->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Quotes", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxPunctuation->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Punctuation", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxPunctuation->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Punctuation", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxDashes->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Dashes and Slashes", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxDashes->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Dashes and Slashes", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxUpper->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Upper-case letters", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxUpper->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Upper-case letters", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxNumbers->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Numbers", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxNumbers->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Numbers", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxLower->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Lower-case letters", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxLower->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Lower-case letters", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxMath->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Math Symbols", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxMath->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Math Symbols", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        checkBoxExtASCII->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Extended ASCII", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxExtASCII->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Extended ASCII", nullptr));
#endif // QT_CONFIG(accessibility)
        checkBoxExtASCII->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Extended ASCII", nullptr));
#if QT_CONFIG(tooltip)
        checkBoxBraces->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Braces", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxBraces->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Braces", nullptr));
#endif // QT_CONFIG(accessibility)
        labelExcludedChars->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Do not include:", nullptr));
#if QT_CONFIG(tooltip)
        editAdditionalChars->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Additional characters to use for the generated password", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        editAdditionalChars->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Additional characters", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        buttonAddHex->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Add non-hex letters to \"do not include\" list", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        buttonAddHex->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Hex Passwords", nullptr));
#endif // QT_CONFIG(accessibility)
        buttonAddHex->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Hex", nullptr));
#if QT_CONFIG(tooltip)
        editExcludedChars->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Character set to exclude from generated password", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        editExcludedChars->setAccessibleName(QCoreApplication::translate("PasswordGeneratorWidget", "Excluded characters", nullptr));
#endif // QT_CONFIG(accessibility)
        label->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Also choose from:", nullptr));
#if QT_CONFIG(tooltip)
        checkBoxExcludeAlike->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Excluded characters: \"0\", \"1\", \"l\", \"I\", \"O\", \"|\", \"\357\271\222\", \"B\", \"8\", \"G\", \"6\"", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        checkBoxExcludeAlike->setAccessibleDescription(QCoreApplication::translate("PasswordGeneratorWidget", "Excluded characters: \"0\", \"1\", \"l\", \"I\", \"O\", \"|\", \"\357\271\222\", \"B\", \"8\", \"G\", \"6\"", nullptr));
#endif // QT_CONFIG(accessibility)
        checkBoxExcludeAlike->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Exclude look-alike characters", nullptr));
        checkBoxEnsureEvery->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Pick characters from every group", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(passwordWidget), QCoreApplication::translate("PasswordGeneratorWidget", "Password", nullptr));
        labelWordList->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Wordlist:", nullptr));
        editWordSeparator->setText(QString());
        labelWordCount->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Word Count:", nullptr));
        wordCaseLabel->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Word Case:", nullptr));
#if QT_CONFIG(tooltip)
        buttonDeleteWordList->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Delete selected wordlist", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        buttonDeleteWordList->setAccessibleDescription(QCoreApplication::translate("PasswordGeneratorWidget", "Delete selected wordlist", nullptr));
#endif // QT_CONFIG(accessibility)
#if QT_CONFIG(tooltip)
        buttonAddWordList->setToolTip(QCoreApplication::translate("PasswordGeneratorWidget", "Add custom wordlist", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        buttonAddWordList->setAccessibleDescription(QCoreApplication::translate("PasswordGeneratorWidget", "Add custom wordlist", nullptr));
#endif // QT_CONFIG(accessibility)
        labelWordSeparator->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Word Separator:", nullptr));
        labelWordListWarning->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Warning: the chosen wordlist is smaller than the minimum recommended size!", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(dicewareWidget), QCoreApplication::translate("PasswordGeneratorWidget", "Passphrase", nullptr));
        buttonClose->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Close", nullptr));
#if QT_CONFIG(shortcut)
        buttonClose->setShortcut(QCoreApplication::translate("PasswordGeneratorWidget", "Esc", nullptr));
#endif // QT_CONFIG(shortcut)
        buttonApply->setText(QCoreApplication::translate("PasswordGeneratorWidget", "Apply Password", nullptr));
    } // retranslateUi

};

namespace Ui {
    class PasswordGeneratorWidget: public Ui_PasswordGeneratorWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PASSWORDGENERATORWIDGET_H
