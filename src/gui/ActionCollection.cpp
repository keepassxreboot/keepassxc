#include "ActionCollection.h"
#include "core/Config.h"

#include <QDebug>

static bool assert_not_exists(const QVector<QAction*>& actions, const QString& name)
{
    for (auto a : actions) {
        if (a->objectName() == name) {
            return false;
        }
    }
    return true;
}

ActionCollection* ActionCollection::instance()
{
    static ActionCollection ac;
    return &ac;
}

QAction* ActionCollection::action(const QString& name) const
{
    if (name.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Unexpected empty action name";
    }
    for (auto* action : m_actions) {
        if (action->objectName() == name) {
            return action;
        }
    }
    qWarning() << "Unexpected unable to find action " << name << "in ActionCollection. Should not happen";
    return nullptr;
}

QAction* ActionCollection::addAction(const QString& name, QObject* parent)
{
    auto a = new QAction(parent);
    a->setObjectName(name);
    return addAction(a);
}

QAction* ActionCollection::addAction(QAction* action)
{
    if (action->objectName().isEmpty()) {
        qWarning() << Q_FUNC_INFO << "'action name' should not be empty";
        return nullptr;
    }
    Q_ASSERT(assert_not_exists(m_actions, action->objectName()));
    m_actions << action;
    return action;
}

void ActionCollection::addActions(const QVector<QAction*> actions)
{
    m_actions.reserve(m_actions.size() + actions.size());
    for (auto a : actions) {
        addAction(a);
    }
}

QKeySequence ActionCollection::defaultShortcut(QAction* action) const
{
    const QList<QKeySequence> shortcuts = defaultShortcuts(action);
    return shortcuts.isEmpty() ? QKeySequence() : shortcuts.first();
}

QList<QKeySequence> ActionCollection::defaultShortcuts(QAction* action) const
{
    Q_ASSERT(m_actions.contains(action));
    return action->property("defaultShortcuts").value<QList<QKeySequence>>();
}

void ActionCollection::setDefaultShortcut(QAction* action, const QKeySequence& shortcut)
{
    setDefaultShortcuts(action, QList<QKeySequence>() << shortcut);
}

void ActionCollection::setDefaultShortcuts(QAction* action, const QList<QKeySequence>& shortcuts)
{
    Q_ASSERT(m_actions.contains(action));
    action->setShortcuts(shortcuts);
    action->setProperty("defaultShortcuts", QVariant::fromValue(shortcuts));
}

void ActionCollection::restoreShortcuts()
{
    const QVector<Config::ShortcutEntry> shortcuts = Config::instance()->getShortcuts();
    QHash<QString, QAction*> actionsByName;
    actionsByName.reserve(shortcuts.size());
    for (auto action : m_actions) {
        actionsByName[action->objectName()] = action;
    }
    for (const auto& shortcut : shortcuts) {
        auto it = actionsByName.find(shortcut.name);
        if (it == actionsByName.end()) {
            qWarning() << "Unexpected " << shortcut.name << ", not found in actions";
            continue;
        }
        const auto key = QKeySequence::fromString(shortcut.shortcut);
        it.value()->setShortcut(key);
    }
}

void ActionCollection::saveShortcuts()
{
    QVector<Config::ShortcutEntry> shortcuts;
    shortcuts.reserve(m_actions.size());
    for (auto a : m_actions) {
        if (defaultShortcuts(a) == a->shortcuts()) {
            continue;
        }
        shortcuts.push_back(Config::ShortcutEntry{a->objectName(), a->shortcut().toString()});
    }
    Config::instance()->setShortcuts(shortcuts);
}

QPair<bool, QAction*>
ActionCollection::isConflictingShortcut(QAction* action, const QKeySequence& seq, QString& outMessage) const
{
    if (seq.isEmpty()) {
        return {false, nullptr}; // Empty sequences don't conflict with anything
    }
    for (auto a : m_actions) {
        if (a == action) {
            continue;
        }
        if (a->shortcut() == seq) {
            outMessage = tr("Shortcut %3 conflicts with action '%1' with title '%2'")
                             .arg(a->objectName(), a->text(), seq.toString());
            return {true, a};
        }
    }
    return {false, nullptr};
}
