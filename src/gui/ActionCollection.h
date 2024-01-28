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

#ifndef KEEPASSXC_ACTION_COLLECTION_H
#define KEEPASSXC_ACTION_COLLECTION_H

#include <QAction>
#include <QKeySequence>
#include <QObject>

/**
 * This class manages all actions that are shortcut configurable.
 * It also allows you to access the actions inside it from anywhere
 * in the gui code.
 */
class ActionCollection : public QObject
{
    Q_OBJECT
    ActionCollection() = default;

public:
    static ActionCollection* instance();

    QList<QAction*> actions() const;

    void addAction(QAction* action);
    void addActions(const QList<QAction*>& actions);

    QKeySequence defaultShortcut(const QAction* a) const;
    QList<QKeySequence> defaultShortcuts(const QAction* a) const;

    void setDefaultShortcut(QAction* a, const QKeySequence& shortcut);
    void setDefaultShortcut(QAction* a, QKeySequence::StandardKey standard, const QKeySequence& fallback);
    void setDefaultShortcuts(QAction* a, const QList<QKeySequence>& shortcut);

    // Check if any action conflicts with @p seq and return the conflicting action
    QAction* isConflictingShortcut(const QAction* action, const QKeySequence& seq) const;

public slots:
    void restoreShortcuts();
    void saveShortcuts();

private:
    QList<QAction*> m_actions;
};

#endif
