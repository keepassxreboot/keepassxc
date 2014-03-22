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

#include "qocoa_mac.h"

#import "Foundation/NSAutoreleasePool.h"
#import "AppKit/NSButton.h"
#import "AppKit/NSFont.h"

class QButtonPrivate : public QObject
{
public:
    QButtonPrivate(QButton *qButton, NSButton *nsButton, QButton::BezelStyle bezelStyle)
        : QObject(qButton), qButton(qButton), nsButton(nsButton)
    {
        switch(bezelStyle) {
            case QButton::Disclosure:
            case QButton::Circular:
#ifdef __MAC_10_7
            case QButton::Inline:
#endif
            case QButton::RoundedDisclosure:
            case QButton::HelpButton:
                [nsButton setTitle:@""];
            default:
                break;
        }

        NSFont* font = 0;
        switch(bezelStyle) {
            case QButton::RoundRect:
                font = [NSFont fontWithName:@"Lucida Grande" size:12];
                break;

            case QButton::Recessed:
                font = [NSFont fontWithName:@"Lucida Grande Bold" size:12];
                break;

#ifdef __MAC_10_7
            case QButton::Inline:
                font = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:NSSmallControlSize]];
                break;
#endif

            default:
                font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:NSRegularControlSize]];
                break;
        }
        [nsButton setFont:font];

        switch(bezelStyle) {
            case QButton::Rounded:
                qButton->setMinimumWidth(40);
                qButton->setFixedHeight(24);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QButton::RegularSquare:
            case QButton::TexturedSquare:
                qButton->setMinimumSize(14, 23);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QButton::ShadowlessSquare:
                qButton->setMinimumSize(5, 25);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QButton::SmallSquare:
                qButton->setMinimumSize(4, 21);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QButton::TexturedRounded:
                qButton->setMinimumSize(10, 22);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QButton::RoundRect:
            case QButton::Recessed:
                qButton->setMinimumWidth(16);
                qButton->setFixedHeight(18);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QButton::Disclosure:
                qButton->setMinimumWidth(13);
                qButton->setFixedHeight(13);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
            case QButton::Circular:
                qButton->setMinimumSize(16, 16);
                qButton->setMaximumHeight(40);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                break;
            case QButton::HelpButton:
            case QButton::RoundedDisclosure:
                qButton->setMinimumWidth(22);
                qButton->setFixedHeight(22);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
#ifdef __MAC_10_7
            case QButton::Inline:
                qButton->setMinimumWidth(10);
                qButton->setFixedHeight(16);
                qButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                break;
#endif
        }

        switch(bezelStyle) {
            case QButton::Recessed:
                [nsButton setButtonType:NSPushOnPushOffButton];
            case QButton::Disclosure:
                [nsButton setButtonType:NSOnOffButton];
            default:
                [nsButton setButtonType:NSMomentaryPushInButton];
        }

        [nsButton setBezelStyle:bezelStyle];
    }

    void clicked()
    {
        emit qButton->clicked(qButton->isChecked());
    }

    ~QButtonPrivate() {
        [[nsButton target] release];
        [nsButton setTarget:nil];
    }

    QButton *qButton;
    NSButton *nsButton;
};

@interface QButtonTarget : NSObject
{
@public
    QPointer<QButtonPrivate> pimpl;
}
-(void)clicked;
@end

@implementation QButtonTarget
-(void)clicked {
    Q_ASSERT(pimpl);
    if (pimpl)
        pimpl->clicked();
}
@end

QButton::QButton(QWidget *parent, BezelStyle bezelStyle) : QWidget(parent)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSButton *button = [[NSButton alloc] init];
    pimpl = new QButtonPrivate(this, button, bezelStyle);

    QButtonTarget *target = [[QButtonTarget alloc] init];
    target->pimpl = pimpl;
    [button setTarget:target];

    [button setAction:@selector(clicked)];

    setupLayout(button, this);

    [button release];

    [pool drain];
}

void QButton::setText(const QString &text)
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return;

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [pimpl->nsButton setTitle:fromQString(text)];
    [pool drain];
}

void QButton::setImage(const QPixmap &image)
{
    Q_ASSERT(pimpl);
    if (pimpl)
        [pimpl->nsButton setImage:fromQPixmap(image)];
}

void QButton::setChecked(bool checked)
{
    Q_ASSERT(pimpl);
    if (pimpl)
        [pimpl->nsButton setState:checked];
}

void QButton::setCheckable(bool checkable)
{
    const NSInteger cellMask = checkable ? NSChangeBackgroundCellMask : NSNoCellMask;

    Q_ASSERT(pimpl);
    if (pimpl)
        [[pimpl->nsButton cell] setShowsStateBy:cellMask];
}

bool QButton::isChecked()
{
    Q_ASSERT(pimpl);
    if (!pimpl)
        return false;

    return [pimpl->nsButton state];
}
