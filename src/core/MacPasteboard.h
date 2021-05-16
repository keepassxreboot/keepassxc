/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_MACPASTEBOARD_H
#define KEEPASSXC_MACPASTEBOARD_H

#include <QObject>
#include <QTextCodec>
#include <QtMacExtras/QMacPasteboardMime>

class MacPasteboard : public QObject, public QMacPasteboardMime
{
public:
    explicit MacPasteboard()
        : QMacPasteboardMime(MIME_ALL)
    {
    }

    QString convertorName() override;
    bool canConvert(const QString& mime, QString flav) override;
    QString mimeFor(QString flav) override;
    QString flavorFor(const QString& mime) override;
    QVariant convertToMime(const QString& mime, QList<QByteArray> data, QString flav) override;
    QList<QByteArray> convertFromMime(const QString& mime, QVariant data, QString flav) override;
};

#endif // KEEPASSXC_MACPASTEBOARD_H
