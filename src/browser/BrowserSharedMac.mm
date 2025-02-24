#include <Foundation/Foundation.h>
#include <QDir>
#include <QString>

namespace BrowserShared
{
    QString macOSLocalServerPath()
    {
        NSString *appGroupIdentifier = @"G2S7P7J672.org.keepassxc.KeePassXC";

        // Get the container URL for the app group identifier
        NSURL *containerURL = [[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:appGroupIdentifier];

        NSString *containerPath = [containerURL path];

        QString homePath = QString::fromNSString(containerPath);

        QDir().mkpath(homePath);

        // The path will become too long therefore we must cut off serverName
        QString socketPath = homePath + "/KeePassXC.BrowserServer";

        return socketPath;
    }
}