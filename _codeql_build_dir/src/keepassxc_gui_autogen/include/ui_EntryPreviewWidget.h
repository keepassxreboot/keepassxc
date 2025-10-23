/********************************************************************************
** Form generated from reading UI file 'EntryPreviewWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ENTRYPREVIEWWIDGET_H
#define UI_ENTRYPREVIEWWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "gui/entry/EntryAttachmentsWidget.h"
#include "gui/tag/TagsEdit.h"
#include "gui/widgets/ElidedLabel.h"

QT_BEGIN_NAMESPACE

class Ui_EntryPreviewWidget
{
public:
    QAction *searchIcon;
    QAction *clearIcon;
    QVBoxLayout *verticalLayout_7;
    QStackedWidget *stackedWidget;
    QWidget *pageEntry;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *entryHorizontalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *entryIcon;
    ElidedLabel *entryTitleLabel;
    QWidget *entryTotp;
    QVBoxLayout *verticalLayout_2;
    QLabel *entryTotpLabel;
    QProgressBar *entryTotpProgress;
    QToolButton *entryTotpButton;
    QToolButton *entryCloseButton;
    QTabWidget *entryTabWidget;
    QWidget *entryGeneralTab;
    QVBoxLayout *verticalLayout_5;
    QWidget *entryGeneralWidget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_7;
    QToolButton *togglePasswordButton;
    QScrollArea *passwordScrollArea;
    QWidget *scrollAreaWidgetContents;
    QHBoxLayout *horizontalLayout_8;
    QLabel *entryPasswordLabel;
    ElidedLabel *entryUrlLabel;
    QSpacerItem *entryMiddleHorizontalSpacer;
    QLabel *entryUsernameTitleLabel;
    QHBoxLayout *horizontalLayout_3;
    QToolButton *toggleEntryNotesButton;
    QTextEdit *entryNotesTextEdit;
    QLabel *entryTagsTitleLabel;
    QHBoxLayout *horizontalLayout_6;
    QToolButton *toggleUsernameButton;
    QLineEdit *entryUsernameLabel;
    TagsEdit *entryTagsList;
    QLabel *entryExpirationLabel;
    QSpacerItem *entryMiddleHorizontalSpacer_3;
    QLabel *entryPasswordTitleLabel;
    QLabel *entryNotesTitleLabel;
    QLabel *entryExpirationTitleLabel;
    QLabel *entryUrlTitleLabel;
    QWidget *entryAdvancedTab;
    QGridLayout *gridLayout_1;
    QLabel *attachmentsTitleLabel;
    EntryAttachmentsWidget *entryAttachmentsWidget;
    QLabel *attributesTitleLabel;
    QTableWidget *entryAttributesTable;
    QWidget *entryAutotypeTab;
    QVBoxLayout *verticalLayout_4;
    QWidget *entryAutotypeWidget;
    QGridLayout *gridLayout_3;
    QLabel *entrySequenceTitleLabel;
    QLabel *entrySequenceLabel;
    QTreeWidget *entryAutotypeTree;
    QWidget *pageGroup;
    QVBoxLayout *verticalLayout_13;
    QHBoxLayout *groupHorizontalLayout;
    QHBoxLayout *horizontalLayout_5;
    QLabel *groupIcon;
    ElidedLabel *groupTitleLabel;
    QToolButton *groupCloseButton;
    QTabWidget *groupTabWidget;
    QWidget *groupGeneralTab;
    QVBoxLayout *verticalLayout_8;
    QWidget *groupGeneralWidget;
    QGridLayout *gridLayout_2;
    QLabel *groupExpirationLabel;
    QLabel *groupAutotypeTitleLabel;
    QLabel *groupSearchingLabel;
    QLabel *groupExpirationTitleLabel;
    QLabel *groupSearchingTitleLabel;
    QLabel *groupNotesTitleLabel;
    QHBoxLayout *horizontalLayout_4;
    QToolButton *toggleGroupNotesButton;
    QTextEdit *groupNotesTextEdit;
    QLabel *groupAutotypeLabel;
    QWidget *groupShareTab;
    QVBoxLayout *verticalLayout_12;
    QWidget *groupShareWidget;
    QGridLayout *gridLayout_4;
    QLabel *groupSharePathLabel;
    QLabel *groupShareTypeLabel;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *EntryPreviewWidget)
    {
        if (EntryPreviewWidget->objectName().isEmpty())
            EntryPreviewWidget->setObjectName(QString::fromUtf8("EntryPreviewWidget"));
        EntryPreviewWidget->resize(530, 257);
        searchIcon = new QAction(EntryPreviewWidget);
        searchIcon->setObjectName(QString::fromUtf8("searchIcon"));
        clearIcon = new QAction(EntryPreviewWidget);
        clearIcon->setObjectName(QString::fromUtf8("clearIcon"));
        verticalLayout_7 = new QVBoxLayout(EntryPreviewWidget);
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(EntryPreviewWidget);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        pageEntry = new QWidget();
        pageEntry->setObjectName(QString::fromUtf8("pageEntry"));
        verticalLayout_3 = new QVBoxLayout(pageEntry);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        entryHorizontalLayout = new QHBoxLayout();
        entryHorizontalLayout->setObjectName(QString::fromUtf8("entryHorizontalLayout"));
        entryHorizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        entryHorizontalLayout->setContentsMargins(5, 3, -1, 3);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(12);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        entryIcon = new QLabel(pageEntry);
        entryIcon->setObjectName(QString::fromUtf8("entryIcon"));
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(entryIcon->sizePolicy().hasHeightForWidth());
        entryIcon->setSizePolicy(sizePolicy);
        entryIcon->setMinimumSize(QSize(16, 0));

        horizontalLayout->addWidget(entryIcon);

        entryTitleLabel = new ElidedLabel(pageEntry);
        entryTitleLabel->setObjectName(QString::fromUtf8("entryTitleLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(entryTitleLabel->sizePolicy().hasHeightForWidth());
        entryTitleLabel->setSizePolicy(sizePolicy1);
        QFont font;
        font.setPointSize(12);
        entryTitleLabel->setFont(font);
        entryTitleLabel->setFocusPolicy(Qt::ClickFocus);
        entryTitleLabel->setTextFormat(Qt::PlainText);
        entryTitleLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

        horizontalLayout->addWidget(entryTitleLabel);


        entryHorizontalLayout->addLayout(horizontalLayout);

        entryTotp = new QWidget(pageEntry);
        entryTotp->setObjectName(QString::fromUtf8("entryTotp"));
        entryTotp->setMinimumSize(QSize(0, 0));
        verticalLayout_2 = new QVBoxLayout(entryTotp);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        entryTotpLabel = new QLabel(entryTotp);
        entryTotpLabel->setObjectName(QString::fromUtf8("entryTotpLabel"));
        QFont font1;
        font1.setPointSize(10);
        font1.setBold(true);
        entryTotpLabel->setFont(font1);
        entryTotpLabel->setText(QString::fromUtf8("1234567"));
        entryTotpLabel->setTextInteractionFlags(Qt::TextInteractionFlag::LinksAccessibleByMouse|Qt::TextInteractionFlag::TextSelectableByKeyboard|Qt::TextInteractionFlag::TextSelectableByMouse);

        verticalLayout_2->addWidget(entryTotpLabel);

        entryTotpProgress = new QProgressBar(entryTotp);
        entryTotpProgress->setObjectName(QString::fromUtf8("entryTotpProgress"));
        entryTotpProgress->setMaximumSize(QSize(57, 4));
        entryTotpProgress->setValue(50);
        entryTotpProgress->setTextVisible(false);

        verticalLayout_2->addWidget(entryTotpProgress);


        entryHorizontalLayout->addWidget(entryTotp);

        entryTotpButton = new QToolButton(pageEntry);
        entryTotpButton->setObjectName(QString::fromUtf8("entryTotpButton"));
        entryTotpButton->setCheckable(true);
        entryTotpButton->setChecked(true);

        entryHorizontalLayout->addWidget(entryTotpButton);

        entryCloseButton = new QToolButton(pageEntry);
        entryCloseButton->setObjectName(QString::fromUtf8("entryCloseButton"));

        entryHorizontalLayout->addWidget(entryCloseButton);

        entryHorizontalLayout->setStretch(0, 1);

        verticalLayout_3->addLayout(entryHorizontalLayout);

        entryTabWidget = new QTabWidget(pageEntry);
        entryTabWidget->setObjectName(QString::fromUtf8("entryTabWidget"));
        entryTabWidget->setDocumentMode(false);
        entryTabWidget->setTabsClosable(false);
        entryTabWidget->setMovable(false);
        entryGeneralTab = new QWidget();
        entryGeneralTab->setObjectName(QString::fromUtf8("entryGeneralTab"));
        verticalLayout_5 = new QVBoxLayout(entryGeneralTab);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        entryGeneralWidget = new QWidget(entryGeneralTab);
        entryGeneralWidget->setObjectName(QString::fromUtf8("entryGeneralWidget"));
        gridLayout = new QGridLayout(entryGeneralWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setHorizontalSpacing(8);
        gridLayout->setVerticalSpacing(6);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        togglePasswordButton = new QToolButton(entryGeneralWidget);
        togglePasswordButton->setObjectName(QString::fromUtf8("togglePasswordButton"));
        togglePasswordButton->setText(QString::fromUtf8(""));
        togglePasswordButton->setIconSize(QSize(14, 14));
        togglePasswordButton->setCheckable(true);

        horizontalLayout_7->addWidget(togglePasswordButton);

        passwordScrollArea = new QScrollArea(entryGeneralWidget);
        passwordScrollArea->setObjectName(QString::fromUtf8("passwordScrollArea"));
        passwordScrollArea->setFrameShape(QFrame::NoFrame);
        passwordScrollArea->setFrameShadow(QFrame::Plain);
        passwordScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        passwordScrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        passwordScrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 157, 63));
        horizontalLayout_8 = new QHBoxLayout(scrollAreaWidgetContents);
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        horizontalLayout_8->setContentsMargins(6, 0, 0, 0);
        entryPasswordLabel = new QLabel(scrollAreaWidgetContents);
        entryPasswordLabel->setObjectName(QString::fromUtf8("entryPasswordLabel"));
        entryPasswordLabel->setFocusPolicy(Qt::ClickFocus);
        entryPasswordLabel->setText(QString::fromUtf8("TextLabel"));
        entryPasswordLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);
        entryPasswordLabel->setProperty("blendIn", QVariant(true));

        horizontalLayout_8->addWidget(entryPasswordLabel);

        passwordScrollArea->setWidget(scrollAreaWidgetContents);

        horizontalLayout_7->addWidget(passwordScrollArea);


        gridLayout->addLayout(horizontalLayout_7, 1, 1, 1, 2);

        entryUrlLabel = new ElidedLabel(entryGeneralWidget);
        entryUrlLabel->setObjectName(QString::fromUtf8("entryUrlLabel"));
        sizePolicy1.setHeightForWidth(entryUrlLabel->sizePolicy().hasHeightForWidth());
        entryUrlLabel->setSizePolicy(sizePolicy1);
        entryUrlLabel->setMinimumSize(QSize(150, 0));
        entryUrlLabel->setCursor(QCursor(Qt::PointingHandCursor));
        entryUrlLabel->setFocusPolicy(Qt::ClickFocus);
        entryUrlLabel->setText(QString::fromUtf8("https://example.com"));
        entryUrlLabel->setTextFormat(Qt::RichText);
        entryUrlLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
        entryUrlLabel->setProperty("blendIn", QVariant(true));

        gridLayout->addWidget(entryUrlLabel, 0, 5, 1, 1);

        entryMiddleHorizontalSpacer = new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(entryMiddleHorizontalSpacer, 0, 3, 1, 1);

        entryUsernameTitleLabel = new QLabel(entryGeneralWidget);
        entryUsernameTitleLabel->setObjectName(QString::fromUtf8("entryUsernameTitleLabel"));
        sizePolicy.setHeightForWidth(entryUsernameTitleLabel->sizePolicy().hasHeightForWidth());
        entryUsernameTitleLabel->setSizePolicy(sizePolicy);
        QFont font2;
        font2.setBold(true);
        font2.setWeight(75);
        entryUsernameTitleLabel->setFont(font2);
        entryUsernameTitleLabel->setLayoutDirection(Qt::LeftToRight);
        entryUsernameTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(entryUsernameTitleLabel, 0, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(6);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        toggleEntryNotesButton = new QToolButton(entryGeneralWidget);
        toggleEntryNotesButton->setObjectName(QString::fromUtf8("toggleEntryNotesButton"));
        toggleEntryNotesButton->setIconSize(QSize(14, 14));
        toggleEntryNotesButton->setCheckable(true);

        horizontalLayout_3->addWidget(toggleEntryNotesButton, 0, Qt::AlignTop);

        entryNotesTextEdit = new QTextEdit(entryGeneralWidget);
        entryNotesTextEdit->setObjectName(QString::fromUtf8("entryNotesTextEdit"));
        entryNotesTextEdit->setFocusPolicy(Qt::ClickFocus);
        entryNotesTextEdit->setFrameShape(QFrame::NoFrame);
        entryNotesTextEdit->setFrameShadow(QFrame::Plain);
        entryNotesTextEdit->setLineWidth(0);
        entryNotesTextEdit->setTabChangesFocus(true);
        entryNotesTextEdit->setReadOnly(true);
        entryNotesTextEdit->setTabStopDistance(10.000000000000000);
        entryNotesTextEdit->setProperty("blendIn", QVariant(true));

        horizontalLayout_3->addWidget(entryNotesTextEdit);


        gridLayout->addLayout(horizontalLayout_3, 3, 1, 1, 5);

        entryTagsTitleLabel = new QLabel(entryGeneralWidget);
        entryTagsTitleLabel->setObjectName(QString::fromUtf8("entryTagsTitleLabel"));
        sizePolicy.setHeightForWidth(entryTagsTitleLabel->sizePolicy().hasHeightForWidth());
        entryTagsTitleLabel->setSizePolicy(sizePolicy);
        entryTagsTitleLabel->setFont(font2);
        entryTagsTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(entryTagsTitleLabel, 2, 0, 1, 1);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(8);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        toggleUsernameButton = new QToolButton(entryGeneralWidget);
        toggleUsernameButton->setObjectName(QString::fromUtf8("toggleUsernameButton"));
        toggleUsernameButton->setIconSize(QSize(14, 14));
        toggleUsernameButton->setCheckable(true);

        horizontalLayout_6->addWidget(toggleUsernameButton);

        entryUsernameLabel = new QLineEdit(entryGeneralWidget);
        entryUsernameLabel->setObjectName(QString::fromUtf8("entryUsernameLabel"));
        entryUsernameLabel->setMinimumSize(QSize(150, 0));
        entryUsernameLabel->setFocusPolicy(Qt::ClickFocus);
        entryUsernameLabel->setText(QString::fromUtf8("username"));
        entryUsernameLabel->setFrame(false);
        entryUsernameLabel->setCursorPosition(8);
        entryUsernameLabel->setDragEnabled(true);
        entryUsernameLabel->setReadOnly(true);
        entryUsernameLabel->setProperty("blendIn", QVariant(true));

        horizontalLayout_6->addWidget(entryUsernameLabel);


        gridLayout->addLayout(horizontalLayout_6, 0, 1, 1, 2);

        entryTagsList = new TagsEdit(entryGeneralWidget);
        entryTagsList->setObjectName(QString::fromUtf8("entryTagsList"));
        entryTagsList->setFocusPolicy(Qt::ClickFocus);
        entryTagsList->setProperty("blendIn", QVariant(true));

        gridLayout->addWidget(entryTagsList, 2, 1, 1, 5);

        entryExpirationLabel = new QLabel(entryGeneralWidget);
        entryExpirationLabel->setObjectName(QString::fromUtf8("entryExpirationLabel"));
        sizePolicy1.setHeightForWidth(entryExpirationLabel->sizePolicy().hasHeightForWidth());
        entryExpirationLabel->setSizePolicy(sizePolicy1);
        entryExpirationLabel->setFocusPolicy(Qt::ClickFocus);
        entryExpirationLabel->setText(QString::fromUtf8("expired"));
        entryExpirationLabel->setTextFormat(Qt::PlainText);
        entryExpirationLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);
        entryExpirationLabel->setProperty("blendIn", QVariant(true));

        gridLayout->addWidget(entryExpirationLabel, 1, 5, 1, 1);

        entryMiddleHorizontalSpacer_3 = new QSpacerItem(10, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        gridLayout->addItem(entryMiddleHorizontalSpacer_3, 1, 3, 1, 1);

        entryPasswordTitleLabel = new QLabel(entryGeneralWidget);
        entryPasswordTitleLabel->setObjectName(QString::fromUtf8("entryPasswordTitleLabel"));
        sizePolicy.setHeightForWidth(entryPasswordTitleLabel->sizePolicy().hasHeightForWidth());
        entryPasswordTitleLabel->setSizePolicy(sizePolicy);
        entryPasswordTitleLabel->setFont(font2);
        entryPasswordTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(entryPasswordTitleLabel, 1, 0, 1, 1);

        entryNotesTitleLabel = new QLabel(entryGeneralWidget);
        entryNotesTitleLabel->setObjectName(QString::fromUtf8("entryNotesTitleLabel"));
        sizePolicy.setHeightForWidth(entryNotesTitleLabel->sizePolicy().hasHeightForWidth());
        entryNotesTitleLabel->setSizePolicy(sizePolicy);
        entryNotesTitleLabel->setFont(font2);
        entryNotesTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout->addWidget(entryNotesTitleLabel, 3, 0, 1, 1);

        entryExpirationTitleLabel = new QLabel(entryGeneralWidget);
        entryExpirationTitleLabel->setObjectName(QString::fromUtf8("entryExpirationTitleLabel"));
        sizePolicy.setHeightForWidth(entryExpirationTitleLabel->sizePolicy().hasHeightForWidth());
        entryExpirationTitleLabel->setSizePolicy(sizePolicy);
        entryExpirationTitleLabel->setFont(font2);
        entryExpirationTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(entryExpirationTitleLabel, 1, 4, 1, 1);

        entryUrlTitleLabel = new QLabel(entryGeneralWidget);
        entryUrlTitleLabel->setObjectName(QString::fromUtf8("entryUrlTitleLabel"));
        sizePolicy.setHeightForWidth(entryUrlTitleLabel->sizePolicy().hasHeightForWidth());
        entryUrlTitleLabel->setSizePolicy(sizePolicy);
        entryUrlTitleLabel->setFont(font2);
        entryUrlTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout->addWidget(entryUrlTitleLabel, 0, 4, 1, 1);

        gridLayout->setRowStretch(3, 1);
        gridLayout->setColumnStretch(1, 1);
        gridLayout->setColumnStretch(5, 1);

        verticalLayout_5->addWidget(entryGeneralWidget);

        entryTabWidget->addTab(entryGeneralTab, QString());
        entryAdvancedTab = new QWidget();
        entryAdvancedTab->setObjectName(QString::fromUtf8("entryAdvancedTab"));
        gridLayout_1 = new QGridLayout(entryAdvancedTab);
        gridLayout_1->setObjectName(QString::fromUtf8("gridLayout_1"));
        gridLayout_1->setHorizontalSpacing(8);
        gridLayout_1->setContentsMargins(5, 5, 5, 5);
        attachmentsTitleLabel = new QLabel(entryAdvancedTab);
        attachmentsTitleLabel->setObjectName(QString::fromUtf8("attachmentsTitleLabel"));
        sizePolicy.setHeightForWidth(attachmentsTitleLabel->sizePolicy().hasHeightForWidth());
        attachmentsTitleLabel->setSizePolicy(sizePolicy);
        attachmentsTitleLabel->setFont(font2);

        gridLayout_1->addWidget(attachmentsTitleLabel, 0, 1, 1, 1);

        entryAttachmentsWidget = new EntryAttachmentsWidget(entryAdvancedTab);
        entryAttachmentsWidget->setObjectName(QString::fromUtf8("entryAttachmentsWidget"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(entryAttachmentsWidget->sizePolicy().hasHeightForWidth());
        entryAttachmentsWidget->setSizePolicy(sizePolicy2);
        entryAttachmentsWidget->setFocusPolicy(Qt::ClickFocus);

        gridLayout_1->addWidget(entryAttachmentsWidget, 1, 1, 1, 1);

        attributesTitleLabel = new QLabel(entryAdvancedTab);
        attributesTitleLabel->setObjectName(QString::fromUtf8("attributesTitleLabel"));
        sizePolicy.setHeightForWidth(attributesTitleLabel->sizePolicy().hasHeightForWidth());
        attributesTitleLabel->setSizePolicy(sizePolicy);
        attributesTitleLabel->setFont(font2);

        gridLayout_1->addWidget(attributesTitleLabel, 0, 0, 1, 1);

        entryAttributesTable = new QTableWidget(entryAdvancedTab);
        entryAttributesTable->setObjectName(QString::fromUtf8("entryAttributesTable"));
        entryAttributesTable->setFocusPolicy(Qt::NoFocus);
        entryAttributesTable->setStyleSheet(QString::fromUtf8("QTableView::item {padding: 3px;}"));
        entryAttributesTable->setFrameShape(QFrame::NoFrame);
        entryAttributesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        entryAttributesTable->setProperty("showDropIndicator", QVariant(false));
        entryAttributesTable->setSelectionMode(QAbstractItemView::NoSelection);
        entryAttributesTable->setShowGrid(false);
        entryAttributesTable->setCornerButtonEnabled(false);
        entryAttributesTable->horizontalHeader()->setVisible(false);
        entryAttributesTable->horizontalHeader()->setStretchLastSection(true);
        entryAttributesTable->verticalHeader()->setVisible(false);

        gridLayout_1->addWidget(entryAttributesTable, 1, 0, 1, 1);

        entryTabWidget->addTab(entryAdvancedTab, QString());
        entryAutotypeTab = new QWidget();
        entryAutotypeTab->setObjectName(QString::fromUtf8("entryAutotypeTab"));
        verticalLayout_4 = new QVBoxLayout(entryAutotypeTab);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        entryAutotypeWidget = new QWidget(entryAutotypeTab);
        entryAutotypeWidget->setObjectName(QString::fromUtf8("entryAutotypeWidget"));
        gridLayout_3 = new QGridLayout(entryAutotypeWidget);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        gridLayout_3->setContentsMargins(0, 0, 0, 5);
        entrySequenceTitleLabel = new QLabel(entryAutotypeWidget);
        entrySequenceTitleLabel->setObjectName(QString::fromUtf8("entrySequenceTitleLabel"));
        sizePolicy.setHeightForWidth(entrySequenceTitleLabel->sizePolicy().hasHeightForWidth());
        entrySequenceTitleLabel->setSizePolicy(sizePolicy);
        entrySequenceTitleLabel->setFont(font2);
        entrySequenceTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout_3->addWidget(entrySequenceTitleLabel, 0, 0, 1, 1);

        entrySequenceLabel = new QLabel(entryAutotypeWidget);
        entrySequenceLabel->setObjectName(QString::fromUtf8("entrySequenceLabel"));
        sizePolicy1.setHeightForWidth(entrySequenceLabel->sizePolicy().hasHeightForWidth());
        entrySequenceLabel->setSizePolicy(sizePolicy1);
        entrySequenceLabel->setText(QString::fromUtf8("sequence"));
        entrySequenceLabel->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        entrySequenceLabel->setWordWrap(true);

        gridLayout_3->addWidget(entrySequenceLabel, 0, 1, 1, 1);


        verticalLayout_4->addWidget(entryAutotypeWidget);

        entryAutotypeTree = new QTreeWidget(entryAutotypeTab);
        entryAutotypeTree->setObjectName(QString::fromUtf8("entryAutotypeTree"));
        entryAutotypeTree->setFocusPolicy(Qt::NoFocus);
        entryAutotypeTree->setFrameShadow(QFrame::Sunken);
        entryAutotypeTree->setProperty("showDropIndicator", QVariant(true));
        entryAutotypeTree->setSelectionMode(QAbstractItemView::NoSelection);
        entryAutotypeTree->setRootIsDecorated(true);
        entryAutotypeTree->setWordWrap(false);
        entryAutotypeTree->setColumnCount(2);
        entryAutotypeTree->header()->setCascadingSectionResizes(false);
        entryAutotypeTree->header()->setMinimumSectionSize(50);
        entryAutotypeTree->header()->setDefaultSectionSize(250);
        entryAutotypeTree->header()->setStretchLastSection(true);

        verticalLayout_4->addWidget(entryAutotypeTree);

        entryTabWidget->addTab(entryAutotypeTab, QString());

        verticalLayout_3->addWidget(entryTabWidget);

        stackedWidget->addWidget(pageEntry);
        pageGroup = new QWidget();
        pageGroup->setObjectName(QString::fromUtf8("pageGroup"));
        verticalLayout_13 = new QVBoxLayout(pageGroup);
        verticalLayout_13->setObjectName(QString::fromUtf8("verticalLayout_13"));
        verticalLayout_13->setContentsMargins(0, 0, 0, 0);
        groupHorizontalLayout = new QHBoxLayout();
        groupHorizontalLayout->setObjectName(QString::fromUtf8("groupHorizontalLayout"));
        groupHorizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        groupHorizontalLayout->setContentsMargins(5, 3, -1, 3);
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(12);
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        groupIcon = new QLabel(pageGroup);
        groupIcon->setObjectName(QString::fromUtf8("groupIcon"));
        sizePolicy.setHeightForWidth(groupIcon->sizePolicy().hasHeightForWidth());
        groupIcon->setSizePolicy(sizePolicy);

        horizontalLayout_5->addWidget(groupIcon);

        groupTitleLabel = new ElidedLabel(pageGroup);
        groupTitleLabel->setObjectName(QString::fromUtf8("groupTitleLabel"));
        sizePolicy1.setHeightForWidth(groupTitleLabel->sizePolicy().hasHeightForWidth());
        groupTitleLabel->setSizePolicy(sizePolicy1);
        groupTitleLabel->setFont(font);
        groupTitleLabel->setTextFormat(Qt::AutoText);

        horizontalLayout_5->addWidget(groupTitleLabel);


        groupHorizontalLayout->addLayout(horizontalLayout_5);

        groupCloseButton = new QToolButton(pageGroup);
        groupCloseButton->setObjectName(QString::fromUtf8("groupCloseButton"));

        groupHorizontalLayout->addWidget(groupCloseButton);


        verticalLayout_13->addLayout(groupHorizontalLayout);

        groupTabWidget = new QTabWidget(pageGroup);
        groupTabWidget->setObjectName(QString::fromUtf8("groupTabWidget"));
        groupTabWidget->setDocumentMode(false);
        groupTabWidget->setTabsClosable(false);
        groupTabWidget->setMovable(false);
        groupGeneralTab = new QWidget();
        groupGeneralTab->setObjectName(QString::fromUtf8("groupGeneralTab"));
        verticalLayout_8 = new QVBoxLayout(groupGeneralTab);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        groupGeneralWidget = new QWidget(groupGeneralTab);
        groupGeneralWidget->setObjectName(QString::fromUtf8("groupGeneralWidget"));
        gridLayout_2 = new QGridLayout(groupGeneralWidget);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setContentsMargins(0, 0, 0, 0);
        groupExpirationLabel = new QLabel(groupGeneralWidget);
        groupExpirationLabel->setObjectName(QString::fromUtf8("groupExpirationLabel"));
        sizePolicy1.setHeightForWidth(groupExpirationLabel->sizePolicy().hasHeightForWidth());
        groupExpirationLabel->setSizePolicy(sizePolicy1);

        gridLayout_2->addWidget(groupExpirationLabel, 2, 2, 1, 1);

        groupAutotypeTitleLabel = new QLabel(groupGeneralWidget);
        groupAutotypeTitleLabel->setObjectName(QString::fromUtf8("groupAutotypeTitleLabel"));
        sizePolicy.setHeightForWidth(groupAutotypeTitleLabel->sizePolicy().hasHeightForWidth());
        groupAutotypeTitleLabel->setSizePolicy(sizePolicy);
        groupAutotypeTitleLabel->setFont(font2);
        groupAutotypeTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(groupAutotypeTitleLabel, 0, 0, 1, 1);

        groupSearchingLabel = new QLabel(groupGeneralWidget);
        groupSearchingLabel->setObjectName(QString::fromUtf8("groupSearchingLabel"));
        sizePolicy1.setHeightForWidth(groupSearchingLabel->sizePolicy().hasHeightForWidth());
        groupSearchingLabel->setSizePolicy(sizePolicy1);

        gridLayout_2->addWidget(groupSearchingLabel, 1, 2, 1, 1);

        groupExpirationTitleLabel = new QLabel(groupGeneralWidget);
        groupExpirationTitleLabel->setObjectName(QString::fromUtf8("groupExpirationTitleLabel"));
        sizePolicy.setHeightForWidth(groupExpirationTitleLabel->sizePolicy().hasHeightForWidth());
        groupExpirationTitleLabel->setSizePolicy(sizePolicy);
        groupExpirationTitleLabel->setFont(font2);
        groupExpirationTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(groupExpirationTitleLabel, 2, 0, 1, 1);

        groupSearchingTitleLabel = new QLabel(groupGeneralWidget);
        groupSearchingTitleLabel->setObjectName(QString::fromUtf8("groupSearchingTitleLabel"));
        sizePolicy.setHeightForWidth(groupSearchingTitleLabel->sizePolicy().hasHeightForWidth());
        groupSearchingTitleLabel->setSizePolicy(sizePolicy);
        groupSearchingTitleLabel->setFont(font2);
        groupSearchingTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_2->addWidget(groupSearchingTitleLabel, 1, 0, 1, 1);

        groupNotesTitleLabel = new QLabel(groupGeneralWidget);
        groupNotesTitleLabel->setObjectName(QString::fromUtf8("groupNotesTitleLabel"));
        sizePolicy.setHeightForWidth(groupNotesTitleLabel->sizePolicy().hasHeightForWidth());
        groupNotesTitleLabel->setSizePolicy(sizePolicy);
        groupNotesTitleLabel->setFont(font2);
        groupNotesTitleLabel->setAlignment(Qt::AlignRight|Qt::AlignTop|Qt::AlignTrailing);

        gridLayout_2->addWidget(groupNotesTitleLabel, 3, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(6);
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        toggleGroupNotesButton = new QToolButton(groupGeneralWidget);
        toggleGroupNotesButton->setObjectName(QString::fromUtf8("toggleGroupNotesButton"));
        toggleGroupNotesButton->setCheckable(true);

        horizontalLayout_4->addWidget(toggleGroupNotesButton, 0, Qt::AlignTop);

        groupNotesTextEdit = new QTextEdit(groupGeneralWidget);
        groupNotesTextEdit->setObjectName(QString::fromUtf8("groupNotesTextEdit"));
        groupNotesTextEdit->setFocusPolicy(Qt::ClickFocus);
        groupNotesTextEdit->setFrameShape(QFrame::NoFrame);
        groupNotesTextEdit->setFrameShadow(QFrame::Plain);
        groupNotesTextEdit->setLineWidth(0);
        groupNotesTextEdit->setTabChangesFocus(true);
        groupNotesTextEdit->setReadOnly(true);
        groupNotesTextEdit->setProperty("blendIn", QVariant(true));

        horizontalLayout_4->addWidget(groupNotesTextEdit);


        gridLayout_2->addLayout(horizontalLayout_4, 3, 1, 1, 2);

        groupAutotypeLabel = new QLabel(groupGeneralWidget);
        groupAutotypeLabel->setObjectName(QString::fromUtf8("groupAutotypeLabel"));
        sizePolicy1.setHeightForWidth(groupAutotypeLabel->sizePolicy().hasHeightForWidth());
        groupAutotypeLabel->setSizePolicy(sizePolicy1);

        gridLayout_2->addWidget(groupAutotypeLabel, 0, 2, 1, 1);

        gridLayout_2->setRowStretch(3, 1);

        verticalLayout_8->addWidget(groupGeneralWidget);

        groupTabWidget->addTab(groupGeneralTab, QString());
        groupShareTab = new QWidget();
        groupShareTab->setObjectName(QString::fromUtf8("groupShareTab"));
        verticalLayout_12 = new QVBoxLayout(groupShareTab);
        verticalLayout_12->setObjectName(QString::fromUtf8("verticalLayout_12"));
        groupShareWidget = new QWidget(groupShareTab);
        groupShareWidget->setObjectName(QString::fromUtf8("groupShareWidget"));
        gridLayout_4 = new QGridLayout(groupShareWidget);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        gridLayout_4->setContentsMargins(0, 0, 0, 0);
        groupSharePathLabel = new QLabel(groupShareWidget);
        groupSharePathLabel->setObjectName(QString::fromUtf8("groupSharePathLabel"));
        sizePolicy1.setHeightForWidth(groupSharePathLabel->sizePolicy().hasHeightForWidth());
        groupSharePathLabel->setSizePolicy(sizePolicy1);
        groupSharePathLabel->setText(QString::fromUtf8("<path>"));

        gridLayout_4->addWidget(groupSharePathLabel, 1, 1, 1, 1);

        groupShareTypeLabel = new QLabel(groupShareWidget);
        groupShareTypeLabel->setObjectName(QString::fromUtf8("groupShareTypeLabel"));
        groupShareTypeLabel->setFont(font2);
        groupShareTypeLabel->setText(QString::fromUtf8("<type>"));
        groupShareTypeLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_4->addWidget(groupShareTypeLabel, 1, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 147, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_4->addItem(verticalSpacer, 2, 1, 1, 1);


        verticalLayout_12->addWidget(groupShareWidget);

        groupTabWidget->addTab(groupShareTab, QString());

        verticalLayout_13->addWidget(groupTabWidget);

        stackedWidget->addWidget(pageGroup);

        verticalLayout_7->addWidget(stackedWidget);

        QWidget::setTabOrder(entryTotpButton, entryCloseButton);
        QWidget::setTabOrder(entryCloseButton, entryTabWidget);
        QWidget::setTabOrder(entryTabWidget, togglePasswordButton);
        QWidget::setTabOrder(togglePasswordButton, toggleEntryNotesButton);
        QWidget::setTabOrder(toggleEntryNotesButton, groupTabWidget);
        QWidget::setTabOrder(groupTabWidget, toggleGroupNotesButton);
        QWidget::setTabOrder(toggleGroupNotesButton, groupCloseButton);

        retranslateUi(EntryPreviewWidget);

        stackedWidget->setCurrentIndex(0);
        entryTabWidget->setCurrentIndex(0);
        groupTabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(EntryPreviewWidget);
    } // setupUi

    void retranslateUi(QWidget *EntryPreviewWidget)
    {
        searchIcon->setText(QCoreApplication::translate("EntryPreviewWidget", "Search", nullptr));
        clearIcon->setText(QCoreApplication::translate("EntryPreviewWidget", "Clear", nullptr));
        entryIcon->setText(QString());
#if QT_CONFIG(tooltip)
        entryTotpLabel->setToolTip(QCoreApplication::translate("EntryPreviewWidget", "Double click to copy to clipboard", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(tooltip)
        entryTotpButton->setToolTip(QCoreApplication::translate("EntryPreviewWidget", "Display current TOTP value", nullptr));
#endif // QT_CONFIG(tooltip)
        entryTotpButton->setText(QString());
#if QT_CONFIG(tooltip)
        entryCloseButton->setToolTip(QCoreApplication::translate("EntryPreviewWidget", "Close", nullptr));
#endif // QT_CONFIG(tooltip)
        entryCloseButton->setText(QString());
        entryUsernameTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Username", nullptr));
        toggleEntryNotesButton->setText(QString());
        entryTagsTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Tags", nullptr));
        toggleUsernameButton->setText(QString());
#if QT_CONFIG(accessibility)
        entryTagsList->setAccessibleName(QCoreApplication::translate("EntryPreviewWidget", "Tags list", nullptr));
#endif // QT_CONFIG(accessibility)
        entryPasswordTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Password", nullptr));
        entryNotesTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Notes", nullptr));
        entryExpirationTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Expiration", nullptr));
        entryUrlTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "URL", nullptr));
        entryTabWidget->setTabText(entryTabWidget->indexOf(entryGeneralTab), QCoreApplication::translate("EntryPreviewWidget", "General", nullptr));
        attachmentsTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Attachments", nullptr));
        attributesTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Attributes", nullptr));
        entryTabWidget->setTabText(entryTabWidget->indexOf(entryAdvancedTab), QCoreApplication::translate("EntryPreviewWidget", "Advanced", nullptr));
        entrySequenceTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Default Sequence", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = entryAutotypeTree->headerItem();
        ___qtreewidgetitem->setText(1, QCoreApplication::translate("EntryPreviewWidget", "Sequence", nullptr));
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("EntryPreviewWidget", "Window", nullptr));
        entryTabWidget->setTabText(entryTabWidget->indexOf(entryAutotypeTab), QCoreApplication::translate("EntryPreviewWidget", "Autotype", nullptr));
        groupIcon->setText(QString());
#if QT_CONFIG(tooltip)
        groupCloseButton->setToolTip(QCoreApplication::translate("EntryPreviewWidget", "Close", nullptr));
#endif // QT_CONFIG(tooltip)
        groupCloseButton->setText(QString());
        groupAutotypeTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Autotype", nullptr));
        groupExpirationTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Expiration", nullptr));
        groupSearchingTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Searching", nullptr));
        groupNotesTitleLabel->setText(QCoreApplication::translate("EntryPreviewWidget", "Notes", nullptr));
        toggleGroupNotesButton->setText(QString());
        groupTabWidget->setTabText(groupTabWidget->indexOf(groupGeneralTab), QCoreApplication::translate("EntryPreviewWidget", "General", nullptr));
        groupTabWidget->setTabText(groupTabWidget->indexOf(groupShareTab), QCoreApplication::translate("EntryPreviewWidget", "Share", nullptr));
        (void)EntryPreviewWidget;
    } // retranslateUi

};

namespace Ui {
    class EntryPreviewWidget: public Ui_EntryPreviewWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ENTRYPREVIEWWIDGET_H
