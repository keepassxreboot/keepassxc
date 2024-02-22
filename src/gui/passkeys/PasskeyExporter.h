/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_PASSKEYEXPORTER_H
#define KEEPASSXC_PASSKEYEXPORTER_H

#include <QList>
#include <QObject>

class Entry;

class PasskeyExporter : public QObject
{
    Q_OBJECT

public:
    explicit PasskeyExporter() = default;

    void showExportDialog(const QList<Entry*>& items);

private:
    void exportSelectedEntry(const Entry* entry, const QString& folder);
};

#endif // KEEPASSXC_PASSKEYEXPORTER_H
