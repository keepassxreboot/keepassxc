/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#include <QObject>
#include <QStringList>
#include <QWidget>

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
    void performAutoType(const Entry* entry, QWidget* hideWindow = nullptr,
                         const QString& customSequence = QString(), WId window = 0);
    bool registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers);
    void unregisterGlobalShortcut();
    int callEventFilter(void* event);
    static bool checkSyntax(const QString &string);
    static bool checkHighRepetition(const QString &string);
    static bool checkHighDelay(const QString &string);
    void performAutoTypeWithSyntaxCheckingDialog(const Entry *entry,
                                                 QWidget *hideWindow = nullptr,
                                                 const QString &customSequence = QString(),
                                                 WId window = 0);

    inline bool isAvailable() {
        return m_plugin;
    }

    static AutoType* instance();
    static void createTestInstance();

public slots:
    void performGlobalAutoType(const QList<Database*>& dbList);

signals:
    void globalShortcutTriggered();

private slots:
    void performAutoTypeFromGlobal(Entry* entry, const QString& sequence);
    void resetInAutoType();
    void unloadPlugin();

private:
    explicit AutoType(QObject* parent = nullptr, bool test = false);
    ~AutoType();
    void loadPlugin(const QString& pluginPath);
    bool parseActions(const QString& sequence, const Entry* entry, QList<AutoTypeAction*>& actions);
    QList<AutoTypeAction*> createActionFromTemplate(const QString& tmpl, const Entry* entry);
    QString autoTypeSequence(const Entry* entry, const QString& windowTitle = QString());
    bool windowMatchesTitle(const QString& windowTitle, const QString& resolvedTitle);
    bool windowMatchesUrl(const QString& windowTitle, const QString& resolvedUrl);
    bool windowMatches(const QString& windowTitle, const QString& windowPattern);

    bool m_inAutoType;
    int m_autoTypeDelay;
    Qt::Key m_currentGlobalKey;
    Qt::KeyboardModifiers m_currentGlobalModifiers;
    QPluginLoader* m_pluginLoader;
    AutoTypePlatformInterface* m_plugin;
    AutoTypeExecutor* m_executor;
    WId m_windowFromGlobal;
    static AutoType* m_instance;

    Q_DISABLE_COPY(AutoType)
};

inline AutoType* autoType() {
    return AutoType::instance();
}

#endif // KEEPASSX_AUTOTYPE_H
