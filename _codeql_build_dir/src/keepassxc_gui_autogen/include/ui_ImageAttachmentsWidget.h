/********************************************************************************
** Form generated from reading UI file 'ImageAttachmentsWidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.13
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_IMAGEATTACHMENTSWIDGET_H
#define UI_IMAGEATTACHMENTSWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <gui/entry/attachments/ImageAttachmentsView.h>

QT_BEGIN_NAMESPACE

class Ui_ImageAttachmentsWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *zoomLabel;
    QComboBox *zoomComboBox;
    QSpacerItem *horizontalSpacer;
    QFrame *line;
    ImageAttachmentsView *imagesView;

    void setupUi(QWidget *ImageAttachmentsWidget)
    {
        if (ImageAttachmentsWidget->objectName().isEmpty())
            ImageAttachmentsWidget->setObjectName(QString::fromUtf8("ImageAttachmentsWidget"));
        ImageAttachmentsWidget->resize(400, 300);
        verticalLayout = new QVBoxLayout(ImageAttachmentsWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        zoomLabel = new QLabel(ImageAttachmentsWidget);
        zoomLabel->setObjectName(QString::fromUtf8("zoomLabel"));

        horizontalLayout->addWidget(zoomLabel);

        zoomComboBox = new QComboBox(ImageAttachmentsWidget);
        zoomComboBox->setObjectName(QString::fromUtf8("zoomComboBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(zoomComboBox->sizePolicy().hasHeightForWidth());
        zoomComboBox->setSizePolicy(sizePolicy);
        zoomComboBox->setEditable(true);

        horizontalLayout->addWidget(zoomComboBox);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        line = new QFrame(ImageAttachmentsWidget);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout->addWidget(line);

        imagesView = new ImageAttachmentsView(ImageAttachmentsWidget);
        imagesView->setObjectName(QString::fromUtf8("imagesView"));

        verticalLayout->addWidget(imagesView);


        retranslateUi(ImageAttachmentsWidget);

        QMetaObject::connectSlotsByName(ImageAttachmentsWidget);
    } // setupUi

    void retranslateUi(QWidget *ImageAttachmentsWidget)
    {
        ImageAttachmentsWidget->setWindowTitle(QString());
        zoomLabel->setText(QCoreApplication::translate("ImageAttachmentsWidget", "Zoom:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ImageAttachmentsWidget: public Ui_ImageAttachmentsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_IMAGEATTACHMENTSWIDGET_H
