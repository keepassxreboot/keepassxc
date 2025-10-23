/********************************************************************************
** Form generated from reading UI file 'ReportsWidgetHibp.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REPORTSWIDGETHIBP_H
#define UI_REPORTSWIDGETHIBP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableView>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ReportsWidgetHibp
{
public:
    QVBoxLayout *verticalLayout;
    QStackedWidget *stackedWidget;
    QWidget *confirmation;
    QVBoxLayout *verticalLayout_2;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QHBoxLayout *horizontalLayout;
    QPushButton *validationButton;
    QSpacerItem *verticalSpacer;
    QProgressBar *progressBar;
    QWidget *resultsTable;
    QVBoxLayout *verticalLayout_3;
    QTableView *hibpTableView;
    QCheckBox *showKnownBadCheckBox;
    QWidget *noNetwork;
    QVBoxLayout *verticalLayout_5;
    QSpacerItem *verticalSpacer_4;
    QLabel *networkNoticeLabel;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QWidget *ReportsWidgetHibp)
    {
        if (ReportsWidgetHibp->objectName().isEmpty())
            ReportsWidgetHibp->setObjectName(QString::fromUtf8("ReportsWidgetHibp"));
        ReportsWidgetHibp->resize(545, 379);
        verticalLayout = new QVBoxLayout(ReportsWidgetHibp);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        stackedWidget = new QStackedWidget(ReportsWidgetHibp);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        confirmation = new QWidget();
        confirmation->setObjectName(QString::fromUtf8("confirmation"));
        verticalLayout_2 = new QVBoxLayout(confirmation);
        verticalLayout_2->setSpacing(15);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_3);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label = new QLabel(confirmation);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMaximumSize(QSize(450, 16777215));
        label->setWordWrap(true);

        horizontalLayout_2->addWidget(label);


        verticalLayout_2->addLayout(horizontalLayout_2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(-1, 0, -1, -1);
        validationButton = new QPushButton(confirmation);
        validationButton->setObjectName(QString::fromUtf8("validationButton"));
        validationButton->setMaximumSize(QSize(275, 16777215));

        horizontalLayout->addWidget(validationButton);


        verticalLayout_2->addLayout(horizontalLayout);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer);

        progressBar = new QProgressBar(confirmation);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);

        verticalLayout_2->addWidget(progressBar);

        stackedWidget->addWidget(confirmation);
        resultsTable = new QWidget();
        resultsTable->setObjectName(QString::fromUtf8("resultsTable"));
        verticalLayout_3 = new QVBoxLayout(resultsTable);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        hibpTableView = new QTableView(resultsTable);
        hibpTableView->setObjectName(QString::fromUtf8("hibpTableView"));
        hibpTableView->setContextMenuPolicy(Qt::CustomContextMenu);
        hibpTableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        hibpTableView->setProperty("showDropIndicator", QVariant(false));
        hibpTableView->setAlternatingRowColors(true);
        hibpTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        hibpTableView->setTextElideMode(Qt::ElideMiddle);
        hibpTableView->setSortingEnabled(true);
        hibpTableView->horizontalHeader()->setStretchLastSection(true);
        hibpTableView->verticalHeader()->setVisible(false);

        verticalLayout_3->addWidget(hibpTableView);

        showKnownBadCheckBox = new QCheckBox(resultsTable);
        showKnownBadCheckBox->setObjectName(QString::fromUtf8("showKnownBadCheckBox"));

        verticalLayout_3->addWidget(showKnownBadCheckBox);

        stackedWidget->addWidget(resultsTable);
        noNetwork = new QWidget();
        noNetwork->setObjectName(QString::fromUtf8("noNetwork"));
        verticalLayout_5 = new QVBoxLayout(noNetwork);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_4);

        networkNoticeLabel = new QLabel(noNetwork);
        networkNoticeLabel->setObjectName(QString::fromUtf8("networkNoticeLabel"));
        networkNoticeLabel->setMaximumSize(QSize(450, 16777215));
        networkNoticeLabel->setWordWrap(true);

        verticalLayout_5->addWidget(networkNoticeLabel);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_2);

        stackedWidget->addWidget(noNetwork);

        verticalLayout->addWidget(stackedWidget);

        QWidget::setTabOrder(hibpTableView, showKnownBadCheckBox);
        QWidget::setTabOrder(showKnownBadCheckBox, validationButton);

        retranslateUi(ReportsWidgetHibp);

        stackedWidget->setCurrentIndex(1);
        validationButton->setDefault(true);


        QMetaObject::connectSlotsByName(ReportsWidgetHibp);
    } // setupUi

    void retranslateUi(QWidget *ReportsWidgetHibp)
    {
        label->setText(QCoreApplication::translate("ReportsWidgetHibp", "CAUTION: This report requires sending information to the Have I Been Pwned online service (https://haveibeenpwned.com). If you proceed, your database passwords will be cryptographically hashed and the first five characters of those hashes will be sent securely to this service. Your database remains secure and cannot be reconstituted from this information. However, the number of passwords you send and your IP address will be exposed to this service.", nullptr));
        validationButton->setText(QCoreApplication::translate("ReportsWidgetHibp", "Perform Online Analysis", nullptr));
        showKnownBadCheckBox->setText(QCoreApplication::translate("ReportsWidgetHibp", "Also show entries that have been excluded from reports", nullptr));
        networkNoticeLabel->setText(QCoreApplication::translate("ReportsWidgetHibp", "This build of KeePassXC does not have network functions. Networking is required to check your passwords against Have I Been Pwned databases.", nullptr));
        (void)ReportsWidgetHibp;
    } // retranslateUi

};

namespace Ui {
    class ReportsWidgetHibp: public Ui_ReportsWidgetHibp {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REPORTSWIDGETHIBP_H
