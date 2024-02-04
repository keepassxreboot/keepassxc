/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
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

#include "ShortcutSettingsPage.h"

#include "core/Config.h"
#include "gui/ActionCollection.h"
#include "gui/Icons.h"
#include "gui/MessageBox.h"
#include "gui/widgets/ShortcutWidget.h"

#include <QAbstractButton>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

class KeySequenceDialog final : public QDialog
{
public:
    explicit KeySequenceDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , m_keySeqEdit(new ShortcutWidget(this))
        , m_btnBox(new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel
                                            | QDialogButtonBox::RestoreDefaults,
                                        this))
    {
        auto* l = new QVBoxLayout(this);
        connect(m_btnBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(m_btnBox, &QDialogButtonBox::clicked, this, &KeySequenceDialog::restoreDefault);

        auto hLayout = new QHBoxLayout();
        l->addLayout(hLayout);
        hLayout->addWidget(new QLabel(QObject::tr("Enter Shortcut")));
        hLayout->addWidget(m_keySeqEdit);

        l->addStretch();
        l->addWidget(m_btnBox);

        setFocusProxy(m_keySeqEdit);
    }

    QKeySequence keySequence() const
    {
        return m_keySeqEdit->sequence();
    }

    bool shouldRestoreDefault() const
    {
        return m_restoreDefault;
    }

private:
    void restoreDefault(QAbstractButton* btn)
    {
        if (m_btnBox->standardButton(btn) == QDialogButtonBox::RestoreDefaults) {
            m_restoreDefault = true;
            reject();
        }
    }

private:
    bool m_restoreDefault = false;
    ShortcutWidget* const m_keySeqEdit;
    QDialogButtonBox* const m_btnBox;
};

class ShortcutSettingsWidget final : public QWidget
{
public:
    explicit ShortcutSettingsWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_tableView(new QTableView(this))
        , m_filterLineEdit(new QLineEdit(this))
        , m_resetShortcutsButton(new QPushButton(tr("Reset Shortcuts"), this))
    {
        auto h = new QHBoxLayout();
        h->addWidget(m_filterLineEdit);
        h->addWidget(m_resetShortcutsButton);
        h->setStretch(0, 1);

        auto l = new QVBoxLayout(this);
        l->addWidget(new QLabel(tr("Double click an action to change its shortcut")));
        l->addLayout(h);
        l->addWidget(m_tableView);

        m_model.setColumnCount(2);
        m_model.setHorizontalHeaderLabels({QObject::tr("Action"), QObject::tr("Shortcuts")});

        m_proxy.setFilterKeyColumn(-1);
        m_proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxy.setSourceModel(&m_model);

        m_filterLineEdit->setPlaceholderText(tr("Filter..."));
        connect(m_filterLineEdit, &QLineEdit::textChanged, &m_proxy, &QSortFilterProxyModel::setFilterFixedString);

        connect(m_resetShortcutsButton, &QPushButton::clicked, this, [this]() {
            auto ac = ActionCollection::instance();
            for (auto action : ac->actions()) {
                action->setShortcut(ac->defaultShortcut(action));
            }
            loadSettings();
        });

        m_tableView->setModel(&m_proxy);
        m_tableView->setSortingEnabled(true);
        m_tableView->sortByColumn(0, Qt::AscendingOrder);
        m_tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_tableView->verticalHeader()->hide();
        m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(m_tableView, &QTableView::doubleClicked, this, &ShortcutSettingsWidget::onDoubleClicked);
    }

    void loadSettings()
    {
        m_changedActions.clear();
        m_filterLineEdit->clear();
        m_model.setRowCount(0);
        const auto& actions = ActionCollection::instance()->actions();
        for (auto a : actions) {
            auto name = a->toolTip().isEmpty() ? acceleratorsStrippedText(a->text()) : a->toolTip();
            auto col1 = new QStandardItem(name);
            col1->setData(QVariant::fromValue(a), Qt::UserRole);
            auto col2 = new QStandardItem(a->shortcut().toString());
            m_model.appendRow({col1, col2});
        }
    }

    void saveSettings()
    {
        if (m_changedActions.count()) {
            for (const auto& action : m_changedActions.keys()) {
                action->setShortcut(m_changedActions.value(action));
            }
            ActionCollection::instance()->saveShortcuts();
        }
        m_changedActions.clear();
        m_filterLineEdit->clear();
    }

private:
    static QString acceleratorsStrippedText(QString text)
    {
        for (int i = 0; i < text.size(); ++i) {
            if (text.at(i) == QLatin1Char('&') && i + 1 < text.size() && text.at(i + 1) != QLatin1Char('&')) {
                text.remove(i, 1);
            }
        }
        return text;
    }

    void onDoubleClicked(QModelIndex index)
    {
        if (index.column() != 0) {
            index = index.sibling(index.row(), 0);
        }
        index = m_proxy.mapToSource(index);
        auto action = index.data(Qt::UserRole).value<QAction*>();

        KeySequenceDialog dialog(this);
        int ret = dialog.exec();

        QKeySequence change;
        if (ret == QDialog::Accepted) {
            change = dialog.keySequence();
        } else if (dialog.shouldRestoreDefault()) {
            change = ActionCollection::instance()->defaultShortcut(action);
        } else {
            // Rejected
            return;
        }

        auto conflict = ActionCollection::instance()->isConflictingShortcut(action, change);
        bool hasConflict = false;
        if (conflict) {
            // we conflicted with an action inside action collection
            // check if the conflicted action is updated here
            if (!m_changedActions.contains(conflict)) {
                hasConflict = true;
            } else {
                if (m_changedActions.value(conflict) == change) {
                    hasConflict = true;
                }
            }
        } else if (!change.isEmpty()) {
            // we did not conflict with any shortcut inside action collection
            // check if we conflict with any locally modified action
            for (auto chAction : m_changedActions.keys()) {
                if (m_changedActions.value(chAction) == change) {
                    hasConflict = true;
                    conflict = chAction;
                    break;
                }
            }
        }

        if (hasConflict) {
            auto conflictName =
                conflict->toolTip().isEmpty() ? acceleratorsStrippedText(conflict->text()) : conflict->toolTip();
            auto conflictSeq = change.toString();

            auto ans = MessageBox::question(
                this,
                tr("Shortcut Conflict"),
                tr("Shortcut %1 conflicts with '%2'. Overwrite shortcut?").arg(conflictSeq, conflictName),
                MessageBox::Overwrite | MessageBox::Discard,
                MessageBox::Discard);
            if (ans == MessageBox::Discard) {
                // Bail out before making any changes
                return;
            }

            // Reset the conflict shortcut
            m_changedActions[conflict] = {};
            for (auto item : m_model.findItems(conflictSeq, Qt::MatchExactly, 1)) {
                item->setText("");
            }
        }

        m_changedActions[action] = change;
        auto item = m_model.itemFromIndex(index.sibling(index.row(), 1));
        item->setText(change.toString());
    }

    QTableView* m_tableView;
    QLineEdit* m_filterLineEdit;
    QPushButton* m_resetShortcutsButton;
    QStandardItemModel m_model;
    QSortFilterProxyModel m_proxy;
    QHash<QAction*, QKeySequence> m_changedActions;
};

QString ShortcutSettingsPage::name()
{
    return QObject::tr("Shortcuts");
}

QIcon ShortcutSettingsPage::icon()
{
    return icons()->icon("auto-type");
}

QWidget* ShortcutSettingsPage::createWidget()
{
    return new ShortcutSettingsWidget();
}

void ShortcutSettingsPage::loadSettings(QWidget* widget)
{
    static_cast<ShortcutSettingsWidget*>(widget)->loadSettings();
}

void ShortcutSettingsPage::saveSettings(QWidget* widget)
{
    static_cast<ShortcutSettingsWidget*>(widget)->saveSettings();
}
