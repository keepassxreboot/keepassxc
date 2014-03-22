/*
Copyright (C) 2011 by Mike McQuaid

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "qsearchfield.h"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QToolButton>
#include <QStyle>
#include <QApplication>
#include <QDesktopWidget>

#include <QDir>
#include <QDebug>

class QSearchFieldPrivate : public QObject
{
public:
    QSearchFieldPrivate(QSearchField *searchField, QLineEdit *lineEdit, QToolButton *clearButton, QToolButton *searchButton)
        : QObject(searchField), lineEdit(lineEdit), clearButton(clearButton), searchButton(searchButton) {}

    int lineEditFrameWidth() const {
        return lineEdit->style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    }

    int clearButtonPaddedWidth() const {
        return clearButton->width() + lineEditFrameWidth() * 2;
    }

    int clearButtonPaddedHeight() const {
        return clearButton->height() + lineEditFrameWidth() * 2;
    }

    int searchButtonPaddedWidth() const {
        return searchButton->width() + lineEditFrameWidth() * 2;
    }

    int searchButtonPaddedHeight() const {
        return searchButton->height() + lineEditFrameWidth() * 2;
    }

    QPointer<QLineEdit> lineEdit;
    QPointer<QToolButton> clearButton;
    QPointer<QToolButton> searchButton;
    QPointer<QMenu> searchMenu;
};

QSearchField::QSearchField(QWidget *parent) : QWidget(parent)
{
    QLineEdit *lineEdit = new QLineEdit(this);
    connect(lineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(textChanged(QString)));
    connect(lineEdit, SIGNAL(editingFinished()),
            this, SIGNAL(editingFinished()));
    connect(lineEdit, SIGNAL(returnPressed()),
            this, SIGNAL(returnPressed()));
    connect(lineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(setText(QString)));

    int iconsize = style()->pixelMetric(QStyle::PM_SmallIconSize);
    QToolButton *clearButton = new QToolButton(this);
    QIcon clearIcon = QIcon::fromTheme(QLatin1String("edit-clear"),
                                       QIcon(QLatin1String(":/Qocoa/qsearchfield_nonmac_clear.png")));
    clearButton->setIcon(clearIcon);
    clearButton->setIconSize(QSize(iconsize, iconsize));
    clearButton->setFixedSize(QSize(iconsize, iconsize));
    clearButton->setStyleSheet("border: none;");
    clearButton->hide();
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    QToolButton *searchButton = new QToolButton(this);
    QIcon searchIcon = QIcon(QLatin1String(":/Qocoa/qsearchfield_nonmac_magnifier.png"));
    searchButton->setIcon(searchIcon);
    searchButton->setIconSize(QSize(iconsize, iconsize));
    searchButton->setFixedSize(QSize(iconsize, iconsize));
    searchButton->setStyleSheet("border: none;");
    searchButton->setPopupMode(QToolButton::InstantPopup);
    searchButton->setEnabled(false);
    connect(searchButton, SIGNAL(clicked()), this, SLOT(popupMenu()));

    pimpl = new QSearchFieldPrivate(this, lineEdit, clearButton, searchButton);

    lineEdit->setStyleSheet(QString("QLineEdit { padding-left: %1px; padding-right: %2px; } ")
                            .arg(pimpl->searchButtonPaddedWidth())
                            .arg(pimpl->clearButtonPaddedWidth()));
    const int width = qMax(lineEdit->minimumSizeHint().width(), pimpl->clearButtonPaddedWidth() + pimpl->searchButtonPaddedWidth());
    const int height = qMax(lineEdit->minimumSizeHint().height(),
                       qMax(pimpl->clearButtonPaddedHeight(),
                            pimpl->searchButtonPaddedHeight()));
    lineEdit->setMinimumSize(width, height);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(lineEdit);
}

void QSearchField::setMenu(QMenu *menu)
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    pimpl->searchMenu = menu;

    QIcon searchIcon = menu ? QIcon(QLatin1String(":/Qocoa/qsearchfield_nonmac_magnifier_menu.png"))
                            : QIcon(QLatin1String(":/Qocoa/qsearchfield_nonmac_magnifier.png"));
    pimpl->searchButton->setIcon(searchIcon);
    pimpl->searchButton->setEnabled(isEnabled() && menu);
}

void QSearchField::popupMenu()
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    if (pimpl->searchMenu) {
        const QRect screenRect = qApp->desktop()->availableGeometry(pimpl->searchButton);
        const QSize sizeHint = pimpl->searchMenu->sizeHint();
        const QRect rect = pimpl->searchButton->rect();
        const int x = pimpl->searchButton->isRightToLeft()
                ? rect.right() - sizeHint.width()
                : rect.left();
        const int y = pimpl->searchButton->mapToGlobal(QPoint(0, rect.bottom())).y() + sizeHint.height() <= screenRect.height()
                ? rect.bottom()
                : rect.top() - sizeHint.height();
        QPoint point = pimpl->searchButton->mapToGlobal(QPoint(x, y));
        point.rx() = qMax(screenRect.left(), qMin(point.x(), screenRect.right() - sizeHint.width()));
        point.ry() += 1;

        pimpl->searchMenu->popup(point);
    }
}

void QSearchField::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange) {
        Q_ASSERT(pimpl);
        if (!pimpl)
            return;

        const bool enabled = isEnabled();
        pimpl->searchButton->setEnabled(enabled && pimpl->searchMenu);
        pimpl->lineEdit->setEnabled(enabled);
        pimpl->clearButton->setEnabled(enabled);
    }
    QWidget::changeEvent(event);
}

void QSearchField::setText(const QString &text)
{
    Q_ASSERT(pimpl && pimpl->clearButton && pimpl->lineEdit);
    if (!(pimpl && pimpl->clearButton && pimpl->lineEdit))
        return;

    pimpl->clearButton->setVisible(!text.isEmpty());

    if (text != this->text())
        pimpl->lineEdit->setText(text);
}

void QSearchField::setPlaceholderText(const QString &text)
{
    Q_ASSERT(pimpl && pimpl->lineEdit);
    if (!(pimpl && pimpl->lineEdit))
        return;

#if QT_VERSION >= 0x040700
    pimpl->lineEdit->setPlaceholderText(text);
#endif
}

void QSearchField::clear()
{
    Q_ASSERT(pimpl && pimpl->lineEdit);
    if (!(pimpl && pimpl->lineEdit))
        return;

    pimpl->lineEdit->clear();
}

void QSearchField::selectAll()
{
    Q_ASSERT(pimpl && pimpl->lineEdit);
    if (!(pimpl && pimpl->lineEdit))
        return;

    pimpl->lineEdit->selectAll();
}

QString QSearchField::text() const
{
    Q_ASSERT(pimpl && pimpl->lineEdit);
    if (!(pimpl && pimpl->lineEdit))
        return QString();

    return pimpl->lineEdit->text();
}

QString QSearchField::placeholderText() const {
    Q_ASSERT(pimpl && pimpl->lineEdit);
    if (!(pimpl && pimpl->lineEdit))
        return QString();

#if QT_VERSION >= 0x040700
    return pimpl->lineEdit->placeholderText();
#else
    return QString();
#endif
}

void QSearchField::setFocus(Qt::FocusReason reason)
{
    Q_ASSERT(pimpl && pimpl->lineEdit);
    if (pimpl && pimpl->lineEdit)
        pimpl->lineEdit->setFocus(reason);
}

void QSearchField::setFocus()
{
    setFocus(Qt::OtherFocusReason);
}

void QSearchField::resizeEvent(QResizeEvent *resizeEvent)
{
    Q_ASSERT(pimpl && pimpl->clearButton && pimpl->lineEdit);
    if (!(pimpl && pimpl->clearButton && pimpl->lineEdit))
        return;

    QWidget::resizeEvent(resizeEvent);
    const int x = width() - pimpl->clearButtonPaddedWidth();
    const int y = (height() - pimpl->clearButton->height())/2;
    pimpl->clearButton->move(x, y);

    pimpl->searchButton->move(pimpl->lineEditFrameWidth() * 2,
                              (height() - pimpl->searchButton->height())/2);
}
