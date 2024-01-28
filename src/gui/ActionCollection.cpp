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

#include "ActionCollection.h"
#include "core/Config.h"

#include <QDebug>

ActionCollection* ActionCollection::instance()
{
    static ActionCollection ac;
    return &ac;
}

QList<QAction*> ActionCollection::actions() const
{
    return m_actions;
}

void ActionCollection::addAction(QAction* action)
{
    if (!m_actions.contains(action)) {
        m_actions << action;
    }
}

void ActionCollection::addActions(const QList<QAction*>& actions)
{
    for (auto a : actions) {
        addAction(a);
    }
}

QKeySequence ActionCollection::defaultShortcut(const QAction* action) const
{
    auto shortcuts = defaultShortcuts(action);
    return shortcuts.isEmpty() ? QKeySequence() : shortcuts.first();
}

QList<QKeySequence> ActionCollection::defaultShortcuts(const QAction* action) const
{
    return action->property("defaultShortcuts").value<QList<QKeySequence>>();
}

void ActionCollection::setDefaultShortcut(QAction* action, const QKeySequence& shortcut)
{
    setDefaultShortcuts(action, {shortcut});
}

void ActionCollection::setDefaultShortcut(QAction* action,
                                          QKeySequence::StandardKey standard,
                                          const QKeySequence& fallback)
{
    if (!QKeySequence::keyBindings(standard).isEmpty()) {
        setDefaultShortcuts(action, QKeySequence::keyBindings(standard));
    } else if (fallback != 0) {
        setDefaultShortcut(action, QKeySequence(fallback));
    }
}

void ActionCollection::setDefaultShortcuts(QAction* action, const QList<QKeySequence>& shortcuts)
{
    action->setShortcuts(shortcuts);
    action->setProperty("defaultShortcuts", QVariant::fromValue(shortcuts));
}

void ActionCollection::restoreShortcuts()
{
    const auto shortcuts = Config::instance()->getShortcuts();
    QHash<QString, QAction*> actionsByName;
    for (auto action : m_actions) {
        actionsByName.insert(action->objectName(), action);
    }
    for (const auto& shortcut : shortcuts) {
        if (actionsByName.contains(shortcut.name)) {
            const auto key = QKeySequence::fromString(shortcut.shortcut);
            actionsByName.value(shortcut.name)->setShortcut(key);
        }
    }
}

void ActionCollection::saveShortcuts()
{
    QList<Config::ShortcutEntry> shortcuts;
    shortcuts.reserve(m_actions.size());
    for (auto a : m_actions) {
        // Only store non-default shortcut assignments
        if (a->shortcut() != defaultShortcut(a)) {
            shortcuts << Config::ShortcutEntry{a->objectName(), a->shortcut().toString()};
        }
    }
    Config::instance()->setShortcuts(shortcuts);
}

QAction* ActionCollection::isConflictingShortcut(const QAction* action, const QKeySequence& seq) const
{
    // Empty sequences don't conflict with anything
    if (seq.isEmpty()) {
        return nullptr;
    }

    for (auto a : m_actions) {
        if (a != action && a->shortcut() == seq) {
            return a;
        }
    }

    return nullptr;
}
