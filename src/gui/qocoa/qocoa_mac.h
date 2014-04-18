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

#include <AppKit/NSImage.h>
#include <Foundation/NSString.h>
#include <QString>
#include <QVBoxLayout>
#include <QMacCocoaViewContainer>

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
#include <qmacfunctions.h>
#endif

static inline NSString* fromQString(const QString &string)
{
    const QByteArray utf8 = string.toUtf8();
    const char* cString = utf8.constData();
    return [[NSString alloc] initWithUTF8String:cString];
}

static inline QString toQString(NSString *string)
{
    if (!string)
        return QString();
    return QString::fromUtf8([string UTF8String]);
}

static inline NSImage* fromQPixmap(const QPixmap &pixmap)
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    CGImageRef cgImage = pixmap.toMacCGImageRef();
#else
    CGImageRef cgImage = QtMac::toCGImageRef(pixmap);
#endif
    return [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
}

static inline void setupLayout(NSView *cocoaView, QWidget *parent)
{
    parent->setAttribute(Qt::WA_NativeWindow);
    QVBoxLayout *layout = new QVBoxLayout(parent);
    layout->setMargin(0);
    layout->addWidget(new QMacCocoaViewContainer(cocoaView, parent));
}
