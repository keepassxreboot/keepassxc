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

    const QVector<QAction*> actions() const
    {
        return m_actions;
    }

    /**
     * Add an action to the collection
     */
    QAction* addAction(const QString& name, QObject* parent = nullptr);

    /**
     * Add an action to the collection
     */
    QAction* addAction(QAction* action);

    void addActions(const QVector<QAction*> actions);

    /**
     * Retreive an action from the collection
     */
    QAction* action(const QString& name) const;

    void setDefaultShortcut(QAction* a, const QKeySequence& shortcut);
    QKeySequence defaultShortcut(QAction* a) const;

    void setDefaultShortcuts(QAction* a, const QList<QKeySequence>& shortcut);
    QList<QKeySequence> defaultShortcuts(QAction* a) const;

    // Check if any action conflicts with @p seq and return the conflicting action
    QPair<bool, QAction*> isConflictingShortcut(QAction* action, const QKeySequence& seq, QString& outMessage) const;

public Q_SLOTS:
    void restoreShortcuts();
    void saveShortcuts();

private:
    QVector<QAction*> m_actions;
};

#endif
