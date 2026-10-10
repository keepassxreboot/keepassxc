/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QCommandLineParser>
#include <QCoreApplication>

#include "WebProxy.h"
#include "config-keepassx.h"

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setApplicationVersion(KEEPASSXC_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("KeePassXC Web Proxy");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption portOption(
        QStringList() << "p" << "port", QObject::tr("Listen to WebSocket port <port>."), QObject::tr("port"));
    const QCommandLineOption customExtensionIdOption(QStringList() << "e" << "extensionid",
                                                     QObject::tr("Custom extension ID to accept <extensionid>."),
                                                     QObject::tr("extensionid"));
    parser.addOption(portOption);
    parser.addOption(customExtensionIdOption);
    parser.process(application);

    const auto webSocketPort = parser.value(portOption);
    const auto customExtensionId = parser.value(customExtensionIdOption);
    WebProxy web_proxy(webSocketPort, customExtensionId);

    return application.exec();
}
