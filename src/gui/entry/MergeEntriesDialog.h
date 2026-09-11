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

#ifndef KEEPASSX_MERGEENTRIESDIALOG_H
#define KEEPASSX_MERGEENTRIESDIALOG_H

#include <QDialog>
#include <QMap>
#include <QScopedPointer>
#include <QSize>

class Entry;

namespace Ui
{
    class MergeEntriesDialog;
}

/**
 * Let the user merge several entries into one.
 *
 * The user picks the entry to merge into and, for every attribute the entries
 * disagree on, the value to keep. Values of protected attributes are never shown;
 * they are offered by the entry they come from instead.
 */
class MergeEntriesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MergeEntriesDialog(const QList<Entry*>& entries, QWidget* parent = nullptr);
    ~MergeEntriesDialog() override;

signals:
    void entriesMerged(Entry* entry, const QList<Entry*>& discardedEntries);

private slots:
    void updateConflicts();
    void mergeEntries();

private:
    Entry* targetEntry() const;
    bool isConcatenatedNotes(const QString& key) const;
    bool isProtectedAttribute(const QString& key) const;
    QString attributeLabel(const QString& key) const;
    QString valueLabel(const QString& key, const QString& value) const;
    QString entryLabel(const Entry* entry) const;

    const QScopedPointer<Ui::MergeEntriesDialog> m_ui;
    QList<Entry*> m_entries;
    QList<Entry*> m_mergedEntries;
    QMap<QString, QStringList> m_conflicts;
    QSize m_initialSize;
};

#endif // KEEPASSX_MERGEENTRIESDIALOG_H
