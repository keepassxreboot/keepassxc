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

#ifndef KEEPASSXC_CSVEXTRACOLUMNS_H
#define KEEPASSXC_CSVEXTRACOLUMNS_H

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUuid>

class Entry;
class TimeInfo;

// the columns the CSV export writes besides the eleven the import form maps by hand (Group, Title, ...,
// Created): dates and counters, the entry's id, custom fields, attachments, auto-type settings, history, and the
// properties that the entry editor shows on its other tabs. They are recognised by their heading, so a file written by
// this program imports completely without the person having to map anything.
namespace CsvExtraColumns
{
    // The headings of those columns, exactly as the export writes them
    const QStringList& names();

    // Puts one row back into the entry. cells holds the text of the row's cells by heading (only headings that are in
    // the file). Times go into timeInfo, which the caller gives to the entry as the very last step: adding custom
    // fields, attachments and history stamps an entry as modified now.
    //
    // usedIds holds the entry ids already given out in this file. An id that is already in use or is not valid is not
    // used and the entry keeps its fresh one. Everything that cannot be restored, and everything that had to be
    // changed, is added to problems as one line that names the row; a cell that cannot be understood is kept as a
    // custom field with its original text instead of being dropped.
    void apply(Entry* entry,
               TimeInfo& timeInfo,
               const QHash<QString, QString>& cells,
               QSet<QUuid>& usedIds,
               const QString& rowLabel,
               QStringList& problems);

    // The column "Exact Text": text of the other columns that a plain CSV cell cannot carry (half of an emoji is not
    // text that UTF-8 can hold). The export writes such a cell with a replacement character and the exact text here, as
    // a JSON object: heading of the column -> text. Fills texts and returns true, or returns false when the cell is not
    // such an object.
    bool exactTexts(const QString& cell, QHash<QString, QString>& texts);

    // Adds a custom field under a name that is not in use yet (a standard field's name, or a name already taken, gets
    // " (CSV)" added). Returns the name used.
    QString addCustomField(Entry* entry, const QString& name, const QString& value, bool hidden);
} // namespace CsvExtraColumns

#endif // KEEPASSXC_CSVEXTRACOLUMNS_H
