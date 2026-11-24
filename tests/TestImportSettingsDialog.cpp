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

#include "TestImportSettingsDialog.h"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>

#include "gui/wizard/ImportSettingsDialog.h"

QTEST_MAIN(TestImportSettingsDialog)

namespace
{
    // The columns of the table: Apply | Setting | Your database now | In the file
    const int SettingColumn = 1;
    const int CurrentColumn = 2;
    const int FileColumn = 3;

    QList<DatabaseSettingChange> sampleChanges()
    {
        DatabaseSettingChange name;
        name.id = "name";
        name.label = "Database name";
        name.currentValue = "Personal";
        name.fileValue = "Work vault";

        DatabaseSettingChange history;
        history.id = "historyMaxItems";
        history.label = "Old versions kept per entry";
        history.currentValue = "10";
        history.fileValue = "No limit";

        DatabaseSettingChange browserKey;
        browserKey.id = "customData:KPXC_BROWSER_laptop";
        browserKey.label = "Browser extension connection: laptop";
        browserKey.currentValue = "(not set)";
        browserKey.fileValue = "(hidden)";
        browserKey.note = "Carries over the trust link between this database and a browser extension.";

        return {name, history, browserKey};
    }

    QPushButton* buttonWithText(const QDialog& dialog, const QString& text)
    {
        for (auto* button : dialog.findChildren<QPushButton*>()) {
            if (button->text().remove('&') == text) {
                return button;
            }
        }
        return nullptr;
    }

    QStringList idsOf(const QList<DatabaseSettingChange>& changes)
    {
        QStringList ids;
        for (const auto& change : changes) {
            ids << change.id;
        }
        return ids;
    }
} // namespace

// Catches: a setting missing from the list, shown with the wrong values, or ticked without the user doing so (which
// would overwrite the open database without a choice).
void TestImportSettingsDialog::testShowsEveryChangeUntickedByDefault()
{
    ImportSettingsDialog dialog(sampleChanges());
    auto* table = dialog.findChild<QTableWidget*>();
    QVERIFY(table);
    QCOMPARE(table->rowCount(), 3);

    QVERIFY(table->item(0, SettingColumn)->text().contains("Database name"));
    QCOMPARE(table->item(0, CurrentColumn)->text(), QString("Personal"));
    QCOMPARE(table->item(0, FileColumn)->text(), QString("Work vault"));
    QCOMPARE(table->item(1, FileColumn)->text(), QString("No limit"));
    // the warning that goes with a row is shown with it
    QVERIFY(table->item(2, SettingColumn)->text().contains("trust link"));

    for (int row = 0; row < table->rowCount(); ++row) {
        QCOMPARE(table->item(row, 0)->checkState(), Qt::Unchecked);
    }
    QVERIFY(dialog.chosenChanges().isEmpty());
}

// Catches: the wrong settings (or all of them) being applied after the user ticked some.
void TestImportSettingsDialog::testReturnsOnlyTheTickedChanges()
{
    ImportSettingsDialog dialog(sampleChanges());
    auto* table = dialog.findChild<QTableWidget*>();
    QVERIFY(table);

    table->item(0, 0)->setCheckState(Qt::Checked);
    table->item(2, 0)->setCheckState(Qt::Checked);

    QCOMPARE(idsOf(dialog.chosenChanges()), QStringList({"name", "customData:KPXC_BROWSER_laptop"}));
}

// Catches: the select-all / select-none buttons doing nothing, or doing the opposite.
void TestImportSettingsDialog::testSelectAllAndSelectNone()
{
    ImportSettingsDialog dialog(sampleChanges());
    auto* all = buttonWithText(dialog, "Select all");
    auto* none = buttonWithText(dialog, "Select none");
    QVERIFY(all);
    QVERIFY(none);

    all->click();
    QCOMPARE(dialog.chosenChanges().size(), 3);
    none->click();
    QCOMPARE(dialog.chosenChanges().size(), 0);
}

// Catches: things the file stores but that are not offered being skipped without a word.
void TestImportSettingsDialog::testNamesWhatIsNotOffered()
{
    ImportSettingsDialog dialog(sampleChanges());
    QString shown;
    for (auto* label : dialog.findChildren<QLabel*>()) {
        shown += label->text() + "\n";
    }
    for (const auto& item : DatabaseSettingsDiff::notOffered()) {
        QVERIFY2(shown.contains(item), qPrintable("not named in the dialog: " + item));
    }
}
