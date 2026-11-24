/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#include "HiddenCharacterList.h"

#include <QObject>
#include <QSaveFile>

QString HiddenCharacterList::filePathFor(const QString& exportFilePath)
{
    QString base = exportFilePath;
    if (base.endsWith(QLatin1String(".xml"), Qt::CaseInsensitive)) {
        base.chop(4);
    }
    return base + QStringLiteral(".hidden-characters.txt");
}

bool HiddenCharacterList::save(const QString& filePath, const QStringList& places, QString* error)
{
    const QString text =
        QObject::tr("These values contain characters that XML cannot store as text (for example hidden control "
                    "characters).\n"
                    "In the XML file they are saved as base64 and are restored exactly when the file is imported "
                    "into KeePassXC.\n"
                    "This list names the places only, never the values.\n"
                    "\n"
                    "Number of values: %1\n"
                    "\n")
            .arg(places.size())
        + places.join(QLatin1Char('\n')) + QLatin1Char('\n');

    // Written to a temporary file that replaces the target only when complete, like the export itself
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    const QByteArray bytes = text.toUtf8();
    if (file.write(bytes) != bytes.size()) {
        if (error) {
            *error = file.errorString();
        }
        file.cancelWriting();
        return false;
    }
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}
