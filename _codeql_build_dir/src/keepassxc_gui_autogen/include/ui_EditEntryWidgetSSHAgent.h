/********************************************************************************
** Form generated from reading UI file 'EditEntryWidgetSSHAgent.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EDITENTRYWIDGETSSHAGENT_H
#define UI_EDITENTRYWIDGETSSHAGENT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_EditEntryWidgetSSHAgent
{
public:
    QGridLayout *gridLayout;
    QCheckBox *removeKeyFromAgentCheckBox;
    QLabel *commentLabel;
    QCheckBox *addKeyToAgentCheckBox;
    QPlainTextEdit *publicKeyEdit;
    QPushButton *decryptButton;
    QSpacerItem *verticalSpacer;
    QLabel *fingerprintLabel;
    QPushButton *copyToClipboardButton;
    QLabel *publicKeyLabel;
    QGroupBox *privateKeyGroupBox;
    QGridLayout *gridLayout_3;
    QRadioButton *attachmentRadioButton;
    QLineEdit *externalFileEdit;
    QHBoxLayout *agentActionsLayout;
    QPushButton *addToAgentButton;
    QPushButton *removeFromAgentButton;
    QPushButton *clearAgentButton;
    QRadioButton *externalFileRadioButton;
    QPushButton *browseButton;
    QPushButton *generateButton;
    QComboBox *attachmentComboBox;
    QCheckBox *requireUserConfirmationCheckBox;
    QHBoxLayout *horizontalLayout_4;
    QLabel *commentTextLabel;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *removeKeyLayout;
    QCheckBox *lifetimeCheckBox;
    QSpinBox *lifetimeSpinBox;
    QSpacerItem *removeKeySpacer;
    QHBoxLayout *horizontalLayout;
    QLabel *fingerprintTextLabel;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QWidget *EditEntryWidgetSSHAgent)
    {
        if (EditEntryWidgetSSHAgent->objectName().isEmpty())
            EditEntryWidgetSSHAgent->setObjectName(QString::fromUtf8("EditEntryWidgetSSHAgent"));
        EditEntryWidgetSSHAgent->resize(452, 618);
        gridLayout = new QGridLayout(EditEntryWidgetSSHAgent);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        removeKeyFromAgentCheckBox = new QCheckBox(EditEntryWidgetSSHAgent);
        removeKeyFromAgentCheckBox->setObjectName(QString::fromUtf8("removeKeyFromAgentCheckBox"));

        gridLayout->addWidget(removeKeyFromAgentCheckBox, 1, 1, 1, 4);

        commentLabel = new QLabel(EditEntryWidgetSSHAgent);
        commentLabel->setObjectName(QString::fromUtf8("commentLabel"));
        commentLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(commentLabel, 14, 1, 1, 1);

        addKeyToAgentCheckBox = new QCheckBox(EditEntryWidgetSSHAgent);
        addKeyToAgentCheckBox->setObjectName(QString::fromUtf8("addKeyToAgentCheckBox"));

        gridLayout->addWidget(addKeyToAgentCheckBox, 0, 1, 1, 4);

        publicKeyEdit = new QPlainTextEdit(EditEntryWidgetSSHAgent);
        publicKeyEdit->setObjectName(QString::fromUtf8("publicKeyEdit"));
        QFont font;
        font.setFamily(QString::fromUtf8("Monospace"));
        publicKeyEdit->setFont(font);
        publicKeyEdit->setReadOnly(true);

        gridLayout->addWidget(publicKeyEdit, 15, 3, 1, 2);

        decryptButton = new QPushButton(EditEntryWidgetSSHAgent);
        decryptButton->setObjectName(QString::fromUtf8("decryptButton"));

        gridLayout->addWidget(decryptButton, 14, 4, 1, 1);

        verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(verticalSpacer, 4, 1, 1, 1);

        fingerprintLabel = new QLabel(EditEntryWidgetSSHAgent);
        fingerprintLabel->setObjectName(QString::fromUtf8("fingerprintLabel"));
        fingerprintLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(fingerprintLabel, 13, 1, 1, 1);

        copyToClipboardButton = new QPushButton(EditEntryWidgetSSHAgent);
        copyToClipboardButton->setObjectName(QString::fromUtf8("copyToClipboardButton"));

        gridLayout->addWidget(copyToClipboardButton, 16, 3, 1, 2);

        publicKeyLabel = new QLabel(EditEntryWidgetSSHAgent);
        publicKeyLabel->setObjectName(QString::fromUtf8("publicKeyLabel"));
        publicKeyLabel->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(publicKeyLabel, 15, 1, 1, 1);

        privateKeyGroupBox = new QGroupBox(EditEntryWidgetSSHAgent);
        privateKeyGroupBox->setObjectName(QString::fromUtf8("privateKeyGroupBox"));
        gridLayout_3 = new QGridLayout(privateKeyGroupBox);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        attachmentRadioButton = new QRadioButton(privateKeyGroupBox);
        attachmentRadioButton->setObjectName(QString::fromUtf8("attachmentRadioButton"));
        attachmentRadioButton->setChecked(true);

        gridLayout_3->addWidget(attachmentRadioButton, 0, 0, 1, 1);

        externalFileEdit = new QLineEdit(privateKeyGroupBox);
        externalFileEdit->setObjectName(QString::fromUtf8("externalFileEdit"));
        externalFileEdit->setFocusPolicy(Qt::ClickFocus);

        gridLayout_3->addWidget(externalFileEdit, 3, 3, 1, 1);

        agentActionsLayout = new QHBoxLayout();
        agentActionsLayout->setObjectName(QString::fromUtf8("agentActionsLayout"));
        addToAgentButton = new QPushButton(privateKeyGroupBox);
        addToAgentButton->setObjectName(QString::fromUtf8("addToAgentButton"));

        agentActionsLayout->addWidget(addToAgentButton);

        removeFromAgentButton = new QPushButton(privateKeyGroupBox);
        removeFromAgentButton->setObjectName(QString::fromUtf8("removeFromAgentButton"));

        agentActionsLayout->addWidget(removeFromAgentButton);

        clearAgentButton = new QPushButton(privateKeyGroupBox);
        clearAgentButton->setObjectName(QString::fromUtf8("clearAgentButton"));

        agentActionsLayout->addWidget(clearAgentButton);


        gridLayout_3->addLayout(agentActionsLayout, 4, 3, 1, 1);

        externalFileRadioButton = new QRadioButton(privateKeyGroupBox);
        externalFileRadioButton->setObjectName(QString::fromUtf8("externalFileRadioButton"));

        gridLayout_3->addWidget(externalFileRadioButton, 3, 0, 1, 1);

        browseButton = new QPushButton(privateKeyGroupBox);
        browseButton->setObjectName(QString::fromUtf8("browseButton"));

        gridLayout_3->addWidget(browseButton, 3, 4, 1, 1);

        generateButton = new QPushButton(privateKeyGroupBox);
        generateButton->setObjectName(QString::fromUtf8("generateButton"));

        gridLayout_3->addWidget(generateButton, 0, 4, 1, 1);

        attachmentComboBox = new QComboBox(privateKeyGroupBox);
        attachmentComboBox->setObjectName(QString::fromUtf8("attachmentComboBox"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(attachmentComboBox->sizePolicy().hasHeightForWidth());
        attachmentComboBox->setSizePolicy(sizePolicy);
        attachmentComboBox->setEditable(false);

        gridLayout_3->addWidget(attachmentComboBox, 0, 3, 1, 1);


        gridLayout->addWidget(privateKeyGroupBox, 5, 1, 1, 4);

        requireUserConfirmationCheckBox = new QCheckBox(EditEntryWidgetSSHAgent);
        requireUserConfirmationCheckBox->setObjectName(QString::fromUtf8("requireUserConfirmationCheckBox"));

        gridLayout->addWidget(requireUserConfirmationCheckBox, 2, 1, 1, 4);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        commentTextLabel = new QLabel(EditEntryWidgetSSHAgent);
        commentTextLabel->setObjectName(QString::fromUtf8("commentTextLabel"));
        commentTextLabel->setFont(font);
        commentTextLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        horizontalLayout_4->addWidget(commentTextLabel);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer);


        gridLayout->addLayout(horizontalLayout_4, 14, 3, 1, 1);

        removeKeyLayout = new QHBoxLayout();
        removeKeyLayout->setObjectName(QString::fromUtf8("removeKeyLayout"));
        lifetimeCheckBox = new QCheckBox(EditEntryWidgetSSHAgent);
        lifetimeCheckBox->setObjectName(QString::fromUtf8("lifetimeCheckBox"));

        removeKeyLayout->addWidget(lifetimeCheckBox);

        lifetimeSpinBox = new QSpinBox(EditEntryWidgetSSHAgent);
        lifetimeSpinBox->setObjectName(QString::fromUtf8("lifetimeSpinBox"));
        lifetimeSpinBox->setMaximum(999999999);

        removeKeyLayout->addWidget(lifetimeSpinBox);

        removeKeySpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        removeKeyLayout->addItem(removeKeySpacer);


        gridLayout->addLayout(removeKeyLayout, 3, 1, 1, 4);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        fingerprintTextLabel = new QLabel(EditEntryWidgetSSHAgent);
        fingerprintTextLabel->setObjectName(QString::fromUtf8("fingerprintTextLabel"));
        fingerprintTextLabel->setFont(font);
        fingerprintTextLabel->setTextInteractionFlags(Qt::TextSelectableByKeyboard|Qt::TextSelectableByMouse);

        horizontalLayout->addWidget(fingerprintTextLabel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout->addLayout(horizontalLayout, 13, 3, 1, 2);

        verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout->addItem(verticalSpacer_2, 12, 3, 1, 1);

        QWidget::setTabOrder(addKeyToAgentCheckBox, removeKeyFromAgentCheckBox);
        QWidget::setTabOrder(removeKeyFromAgentCheckBox, requireUserConfirmationCheckBox);
        QWidget::setTabOrder(requireUserConfirmationCheckBox, lifetimeCheckBox);
        QWidget::setTabOrder(lifetimeCheckBox, lifetimeSpinBox);
        QWidget::setTabOrder(lifetimeSpinBox, attachmentRadioButton);
        QWidget::setTabOrder(attachmentRadioButton, externalFileRadioButton);
        QWidget::setTabOrder(externalFileRadioButton, browseButton);
        QWidget::setTabOrder(browseButton, addToAgentButton);
        QWidget::setTabOrder(addToAgentButton, removeFromAgentButton);
        QWidget::setTabOrder(removeFromAgentButton, decryptButton);
        QWidget::setTabOrder(decryptButton, publicKeyEdit);
        QWidget::setTabOrder(publicKeyEdit, copyToClipboardButton);

        retranslateUi(EditEntryWidgetSSHAgent);

        QMetaObject::connectSlotsByName(EditEntryWidgetSSHAgent);
    } // setupUi

    void retranslateUi(QWidget *EditEntryWidgetSSHAgent)
    {
        EditEntryWidgetSSHAgent->setWindowTitle(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Form", nullptr));
        removeKeyFromAgentCheckBox->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Remove key from agent when database is closed/locked", nullptr));
        commentLabel->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Comment", nullptr));
        addKeyToAgentCheckBox->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Add key to agent when database is opened/unlocked", nullptr));
        decryptButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Decrypt", nullptr));
        fingerprintLabel->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Fingerprint", nullptr));
        copyToClipboardButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Copy to clipboard", nullptr));
        publicKeyLabel->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Public key", nullptr));
        privateKeyGroupBox->setTitle(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Private key", nullptr));
        attachmentRadioButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Attachment", nullptr));
#if QT_CONFIG(accessibility)
        externalFileEdit->setAccessibleName(QCoreApplication::translate("EditEntryWidgetSSHAgent", "External key file", nullptr));
#endif // QT_CONFIG(accessibility)
        addToAgentButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Add to agent", nullptr));
        removeFromAgentButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Remove from agent", nullptr));
        clearAgentButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Clear agent", nullptr));
        externalFileRadioButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "External file", nullptr));
#if QT_CONFIG(accessibility)
        browseButton->setAccessibleName(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Browser for key file", nullptr));
#endif // QT_CONFIG(accessibility)
        browseButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Browse\342\200\246", nullptr));
        generateButton->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Generate", nullptr));
#if QT_CONFIG(accessibility)
        attachmentComboBox->setAccessibleName(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Select attachment file", nullptr));
#endif // QT_CONFIG(accessibility)
        requireUserConfirmationCheckBox->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Require user confirmation when this key is used", nullptr));
        commentTextLabel->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "n/a", nullptr));
        lifetimeCheckBox->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Remove key from agent after", nullptr));
#if QT_CONFIG(accessibility)
        lifetimeSpinBox->setAccessibleName(QCoreApplication::translate("EditEntryWidgetSSHAgent", "Remove key from agent after specified seconds", nullptr));
#endif // QT_CONFIG(accessibility)
        lifetimeSpinBox->setSuffix(QCoreApplication::translate("EditEntryWidgetSSHAgent", " seconds", nullptr));
        fingerprintTextLabel->setText(QCoreApplication::translate("EditEntryWidgetSSHAgent", "n/a", nullptr));
    } // retranslateUi

};

namespace Ui {
    class EditEntryWidgetSSHAgent: public Ui_EditEntryWidgetSSHAgent {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EDITENTRYWIDGETSSHAGENT_H
