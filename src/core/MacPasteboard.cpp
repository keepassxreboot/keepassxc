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

#include "MacPasteboard.h"

QString MacPasteboard::convertorName()
{
    return QLatin1String("MacPasteboard");
}

QString MacPasteboard::flavorFor(const QString& mimetype)
{
    if (mimetype == QLatin1String("text/plain")) {
        return QLatin1String("public.utf8-plain-text");
    } else if (mimetype == QLatin1String("application/x-nspasteboard-concealed-type")) {
        return QLatin1String("org.nspasteboard.ConcealedType");
    }

    int i = mimetype.indexOf(QLatin1String("charset="));

    if (i >= 0) {
        QString cs(mimetype.mid(i + 8).toLower());
        i = cs.indexOf(QLatin1Char(';'));

        if (i >= 0) {
            cs = cs.left(i);
        }

        if (cs == QLatin1String("system")) {
            return QLatin1String("public.utf8-plain-text");
        } else if (cs == QLatin1String("iso-10646-ucs-2") || cs == QLatin1String("utf16")) {
            return QLatin1String("public.utf16-plain-text");
        }
    }
    return QString();
}

QString MacPasteboard::mimeFor(QString flavor)
{
    if (flavor == QLatin1String("public.utf8-plain-text"))
        return QLatin1String("text/plain");
    if (flavor == QLatin1String("org.nspasteboard.ConcealedType"))
        return QLatin1String("application/x-nspasteboard-concealed-type");
    if (flavor == QLatin1String("public.utf16-plain-text"))
        return QLatin1String("text/plain;charset=utf16");
    return QString();
}

bool MacPasteboard::canConvert(const QString& mimetype, QString flavor)
{
    Q_UNUSED(mimetype);
    Q_UNUSED(flavor);
    return true;
}

QVariant MacPasteboard::convertToMime(const QString& mimetype, QList<QByteArray> data, QString flavor)
{
    if (data.count() > 1)
        qWarning("QMime::convertToMime: Cannot handle multiple member data");
    const QByteArray& firstData = data.first();
    QVariant ret;
    if (flavor == QLatin1String("public.utf8-plain-text")) {
        ret = QString::fromUtf8(firstData);
    } else if (flavor == QLatin1String("org.nspasteboard.ConcealedType")) {
        ret = QString::fromUtf8(firstData);
    } else if (flavor == QLatin1String("public.utf16-plain-text")) {
        ret = QTextCodec::codecForName("UTF-16")->toUnicode(firstData);
    } else {
        qWarning("QMime::convertToMime: unhandled mimetype: %s", qPrintable(mimetype));
    }
    return ret;
}

QList<QByteArray> MacPasteboard::convertFromMime(const QString&, QVariant data, QString flavor)
{
    QList<QByteArray> ret;
    QString string = data.toString();
    if (flavor == QLatin1String("public.utf8-plain-text"))
        ret.append(string.toUtf8());
    else if (flavor == QLatin1String("org.nspasteboard.ConcealedType"))
        ret.append(string.toUtf8());
    else if (flavor == QLatin1String("public.utf16-plain-text"))
        ret.append(QTextCodec::codecForName("UTF-16")->fromUnicode(string));
    return ret;
}
