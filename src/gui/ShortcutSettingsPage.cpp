/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include <QAbstractButton>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>

class KeySequenceDialog : public QDialog
{
public:
    KeySequenceDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , m_keySeqEdit(new QKeySequenceEdit(this))
        , m_btnBox(new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel
                                            | QDialogButtonBox::RestoreDefaults,
                                        this))
    {
        QVBoxLayout* l = new QVBoxLayout(this);
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
        return m_keySeqEdit->keySequence();
    }

    bool shouldRestoreDefault() const
    {
        return m_restoreDefault;
    }

private:
    void restoreDefault(QAbstractButton* btn)
    {
        if (m_btnBox->standardButton(btn) != QDialogButtonBox::RestoreDefaults) {
            return;
        }
        m_restoreDefault = true;
        reject();
    }

private:
    bool m_restoreDefault = false;
    QKeySequenceEdit* const m_keySeqEdit;
    QDialogButtonBox* const m_btnBox;
};

class ShortcutSettingsWidget : public QWidget
{
    struct ShortcutChange
    {
        bool restoreDefault = false;
        QKeySequence seq;
    };

public:
    ShortcutSettingsWidget(QWidget* parent = nullptr)
        : QWidget(parent)
        , m_treeView(new QTreeView(this))
        , m_filterLineEdit(new QLineEdit(this))
    {
        auto l = new QVBoxLayout(this);
        l->addWidget(new QLabel(tr("Double click an action to change its shortcut")));
        l->addWidget(m_filterLineEdit);
        l->addWidget(m_treeView);

        m_model.setColumnCount(2);
        m_model.setHorizontalHeaderLabels({QObject::tr("Action"), QObject::tr("Shortcuts")});

        m_proxy.setFilterKeyColumn(-1);
        m_proxy.setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxy.setSourceModel(&m_model);
        m_filterLineEdit->setPlaceholderText(tr("Filter..."));
        connect(m_filterLineEdit, &QLineEdit::textChanged, &m_proxy, &QSortFilterProxyModel::setFilterFixedString);

        m_treeView->setModel(&m_proxy);
        m_treeView->setUniformRowHeights(true);
        m_treeView->setAllColumnsShowFocus(true);
        m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

        connect(m_treeView, &QTreeView::doubleClicked, this, &ShortcutSettingsWidget::onDoubleClicked);
    }

    void loadSettings()
    {
        m_model.setRowCount(0);
        const auto& actions = ActionCollection::instance()->actions();
        for (auto a : actions) {
            auto col1 = new QStandardItem(acceleratorsStrippedText(a->text()));
            col1->setData(QVariant::fromValue(a), Qt::UserRole);
            auto col2 = new QStandardItem(a->shortcut().toString());
            m_model.appendRow({col1, col2});
        }
    }

    void saveSettings()
    {
        if (!m_changed) {
            return;
        }
        for (auto it = m_changedActions.begin(); it != m_changedActions.end(); ++it) {
            auto action = it.key();
            auto change = it.value();
            if (change.restoreDefault) {
                action->setShortcut(ActionCollection::instance()->defaultShortcut(action));
            } else {
                action->setShortcut(change.seq);
            }
        }
        ActionCollection::instance()->saveShortcuts();
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

        KeySequenceDialog dlg(this);
        int ret = dlg.exec();

        ShortcutChange change;
        if (ret == QDialog::Accepted) {
            change.seq = dlg.keySequence();
        } else if (dlg.shouldRestoreDefault()) {
            change.restoreDefault = true;
            change.seq = ActionCollection::instance()->defaultShortcut(action);
        } else {
            // Rejected
            return;
        }

        QString errMsg;
        QPair<bool, QAction*> res = ActionCollection::instance()->isConflictingShortcut(action, change.seq, errMsg);
        bool showError = false;
        if (res.first) {
            // we conflicted with an action inside action collection
            // check if the conflicted action is updated here
            auto found = m_changedActions.constFind(res.second);
            if (found == m_changedActions.cend()) {
                showError = true;
            } else {
                auto changedSequence = found.value().seq;
                if (!changedSequence.isEmpty() && changedSequence == change.seq) {
                    showError = true;
                }
            }
        } else {
            // we did not confict with any shortcut inside action collection
            // check if we conflict with any locally modified action
            for (auto it = m_changedActions.cbegin(); it != m_changedActions.cend(); ++it) {
                const auto& sc = it.value();
                if (!change.seq.isEmpty() && change.seq == sc.seq) {
                    showError = true;
                    errMsg = tr("Shortcut %3 conflicts with action '%1' with title '%2'")
                                 .arg(sc.seq.toString(), it.key()->objectName(), it.key()->text());
                    break;
                }
            }
        }

        if (showError) {
            QMessageBox::warning(this, tr("Shortcut Conflict"), errMsg);
            return;
        }

        m_changed = true;
        m_changedActions[action] = change;
        auto item = m_model.itemFromIndex(index.sibling(index.row(), 1));
        item->setText(change.seq.toString());
    }

private:
    QTreeView* const m_treeView;
    QLineEdit* const m_filterLineEdit;
    QStandardItemModel m_model;
    QSortFilterProxyModel m_proxy;
    bool m_changed = false;
    QHash<QAction*, ShortcutChange> m_changedActions;
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
