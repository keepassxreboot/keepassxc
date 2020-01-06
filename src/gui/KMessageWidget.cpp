/* This file is part of the KDE libraries
 *
 * Copyright (c) 2011 Aurélien Gâteau <agateau@kde.org>
 * Copyright (c) 2014 Dominik Haumann <dhaumann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include "KMessageWidget.h"

#include "core/FilePath.h"
#include "core/Global.h"

#include <QAction>
#include <QEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QShowEvent>
#include <QTimeLine>
#include <QToolButton>
#include <QStyle>
#include <QtGui/QBitmap>

//---------------------------------------------------------------------
// KMessageWidgetPrivate
//---------------------------------------------------------------------
class KMessageWidgetPrivate
{
public:
    void init(KMessageWidget *);

    KMessageWidget *q;
    QFrame *content;
    QLabel *iconLabel;
    QLabel *textLabel;
    QToolButton *closeButton;
    QTimeLine *timeLine;
    QIcon icon;
    QPixmap closeButtonPixmap;

    KMessageWidget::MessageType messageType;
    bool wordWrap;
    QList<QToolButton *> buttons;
    QPixmap contentSnapShot;

    void createLayout();
    void updateSnapShot();
    void updateLayout();
    void slotTimeLineChanged(qreal);
    void slotTimeLineFinished();

    int bestContentHeight() const;
};

void KMessageWidgetPrivate::init(KMessageWidget *q_ptr)
{
    q = q_ptr;

    q->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    timeLine = new QTimeLine(500, q);
    QObject::connect(timeLine, SIGNAL(valueChanged(qreal)), q, SLOT(slotTimeLineChanged(qreal)));
    QObject::connect(timeLine, SIGNAL(finished()), q, SLOT(slotTimeLineFinished()));

    content = new QFrame(q);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    wordWrap = false;

    iconLabel = new QLabel(content);
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    iconLabel->hide();

    textLabel = new QLabel(content);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    textLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    QObject::connect(textLabel, SIGNAL(linkActivated(QString)), q, SIGNAL(linkActivated(QString)));
    QObject::connect(textLabel, SIGNAL(linkHovered(QString)), q, SIGNAL(linkHovered(QString)));

    QAction *closeAction = new QAction(q);
    closeAction->setText(KMessageWidget::tr("&Close"));
    closeAction->setToolTip(KMessageWidget::tr("Close message"));
    closeAction->setIcon(FilePath::instance()->icon("actions", "message-close"));

    QObject::connect(closeAction, SIGNAL(triggered(bool)), q, SLOT(animatedHide()));

    closeButton = new QToolButton(content);
    closeButton->setAutoRaise(true);
    closeButton->setDefaultAction(closeAction);
    closeButtonPixmap = QPixmap(closeButton->icon().pixmap(closeButton->icon().actualSize(QSize(16, 16))));

    q->setMessageType(KMessageWidget::Information);
}

void KMessageWidgetPrivate::createLayout()
{
    delete content->layout();

    content->resize(q->size());

    qDeleteAll(buttons);
    buttons.clear();

    const auto actions = q->actions();
    for (QAction *action: actions) {
        QToolButton *button = new QToolButton(content);
        button->setDefaultAction(action);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        buttons.append(button);
    }

    // AutoRaise reduces visual clutter, but we don't want to turn it on if
    // there are other buttons, otherwise the close button will look different
    // from the others.
    closeButton->setAutoRaise(buttons.isEmpty());

    if (wordWrap) {
        QGridLayout *layout = new QGridLayout(content);
        // Set alignment to make sure icon does not move down if text wraps
        layout->addWidget(iconLabel, 0, 0, 1, 1, Qt::AlignHCenter | Qt::AlignTop);
        layout->addWidget(textLabel, 0, 1);

        QHBoxLayout *buttonLayout = new QHBoxLayout;
        buttonLayout->addStretch();
        for (QToolButton* button: asConst(buttons)) {
            // For some reason, calling show() is necessary if wordwrap is true,
            // otherwise the buttons do not show up. It is not needed if
            // wordwrap is false.
            button->show();
            buttonLayout->addWidget(button);
        }
        buttonLayout->addWidget(closeButton);
        layout->addItem(buttonLayout, 1, 0, 1, 2);
    } else {
        QHBoxLayout *layout = new QHBoxLayout(content);
        layout->addWidget(iconLabel);
        layout->addWidget(textLabel);

        for (QToolButton* button: asConst(buttons)) {
            layout->addWidget(button);
        }

        layout->addWidget(closeButton);
    };

    if (q->isVisible()) {
        q->setFixedHeight(content->sizeHint().height());
    }
    q->updateGeometry();
}

void KMessageWidgetPrivate::updateLayout()
{
    if (content->layout()) {
        createLayout();
    }
}

void KMessageWidgetPrivate::updateSnapShot()
{
    // Attention: updateSnapShot calls QWidget::render(), which causes the whole
    // window layouts to be activated. Calling this method from resizeEvent()
    // can lead to infinite recursion, see:
    // https://bugs.kde.org/show_bug.cgi?id=311336
    contentSnapShot = QPixmap(content->size() * q->devicePixelRatio());
    contentSnapShot.setDevicePixelRatio(q->devicePixelRatio());
    contentSnapShot.fill(Qt::transparent);
    content->render(&contentSnapShot, QPoint(), QRegion(), QWidget::DrawChildren);
}

void KMessageWidgetPrivate::slotTimeLineChanged(qreal value)
{
    q->setFixedHeight(qMin(value * 2, qreal(1.0)) * content->height());
    q->update();
}

void KMessageWidgetPrivate::slotTimeLineFinished()
{
    if (timeLine->direction() == QTimeLine::Forward) {
        // Show
        // We set the whole geometry here, because it may be wrong if a
        // KMessageWidget is shown right when the toplevel window is created.
        content->setGeometry(0, 0, q->width(), bestContentHeight());

        // notify about finished animation
        emit q->showAnimationFinished();
    } else {
        // hide and notify about finished animation
        q->hide();
        emit q->hideAnimationFinished();
    }
}

int KMessageWidgetPrivate::bestContentHeight() const
{
    int height = content->heightForWidth(q->width());
    if (height == -1) {
        height = content->sizeHint().height();
    }
    return height;
}

//---------------------------------------------------------------------
// KMessageWidget
//---------------------------------------------------------------------
KMessageWidget::KMessageWidget(QWidget *parent)
: QFrame(parent)
, d(new KMessageWidgetPrivate)
{
    d->init(this);
}

KMessageWidget::KMessageWidget(const QString &text, QWidget *parent)
: QFrame(parent)
, d(new KMessageWidgetPrivate)
{
    d->init(this);
    setText(text);
}

KMessageWidget::~KMessageWidget()
{
    delete d;
}

QString KMessageWidget::text() const
{
    return d->textLabel->text();
}

void KMessageWidget::setText(const QString &text)
{
    d->textLabel->setText(text);
    updateGeometry();
}

KMessageWidget::MessageType KMessageWidget::messageType() const
{
    return d->messageType;
}

void KMessageWidget::setMessageType(KMessageWidget::MessageType type)
{
    d->messageType = type;
    QColor bg0, bg1, bg2, border;
    QColor fg = QColor(238, 238, 238);
    switch (type) {
    case Positive:
        bg1.setRgb(37, 163, 83);
        break;
    case Information:
        bg1.setRgb(24, 187, 242);
        break;
    case Warning:
        bg1.setRgb(252, 193, 57);
        fg = QColor(48, 48, 48);
        break;
    case Error:
        bg1.setRgb(198, 69, 21);
        break;
    }

    // Colors
    bg0 = bg1.lighter(105);
    bg2 = bg1.darker(105);
    border = bg1.darker(115);

    // Tint close icon
    auto closeButtonPixmap = d->closeButtonPixmap;
    QPainter painter;
    painter.begin(&closeButtonPixmap);
    painter.setRenderHints(QPainter::HighQualityAntialiasing);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(QRect(0, 0, 16, 16), fg);
    painter.end();
    d->closeButton->setIcon(closeButtonPixmap);
    d->closeButton->setStyleSheet(QStringLiteral("QToolButton {"
                                  "  background: transparent;"
                                  "  border-radius: 2px;"
                                  "  border: none; }"
                                  "QToolButton:hover, QToolButton:focus {"
                                  "  border: 1px solid %1; }").arg(fg.name()));

    d->content->setStyleSheet(
        QStringLiteral(".QFrame {"
        "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,"
        "    stop: 0 %1,"
        "    stop: 0.1 %2,"
        "    stop: 1.0 %3);"
        "    border-radius: 2px;"
        "    border: 1px solid %4;"
        "    margin: %5px;"
        "    padding: 5px;"
        "}"
        ".QLabel { color: %6; }"
        )
        .arg(bg0.name(),
             bg1.name(),
             bg2.name(),
             border.name())
        // DefaultFrameWidth returns the size of the external margin + border width. We know our border is 1px,
        // so we subtract this from the frame normal QStyle FrameWidth to get our margin
        .arg(style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, this) - 1)
        .arg(fg.name())
    );
}

QSize KMessageWidget::sizeHint() const
{
    ensurePolished();
    return d->content->sizeHint();
}

QSize KMessageWidget::minimumSizeHint() const
{
    ensurePolished();
    return d->content->minimumSizeHint();
}

bool KMessageWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Polish && !d->content->layout()) {
        d->createLayout();
    }
    return QFrame::event(event);
}

void KMessageWidget::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);

    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->content->resize(width(), d->bestContentHeight());
    }
}

int KMessageWidget::heightForWidth(int width) const
{
    ensurePolished();
    return d->content->heightForWidth(width);
}

void KMessageWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);
    if (d->timeLine->state() == QTimeLine::Running) {
        QPainter painter(this);
        painter.setOpacity(d->timeLine->currentValue() * d->timeLine->currentValue());
        painter.drawPixmap(0, 0, d->contentSnapShot);
    }
}

bool KMessageWidget::wordWrap() const
{
    return d->wordWrap;
}

void KMessageWidget::setWordWrap(bool wordWrap)
{
    d->wordWrap = wordWrap;
    d->textLabel->setWordWrap(wordWrap);
    QSizePolicy policy = sizePolicy();
    policy.setHeightForWidth(wordWrap);
    setSizePolicy(policy);
    d->updateLayout();
    // Without this, when user does wordWrap -> !wordWrap -> wordWrap, a minimum
    // height is set, causing the widget to be too high.
    // Mostly visible in test programs.
    if (wordWrap) {
        setMinimumHeight(0);
    }
}

bool KMessageWidget::isCloseButtonVisible() const
{
    return d->closeButton->isVisible();
}

void KMessageWidget::setCloseButtonVisible(bool show)
{
    d->closeButton->setVisible(show);
    updateGeometry();
}

void KMessageWidget::addAction(QAction *action)
{
    QFrame::addAction(action);
    d->updateLayout();
}

void KMessageWidget::removeAction(QAction *action)
{
    QFrame::removeAction(action);
    d->updateLayout();
}

void KMessageWidget::animatedShow()
{
    if (!style()->styleHint(QStyle::SH_Widget_Animate, nullptr, this)) {
        show();
        emit showAnimationFinished();
        return;
    }

    if (isVisible()) {
        return;
    }

    QFrame::show();
    setFixedHeight(0);
    int wantedHeight = d->bestContentHeight();
    d->content->setGeometry(0, -wantedHeight, width(), wantedHeight);

    d->updateSnapShot();

    d->timeLine->setDirection(QTimeLine::Forward);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->timeLine->start();
    }
}

void KMessageWidget::animatedHide()
{
    if (!style()->styleHint(QStyle::SH_Widget_Animate, nullptr, this)) {
        hide();
        emit hideAnimationFinished();
        return;
    }

    if (!isVisible()) {
        hide();
        return;
    }

    d->content->move(0, -d->content->height());
    d->updateSnapShot();

    d->timeLine->setDirection(QTimeLine::Backward);
    if (d->timeLine->state() == QTimeLine::NotRunning) {
        d->timeLine->start();
    }
}

bool KMessageWidget::isHideAnimationRunning() const
{
    return (d->timeLine->direction() == QTimeLine::Backward)
    && (d->timeLine->state() == QTimeLine::Running);
}

bool KMessageWidget::isShowAnimationRunning() const
{
    return (d->timeLine->direction() == QTimeLine::Forward)
    && (d->timeLine->state() == QTimeLine::Running);
}

QIcon KMessageWidget::icon() const
{
    return d->icon;
}

void KMessageWidget::setIcon(const QIcon &icon)
{
    d->icon = icon;
    if (d->icon.isNull()) {
        d->iconLabel->hide();
    } else {
        const int size = style()->pixelMetric(QStyle::PM_ToolBarIconSize);
        d->iconLabel->setPixmap(d->icon.pixmap(size));
        d->iconLabel->show();
    }
}

#include "moc_KMessageWidget.cpp"
