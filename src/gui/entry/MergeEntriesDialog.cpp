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

#include "MergeEntriesDialog.h"
#include "ui_MergeEntriesDialog.h"

#include "core/Entry.h"
#include "core/Group.h"

#include <QComboBox>
#include <QHeaderView>
#include <QPushButton>

MergeEntriesDialog::MergeEntriesDialog(const QList<Entry*>& entries, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::MergeEntriesDialog())
    , m_entries(entries)
{
    m_ui->setupUi(this);
    m_initialSize = size();

    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setAttribute(Qt::WA_DeleteOnClose);

    for (const Entry* entry : asConst(m_entries)) {
        m_ui->targetComboBox->addItem(entryLabel(entry));
    }

    // The most recently modified entry is the most likely one to keep
    int newestIndex = 0;
    for (int i = 1; i < m_entries.size(); ++i) {
        if (m_entries.at(i)->timeInfo().lastModificationTime()
            > m_entries.at(newestIndex)->timeInfo().lastModificationTime()) {
            newestIndex = i;
        }
    }
    m_ui->targetComboBox->setCurrentIndex(newestIndex);

    m_ui->conflictsTableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    updateConflicts();

    connect(m_ui->targetComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateConflicts()));
    connect(m_ui->concatenateNotesCheckBox, SIGNAL(toggled(bool)), SLOT(updateConflicts()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(close()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(mergeEntries()));
}

MergeEntriesDialog::~MergeEntriesDialog() = default;

Entry* MergeEntriesDialog::targetEntry() const
{
    return m_entries.value(m_ui->targetComboBox->currentIndex());
}

void MergeEntriesDialog::updateConflicts()
{
    auto* target = targetEntry();
    if (!target) {
        return;
    }

    // Which entries take part depends on the entry they are merged into
    auto others = m_entries;
    others.removeAll(target);
    const auto unmergeableEntries = target->unmergeableEntries(others);
    m_mergedEntries.clear();
    for (auto* entry : asConst(others)) {
        if (!unmergeableEntries.contains(entry)) {
            m_mergedEntries << entry;
        }
    }
    m_conflicts = Entry::conflictingAttributes(QList<Entry*>({target}) + m_mergedEntries);

    QStringList unmergeableLabels;
    for (const Entry* entry : unmergeableEntries) {
        unmergeableLabels << entryLabel(entry);
    }
    m_ui->unmergeableEntriesLabel->setText(
        tr("An entry can hold only one passkey. Left out of the merge and kept unchanged: %1")
            .arg(unmergeableLabels.join(", ")));
    m_ui->unmergeableEntriesLabel->setVisible(!unmergeableEntries.isEmpty());

    // Entries that agree on everything are merged without asking anything
    const auto hasEntriesToMerge = !m_mergedEntries.isEmpty();
    const auto hasConflicts = !m_conflicts.isEmpty();
    m_ui->conflictsLabel->setVisible(hasConflicts);
    m_ui->conflictsTableWidget->setVisible(hasConflicts);
    m_ui->keepDiscardedValuesCheckBox->setVisible(hasConflicts);
    m_ui->concatenateNotesCheckBox->setVisible(m_conflicts.contains(EntryAttributes::NotesKey));
    m_ui->noConflictsLabel->setVisible(hasEntriesToMerge && !hasConflicts);
    m_ui->deleteMergedEntriesCheckBox->setVisible(hasEntriesToMerge);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(hasEntriesToMerge);

    // Drop the previous rows along with the combo boxes they own
    m_ui->conflictsTableWidget->setRowCount(0);
    m_ui->conflictsTableWidget->setRowCount(m_conflicts.size());

    int row = 0;
    for (auto i = m_conflicts.constBegin(); i != m_conflicts.constEnd(); ++i, ++row) {
        auto* keyItem = new QTableWidgetItem(attributeLabel(i.key()));
        keyItem->setData(Qt::UserRole, i.key());
        m_ui->conflictsTableWidget->setItem(row, 0, keyItem);

        auto* valueComboBox = new QComboBox(m_ui->conflictsTableWidget);
        if (isConcatenatedNotes(i.key())) {
            // Combined notes leave nothing to choose from
            valueComboBox->addItem(tr("The notes of all entries are combined"));
            valueComboBox->setEnabled(false);
        } else {
            for (const QString& value : i.value()) {
                valueComboBox->addItem(valueLabel(i.key(), value), value);
            }

            // Preselect what the entry we merge into already holds
            const auto targetIndex = valueComboBox->findData(target->attributes()->value(i.key()));
            if (targetIndex >= 0) {
                valueComboBox->setCurrentIndex(targetIndex);
            }
        }

        m_ui->conflictsTableWidget->setCellWidget(row, 1, valueComboBox);
    }

    if (hasConflicts) {
        resize(size().expandedTo(m_initialSize));
    } else {
        // Do not leave the room the conflict table would have taken up empty
        adjustSize();
    }
}

void MergeEntriesDialog::mergeEntries()
{
    auto* target = targetEntry();
    if (!target || m_mergedEntries.isEmpty()) {
        close();
        return;
    }

    QHash<QString, QString> resolvedAttributes;
    for (int row = 0; row < m_ui->conflictsTableWidget->rowCount(); ++row) {
        const auto* keyItem = m_ui->conflictsTableWidget->item(row, 0);
        const auto* valueComboBox = qobject_cast<QComboBox*>(m_ui->conflictsTableWidget->cellWidget(row, 1));
        if (!keyItem || !valueComboBox || isConcatenatedNotes(keyItem->data(Qt::UserRole).toString())) {
            continue;
        }

        resolvedAttributes.insert(keyItem->data(Qt::UserRole).toString(), valueComboBox->currentData().toString());
    }

    Entry::MergeFlags flags = Entry::MergeKeepDiscardedUrls;
    if (m_ui->keepDiscardedValuesCheckBox->isChecked()) {
        flags |= Entry::MergeKeepDiscardedValues;
    }
    if (m_ui->concatenateNotesCheckBox->isChecked()) {
        flags |= Entry::MergeConcatenateNotes;
    }

    // Record the state before the merge, so that it can be restored from history
    target->beginUpdate();
    target->mergeFrom(m_mergedEntries, resolvedAttributes, flags);
    target->endUpdate();

    emit entriesMerged(target, m_ui->deleteMergedEntriesCheckBox->isChecked() ? m_mergedEntries : QList<Entry*>());
    close();
}

bool MergeEntriesDialog::isConcatenatedNotes(const QString& key) const
{
    return key == EntryAttributes::NotesKey && m_ui->concatenateNotesCheckBox->isChecked();
}

bool MergeEntriesDialog::isProtectedAttribute(const QString& key) const
{
    for (const Entry* entry : m_entries) {
        if (entry->attributes()->isProtected(key)) {
            return true;
        }
    }

    return false;
}

QString MergeEntriesDialog::attributeLabel(const QString& key) const
{
    if (key == EntryAttributes::TitleKey) {
        return tr("Title");
    }
    if (key == EntryAttributes::UserNameKey) {
        return tr("Username");
    }
    if (key == EntryAttributes::PasswordKey) {
        return tr("Password");
    }
    if (key == EntryAttributes::URLKey) {
        return tr("URL");
    }
    if (key == EntryAttributes::NotesKey) {
        return tr("Notes");
    }

    return key;
}

QString MergeEntriesDialog::valueLabel(const QString& key, const QString& value) const
{
    if (!isProtectedAttribute(key)) {
        return value;
    }

    // Never reveal a protected value: offer it by the entries that hold it instead
    QStringList titles;
    for (const Entry* entry : m_entries) {
        if (entry->attributes()->value(key) == value) {
            titles << entryLabel(entry);
        }
    }

    return tr("Value of %1").arg(titles.join(", "));
}

QString MergeEntriesDialog::entryLabel(const Entry* entry) const
{
    auto label = entry->resolvePlaceholder(entry->title());
    if (label.isEmpty()) {
        label = tr("(no title)");
    }

    const auto username = entry->resolvePlaceholder(entry->username());
    if (!username.isEmpty()) {
        label = QString("%1 (%2)").arg(label, username);
    }

    if (entry->group()) {
        label = tr("%1 — in %2").arg(label, entry->group()->name());
    }

    return label;
}
