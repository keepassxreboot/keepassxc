#include "SafariWebExtensionCheckbox.h"

#include <QApplication>

#include <SafariServices/SafariServices.h>

SafariWebExtensionCheckbox::SafariWebExtensionCheckbox(QWidget *parent)
    : QCheckBox(parent)
{
    connect(qApp, &QApplication::applicationStateChanged, this, &SafariWebExtensionCheckbox::onApplicationStateChanged);
}

void SafariWebExtensionCheckbox::mousePressEvent(QMouseEvent *e) {
    NSString *appIdentifier = QString::fromUtf8(APPLE_APP_IDENTIFIER).toNSString();
    NSString *extensionIdentifier = [appIdentifier stringByAppendingString:@".SafariWebExtension"];

    if (e->button() == Qt::LeftButton) {
        [SFSafariApplication showPreferencesForExtensionWithIdentifier:extensionIdentifier completionHandler:nil];
    }
}

void SafariWebExtensionCheckbox::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (state != Qt::ApplicationActive) {
        return;
    }

    NSString *appIdentifier = QString::fromUtf8(APPLE_APP_IDENTIFIER).toNSString();
    NSString *extensionIdentifier = [appIdentifier stringByAppendingString:@".SafariWebExtension"];

    [SFSafariExtensionManager getStateOfSafariExtensionWithIdentifier:extensionIdentifier completionHandler:^(SFSafariExtensionState *state, NSError *error) {
        if (error) {
            NSLog(@"Error fetching extension state: %@", error.localizedDescription);
            return;
        }

        setChecked(state.isEnabled);
    }];
}
