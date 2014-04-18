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

#include "qbutton.h"

#include <QToolBar>
#include <QToolButton>
#include <QPushButton>
#include <QVBoxLayout>

class QButtonPrivate : public QObject
{
public:
    QButtonPrivate(QButton *button, QAbstractButton *abstractButton)
        : QObject(button), abstractButton(abstractButton) {}
    QPointer<QAbstractButton> abstractButton;
};

QButton::QButton(QWidget *parent, BezelStyle) : QWidget(parent)
{
    QAbstractButton *button = 0;
    if (qobject_cast<QToolBar*>(parent))
        button = new QToolButton(this);
    else
        button = new QPushButton(this);
    connect(button, SIGNAL(clicked()),
            this, SIGNAL(clicked()));
    pimpl = new QButtonPrivate(this, button);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(button);
}

void QButton::setText(const QString &text)
{
    Q_ASSERT(pimpl);
    if (pimpl)
        pimpl->abstractButton->setText(text);
}

void QButton::setImage(const QPixmap &image)
{
    Q_ASSERT(pimpl);
    if (pimpl)
        pimpl->abstractButton->setIcon(image);
}

void QButton::setChecked(bool checked)
{
    Q_ASSERT(pimpl);
    if (pimpl)
        pimpl->abstractButton->setChecked(checked);
}

void QButton::setCheckable(bool checkable)
{
    Q_ASSERT(pimpl);
    if (pimpl)
        pimpl->abstractButton->setCheckable(checkable);
}

bool QButton::isChecked()
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return false;

    return pimpl->abstractButton->isChecked();
}
