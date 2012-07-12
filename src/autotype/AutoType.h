/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_AUTOTYPE_H
#define KEEPASSX_AUTOTYPE_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

#include "core/Global.h"

class AutoTypeAction;
class AutoTypeExecutor;
class AutoTypePlatformInterface;
class Database;
class Entry;
class QPluginLoader;

class AutoType : public QObject
{
    Q_OBJECT

public:
    QStringList windowTitles();
    void performAutoType(const Entry* entry, QWidget* hideWindow = Q_NULLPTR,
                         const QString& customSequence = QString());
    void performGlobalAutoType(const QList<Database*> dbList);
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void unregisterGlobalShortcut();
    int callEventFilter(void* event);

    inline bool isAvailable() {
        return m_plugin;
    }

    static AutoType* instance();

Q_SIGNALS:
    void globalShortcutTriggered();

private:
    explicit AutoType(QObject* parent = Q_NULLPTR);
    ~AutoType();
    bool parseActions(const QString& sequence, const Entry* entry, QList<AutoTypeAction*>& actions);
    QList<AutoTypeAction*> createActionFromTemplate(const QString& tmpl, const Entry* entry);

    bool m_inAutoType;
    Qt::Key m_currentGlobalKey;
    Qt::KeyboardModifiers m_currentGlobalModifiers;
    QPluginLoader* m_pluginLoader;
    AutoTypePlatformInterface* m_plugin;
    AutoTypeExecutor* m_executor;
    static AutoType* m_instance;

    Q_DISABLE_COPY(AutoType)
};

inline AutoType* autoType() {
    return AutoType::instance();
}

#endif // KEEPASSX_AUTOTYPE_H
