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

#include "UrlOverrideSettingsPage.h"

#include "UrlOverride.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
    enum Column
    {
        EnabledColumn,
        SchemeColumn,
        CommandColumn
    };
} // namespace

class UrlOverrideSettingsWidget final : public QWidget
{
public:
    explicit UrlOverrideSettingsWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_table(new QTableWidget(0, 3, this))
        , m_addButton(new QPushButton(QObject::tr("Add"), this))
        , m_removeButton(new QPushButton(QObject::tr("Remove"), this))
        , m_moveUpButton(new QPushButton(QObject::tr("Move Up"), this))
        , m_moveDownButton(new QPushButton(QObject::tr("Move Down"), this))
    {
        auto* layout = new QVBoxLayout(this);

        auto* infoLabel = new QLabel(
            QObject::tr("Define rules to launch an external command instead of the default action when opening a "
                        "URL. The first enabled rule whose URL Scheme (e.g. \"http\", \"ftp\", or a custom scheme "
                        "such as \"kdbx\") exactly matches an entry's URL scheme is used. The command may use the "
                        "same placeholders as Auto-Type (e.g. {USERNAME}, {PASSWORD}, {URL:HOST}, {URL:PORT}) and "
                        "must start with \"cmd://\" to be executed as a command."),
            this);
        infoLabel->setWordWrap(true);
        layout->addWidget(infoLabel);

        m_table->setHorizontalHeaderLabels(
            {QObject::tr("Enabled"), QObject::tr("URL Scheme"), QObject::tr("Command")});
        m_table->horizontalHeader()->setSectionResizeMode(EnabledColumn, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(SchemeColumn, QHeaderView::Interactive);
        m_table->horizontalHeader()->setSectionResizeMode(CommandColumn, QHeaderView::Stretch);
        m_table->verticalHeader()->hide();
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setSelectionMode(QAbstractItemView::SingleSelection);
        layout->addWidget(m_table);

        auto* buttonLayout = new QHBoxLayout();
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_removeButton);
        buttonLayout->addStretch();
        buttonLayout->addWidget(m_moveUpButton);
        buttonLayout->addWidget(m_moveDownButton);
        layout->addLayout(buttonLayout);

        QObject::connect(m_addButton, &QPushButton::clicked, this, [this] { addRow(true, {}, {}); });
        QObject::connect(m_removeButton, &QPushButton::clicked, this, &UrlOverrideSettingsWidget::removeSelectedRow);
        QObject::connect(m_moveUpButton, &QPushButton::clicked, this, [this] { moveSelectedRow(-1); });
        QObject::connect(m_moveDownButton, &QPushButton::clicked, this, [this] { moveSelectedRow(1); });
    }

    void loadSettings()
    {
        m_originalRules = UrlOverride::getRules();
        m_table->setRowCount(0);
        for (const auto& rule : m_originalRules) {
            addRow(rule.enabled, rule.scheme, rule.command);
        }
    }

    void saveSettings()
    {
        auto rules = tableRules();
        if (rules == m_originalRules) {
            // Nothing changed, no need to ask for confirmation
            return;
        }

        auto answer = MessageBox::question(
            this,
            QObject::tr("Confirm URL Scheme Overrides"),
            QObject::tr("You are about to change how KeePassXC opens URLs with certain schemes. Entries whose URL "
                        "scheme matches one of these rules will run the configured command instead of the default "
                        "action.\n\nDo you want to save these changes?"),
            MessageBox::Yes | MessageBox::Cancel,
            MessageBox::Cancel);
        if (answer != MessageBox::Yes) {
            // Discard the edits and restore the table to the last saved state
            m_table->setRowCount(0);
            for (const auto& rule : m_originalRules) {
                addRow(rule.enabled, rule.scheme, rule.command);
            }
            return;
        }

        UrlOverride::setRules(rules);
        m_originalRules = UrlOverride::getRules();
    }

private:
    QList<UrlOverride::Rule> tableRules() const
    {
        QList<UrlOverride::Rule> rules;
        rules.reserve(m_table->rowCount());
        for (int row = 0; row < m_table->rowCount(); ++row) {
            UrlOverride::Rule rule;
            rule.enabled = m_table->item(row, EnabledColumn)->checkState() == Qt::Checked;
            rule.scheme = UrlOverride::normalizeScheme(m_table->item(row, SchemeColumn)->text());
            rule.command = m_table->item(row, CommandColumn)->text().trimmed();
            if (rule.scheme.isEmpty() && rule.command.isEmpty()) {
                continue;
            }
            rules.append(rule);
        }
        return rules;
    }

    void addRow(bool enabled, const QString& scheme, const QString& command)
    {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto* enabledItem = new QTableWidgetItem();
        enabledItem->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        enabledItem->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(row, EnabledColumn, enabledItem);

        m_table->setItem(row, SchemeColumn, new QTableWidgetItem(scheme));
        m_table->setItem(row, CommandColumn, new QTableWidgetItem(command));

        m_table->selectRow(row);
        m_table->setCurrentCell(row, SchemeColumn);
    }

    void removeSelectedRow()
    {
        auto row = m_table->currentRow();
        if (row >= 0) {
            m_table->removeRow(row);
        }
    }

    void moveSelectedRow(int direction)
    {
        auto row = m_table->currentRow();
        auto newRow = row + direction;
        if (row < 0 || newRow < 0 || newRow >= m_table->rowCount()) {
            return;
        }

        for (int col = 0; col < m_table->columnCount(); ++col) {
            auto* item = m_table->takeItem(row, col);
            auto* otherItem = m_table->takeItem(newRow, col);
            m_table->setItem(newRow, col, item);
            m_table->setItem(row, col, otherItem);
        }
        m_table->selectRow(newRow);
    }

    QTableWidget* const m_table;
    QPushButton* const m_addButton;
    QPushButton* const m_removeButton;
    QPushButton* const m_moveUpButton;
    QPushButton* const m_moveDownButton;
    QList<UrlOverride::Rule> m_originalRules;
};

QString UrlOverrideSettingsPage::name()
{
    return QObject::tr("URL Overrides");
}

QIcon UrlOverrideSettingsPage::icon()
{
    return icons()->icon("internet-web-browser");
}

QWidget* UrlOverrideSettingsPage::createWidget()
{
    return new UrlOverrideSettingsWidget();
}

void UrlOverrideSettingsPage::loadSettings(QWidget* widget)
{
    static_cast<UrlOverrideSettingsWidget*>(widget)->loadSettings();
}

void UrlOverrideSettingsPage::saveSettings(QWidget* widget)
{
    static_cast<UrlOverrideSettingsWidget*>(widget)->saveSettings();
}
