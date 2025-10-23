/********************************************************************************
** Form generated from reading UI file 'DatabaseSettingsWidgetEncryption.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DATABASESETTINGSWIDGETENCRYPTION_H
#define UI_DATABASESETTINGSWIDGETENCRYPTION_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DatabaseSettingsWidgetEncryption
{
public:
    QVBoxLayout *verticalLayout_3;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout;
    QFormLayout *formLayout;
    QLabel *compatibilityLabel;
    QComboBox *compatibilitySelection;
    QLabel *formatCannotBeChanged;
    QSpacerItem *verticalSpacer_2;
    QLabel *label;
    QTabWidget *encryptionSettingsTabWidget;
    QWidget *basicTab;
    QVBoxLayout *verticalLayout_8;
    QHBoxLayout *horizontalLayout_2;
    QLabel *decryptionTimeLabel;
    QLabel *decryptionTimeValueLabel;
    QSpacerItem *horizontalSpacer_2;
    QWidget *decryptionTimeSettings;
    QVBoxLayout *decryptionTimeSliderLayout;
    QSlider *decryptionTimeSlider;
    QHBoxLayout *horizontalLayout;
    QLabel *minTimeLabel;
    QSpacerItem *horizontalSpacer;
    QLabel *maxTimeLabel;
    QLabel *label_2;
    QSpacerItem *verticalSpacer_3;
    QWidget *advancedTab;
    QGridLayout *gridLayout;
    QSpacerItem *horizontalSpacer_4;
    QSpinBox *parallelismSpinBox;
    QLabel *parallelismLabel;
    QSpinBox *memorySpinBox;
    QLabel *memoryUsageLabel;
    QHBoxLayout *horizontalLayout_3;
    QSpinBox *transformRoundsSpinBox;
    QToolButton *transformBenchmarkButton;
    QSpacerItem *horizontalSpacer_3;
    QLabel *transformRoundsLabel;
    QComboBox *kdfComboBox;
    QLabel *kdfLabel;
    QComboBox *algorithmComboBox;
    QLabel *algorithmLabel;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *DatabaseSettingsWidgetEncryption)
    {
        if (DatabaseSettingsWidgetEncryption->objectName().isEmpty())
            DatabaseSettingsWidgetEncryption->setObjectName(QString::fromUtf8("DatabaseSettingsWidgetEncryption"));
        DatabaseSettingsWidgetEncryption->resize(451, 329);
        verticalLayout_3 = new QVBoxLayout(DatabaseSettingsWidgetEncryption);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        scrollArea = new QScrollArea(DatabaseSettingsWidgetEncryption);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setFrameShadow(QFrame::Plain);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 433, 311));
        verticalLayout = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 10, 0, 0);
        formLayout = new QFormLayout();
        formLayout->setObjectName(QString::fromUtf8("formLayout"));
        formLayout->setRowWrapPolicy(QFormLayout::DontWrapRows);
        formLayout->setVerticalSpacing(2);
        compatibilityLabel = new QLabel(scrollAreaWidgetContents);
        compatibilityLabel->setObjectName(QString::fromUtf8("compatibilityLabel"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        compatibilityLabel->setFont(font);

        formLayout->setWidget(0, QFormLayout::LabelRole, compatibilityLabel);

        compatibilitySelection = new QComboBox(scrollAreaWidgetContents);
        compatibilitySelection->setObjectName(QString::fromUtf8("compatibilitySelection"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(compatibilitySelection->sizePolicy().hasHeightForWidth());
        compatibilitySelection->setSizePolicy(sizePolicy);

        formLayout->setWidget(0, QFormLayout::FieldRole, compatibilitySelection);

        formatCannotBeChanged = new QLabel(scrollAreaWidgetContents);
        formatCannotBeChanged->setObjectName(QString::fromUtf8("formatCannotBeChanged"));
        QFont font1;
        font1.setItalic(true);
        formatCannotBeChanged->setFont(font1);
        formatCannotBeChanged->setWordWrap(true);

        formLayout->setWidget(1, QFormLayout::FieldRole, formatCannotBeChanged);


        verticalLayout->addLayout(formLayout);

        verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout->addItem(verticalSpacer_2);

        label = new QLabel(scrollAreaWidgetContents);
        label->setObjectName(QString::fromUtf8("label"));
        label->setFont(font);

        verticalLayout->addWidget(label);

        encryptionSettingsTabWidget = new QTabWidget(scrollAreaWidgetContents);
        encryptionSettingsTabWidget->setObjectName(QString::fromUtf8("encryptionSettingsTabWidget"));
        encryptionSettingsTabWidget->setTabPosition(QTabWidget::North);
        encryptionSettingsTabWidget->setTabShape(QTabWidget::Rounded);
        basicTab = new QWidget();
        basicTab->setObjectName(QString::fromUtf8("basicTab"));
        verticalLayout_8 = new QVBoxLayout(basicTab);
        verticalLayout_8->setSpacing(6);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        verticalLayout_8->setContentsMargins(9, 6, 9, 6);
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        decryptionTimeLabel = new QLabel(basicTab);
        decryptionTimeLabel->setObjectName(QString::fromUtf8("decryptionTimeLabel"));
        decryptionTimeLabel->setFont(font);

        horizontalLayout_2->addWidget(decryptionTimeLabel);

        decryptionTimeValueLabel = new QLabel(basicTab);
        decryptionTimeValueLabel->setObjectName(QString::fromUtf8("decryptionTimeValueLabel"));
        QFont font2;
        font2.setBold(false);
        font2.setWeight(50);
        decryptionTimeValueLabel->setFont(font2);
        decryptionTimeValueLabel->setText(QString::fromUtf8("?? s"));

        horizontalLayout_2->addWidget(decryptionTimeValueLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout_8->addLayout(horizontalLayout_2);

        decryptionTimeSettings = new QWidget(basicTab);
        decryptionTimeSettings->setObjectName(QString::fromUtf8("decryptionTimeSettings"));
        decryptionTimeSliderLayout = new QVBoxLayout(decryptionTimeSettings);
        decryptionTimeSliderLayout->setObjectName(QString::fromUtf8("decryptionTimeSliderLayout"));
        decryptionTimeSliderLayout->setContentsMargins(6, 0, 6, 9);
        decryptionTimeSlider = new QSlider(decryptionTimeSettings);
        decryptionTimeSlider->setObjectName(QString::fromUtf8("decryptionTimeSlider"));
        decryptionTimeSlider->setSingleStep(1);
        decryptionTimeSlider->setPageStep(10);
        decryptionTimeSlider->setValue(10);
        decryptionTimeSlider->setOrientation(Qt::Horizontal);
        decryptionTimeSlider->setTickPosition(QSlider::TicksBelow);
        decryptionTimeSlider->setTickInterval(5);

        decryptionTimeSliderLayout->addWidget(decryptionTimeSlider);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        minTimeLabel = new QLabel(decryptionTimeSettings);
        minTimeLabel->setObjectName(QString::fromUtf8("minTimeLabel"));
        minTimeLabel->setText(QString::fromUtf8("?? ms"));

        horizontalLayout->addWidget(minTimeLabel);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        maxTimeLabel = new QLabel(decryptionTimeSettings);
        maxTimeLabel->setObjectName(QString::fromUtf8("maxTimeLabel"));
        maxTimeLabel->setText(QString::fromUtf8("? s"));

        horizontalLayout->addWidget(maxTimeLabel);


        decryptionTimeSliderLayout->addLayout(horizontalLayout);

        label_2 = new QLabel(decryptionTimeSettings);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy1);
        label_2->setWordWrap(true);

        decryptionTimeSliderLayout->addWidget(label_2);

        verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        decryptionTimeSliderLayout->addItem(verticalSpacer_3);


        verticalLayout_8->addWidget(decryptionTimeSettings);

        encryptionSettingsTabWidget->addTab(basicTab, QString());
        advancedTab = new QWidget();
        advancedTab->setObjectName(QString::fromUtf8("advancedTab"));
        gridLayout = new QGridLayout(advancedTab);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(-1, 6, -1, 6);
        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_4, 0, 2, 1, 1);

        parallelismSpinBox = new QSpinBox(advancedTab);
        parallelismSpinBox->setObjectName(QString::fromUtf8("parallelismSpinBox"));
        parallelismSpinBox->setMinimumSize(QSize(150, 0));
        parallelismSpinBox->setMaximumSize(QSize(150, 16777215));
        parallelismSpinBox->setMinimum(1);
        parallelismSpinBox->setMaximum(128);

        gridLayout->addWidget(parallelismSpinBox, 4, 1, 1, 1);

        parallelismLabel = new QLabel(advancedTab);
        parallelismLabel->setObjectName(QString::fromUtf8("parallelismLabel"));

        gridLayout->addWidget(parallelismLabel, 4, 0, 1, 1);

        memorySpinBox = new QSpinBox(advancedTab);
        memorySpinBox->setObjectName(QString::fromUtf8("memorySpinBox"));
        memorySpinBox->setMinimumSize(QSize(150, 0));
        memorySpinBox->setMaximumSize(QSize(150, 16777215));
        memorySpinBox->setMinimum(1);
        memorySpinBox->setMaximum(1048576);

        gridLayout->addWidget(memorySpinBox, 3, 1, 1, 1);

        memoryUsageLabel = new QLabel(advancedTab);
        memoryUsageLabel->setObjectName(QString::fromUtf8("memoryUsageLabel"));

        gridLayout->addWidget(memoryUsageLabel, 3, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        transformRoundsSpinBox = new QSpinBox(advancedTab);
        transformRoundsSpinBox->setObjectName(QString::fromUtf8("transformRoundsSpinBox"));
        transformRoundsSpinBox->setMinimumSize(QSize(150, 0));
        transformRoundsSpinBox->setMaximumSize(QSize(150, 16777215));
        transformRoundsSpinBox->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        transformRoundsSpinBox->setMinimum(1);
        transformRoundsSpinBox->setMaximum(1000000000);

        horizontalLayout_3->addWidget(transformRoundsSpinBox);

        transformBenchmarkButton = new QToolButton(advancedTab);
        transformBenchmarkButton->setObjectName(QString::fromUtf8("transformBenchmarkButton"));
        transformBenchmarkButton->setFocusPolicy(Qt::WheelFocus);

        horizontalLayout_3->addWidget(transformBenchmarkButton);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);

        horizontalLayout_3->setStretch(0, 40);
        horizontalLayout_3->setStretch(1, 40);

        gridLayout->addLayout(horizontalLayout_3, 2, 1, 1, 1);

        transformRoundsLabel = new QLabel(advancedTab);
        transformRoundsLabel->setObjectName(QString::fromUtf8("transformRoundsLabel"));

        gridLayout->addWidget(transformRoundsLabel, 2, 0, 1, 1);

        kdfComboBox = new QComboBox(advancedTab);
        kdfComboBox->setObjectName(QString::fromUtf8("kdfComboBox"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(kdfComboBox->sizePolicy().hasHeightForWidth());
        kdfComboBox->setSizePolicy(sizePolicy2);

        gridLayout->addWidget(kdfComboBox, 1, 1, 1, 1);

        kdfLabel = new QLabel(advancedTab);
        kdfLabel->setObjectName(QString::fromUtf8("kdfLabel"));

        gridLayout->addWidget(kdfLabel, 1, 0, 1, 1);

        algorithmComboBox = new QComboBox(advancedTab);
        algorithmComboBox->addItem(QString());
        algorithmComboBox->addItem(QString());
        algorithmComboBox->setObjectName(QString::fromUtf8("algorithmComboBox"));
        sizePolicy2.setHeightForWidth(algorithmComboBox->sizePolicy().hasHeightForWidth());
        algorithmComboBox->setSizePolicy(sizePolicy2);

        gridLayout->addWidget(algorithmComboBox, 0, 1, 1, 1);

        algorithmLabel = new QLabel(advancedTab);
        algorithmLabel->setObjectName(QString::fromUtf8("algorithmLabel"));

        gridLayout->addWidget(algorithmLabel, 0, 0, 1, 1);

        encryptionSettingsTabWidget->addTab(advancedTab, QString());

        verticalLayout->addWidget(encryptionSettingsTabWidget);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        verticalLayout->setStretch(4, 1);
        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_3->addWidget(scrollArea);

        QWidget::setTabOrder(decryptionTimeSlider, algorithmComboBox);
        QWidget::setTabOrder(algorithmComboBox, kdfComboBox);
        QWidget::setTabOrder(kdfComboBox, transformRoundsSpinBox);
        QWidget::setTabOrder(transformRoundsSpinBox, transformBenchmarkButton);
        QWidget::setTabOrder(transformBenchmarkButton, memorySpinBox);
        QWidget::setTabOrder(memorySpinBox, parallelismSpinBox);

        retranslateUi(DatabaseSettingsWidgetEncryption);

        encryptionSettingsTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(DatabaseSettingsWidgetEncryption);
    } // setupUi

    void retranslateUi(QWidget *DatabaseSettingsWidgetEncryption)
    {
        compatibilityLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Database format:", nullptr));
#if QT_CONFIG(tooltip)
        compatibilitySelection->setToolTip(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Unless you need to open your database with other programs, always use the latest format.", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(accessibility)
        compatibilitySelection->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Database format", nullptr));
#endif // QT_CONFIG(accessibility)
        formatCannotBeChanged->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Format cannot be changed: Your database uses KDBX 4 features", nullptr));
        label->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Encryption Settings:", nullptr));
        decryptionTimeLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Decryption Time:", nullptr));
#if QT_CONFIG(accessibility)
        decryptionTimeSlider->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Decryption time in seconds", nullptr));
#endif // QT_CONFIG(accessibility)
        label_2->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Higher values offer more protection, but opening the database will take longer.", nullptr));
        encryptionSettingsTabWidget->setTabText(encryptionSettingsTabWidget->indexOf(basicTab), QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Basic", nullptr));
#if QT_CONFIG(accessibility)
        parallelismSpinBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Parallelism", nullptr));
#endif // QT_CONFIG(accessibility)
        parallelismLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Parallelism:", nullptr));
#if QT_CONFIG(accessibility)
        memorySpinBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Memory usage", nullptr));
#endif // QT_CONFIG(accessibility)
        memoryUsageLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Memory Usage:", nullptr));
#if QT_CONFIG(accessibility)
        transformRoundsSpinBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Transform rounds", nullptr));
#endif // QT_CONFIG(accessibility)
        transformRoundsLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Transform rounds:", nullptr));
#if QT_CONFIG(accessibility)
        kdfComboBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Key derivation function", nullptr));
#endif // QT_CONFIG(accessibility)
        kdfLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Key Derivation Function:", nullptr));
        algorithmComboBox->setItemText(0, QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "AES:  256 Bit   (default)", nullptr));
        algorithmComboBox->setItemText(1, QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Twofish:  256 Bit", nullptr));

#if QT_CONFIG(accessibility)
        algorithmComboBox->setAccessibleName(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Encryption algorithm", nullptr));
#endif // QT_CONFIG(accessibility)
        algorithmLabel->setText(QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Encryption Algorithm:", nullptr));
        encryptionSettingsTabWidget->setTabText(encryptionSettingsTabWidget->indexOf(advancedTab), QCoreApplication::translate("DatabaseSettingsWidgetEncryption", "Advanced", nullptr));
        (void)DatabaseSettingsWidgetEncryption;
    } // retranslateUi

};

namespace Ui {
    class DatabaseSettingsWidgetEncryption: public Ui_DatabaseSettingsWidgetEncryption {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DATABASESETTINGSWIDGETENCRYPTION_H
