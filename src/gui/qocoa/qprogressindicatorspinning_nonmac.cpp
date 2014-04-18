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

#include "qprogressindicatorspinning.h"

#include <QVBoxLayout>
#include <QMovie>
#include <QLabel>

class QProgressIndicatorSpinningPrivate : public QObject
{
public:
    QProgressIndicatorSpinningPrivate(QProgressIndicatorSpinning *qProgressIndicatorSpinning,
                                      QMovie *movie)
        : QObject(qProgressIndicatorSpinning), movie(movie) {}

    QPointer<QMovie> movie;
};

QProgressIndicatorSpinning::QProgressIndicatorSpinning(QWidget *parent,
                                                       Thickness thickness)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    QSize size(thickness, thickness);
    QMovie *movie = new QMovie(this);
    movie->setFileName(":/Qocoa/qprogressindicatorspinning_nonmac.gif");
    movie->setScaledSize(size);
    // Roughly match OSX speed.
    movie->setSpeed(200);
    pimpl = new QProgressIndicatorSpinningPrivate(this, movie);

    QLabel *label = new QLabel(this);
    label->setMovie(movie);

    layout->addWidget(label);
    setFixedSize(size);
}


void QProgressIndicatorSpinning::animate(bool animate)
{
    Q_ASSERT(pimpl && pimpl->movie);
    if (!(pimpl && pimpl->movie))
        return;

    if (animate)
        pimpl->movie->start();
    else
        pimpl->movie->stop();
}
