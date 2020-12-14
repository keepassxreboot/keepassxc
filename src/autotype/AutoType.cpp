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

#include "AutoType.h"

#include <QApplication>
#include <QPluginLoader>
#include <QRegularExpression>

#include "config-keepassx.h"

#include "autotype/AutoTypePlatformPlugin.h"
#include "autotype/AutoTypeSelectDialog.h"
#include "autotype/WildcardMatcher.h"
#include "core/AutoTypeMatch.h"
#include "core/Config.h"
#include "core/Database.h"
#include "core/Entry.h"
#include "core/Group.h"
#include "core/ListDeleter.h"
#include "core/Resources.h"
#include "core/Tools.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "gui/osutils/OSUtils.h"

AutoType* AutoType::m_instance = nullptr;

AutoType::AutoType(QObject* parent, bool test)
    : QObject(parent)
    , m_autoTypeDelay(0)
    , m_pluginLoader(new QPluginLoader(this))
    , m_plugin(nullptr)
    , m_executor(nullptr)
    , m_windowState(WindowState::Normal)
    , m_windowForGlobal(0)
{
    // prevent crash when the plugin has unresolved symbols
    m_pluginLoader->setLoadHints(QLibrary::ResolveAllSymbolsHint);

    QString pluginName = "keepassx-autotype-";
    if (!test) {
        pluginName += QApplication::platformName();
    } else {
        pluginName += "test";
    }

    QString pluginPath = resources()->pluginPath(pluginName);

    if (!pluginPath.isEmpty()) {
#ifdef WITH_XC_AUTOTYPE
        loadPlugin(pluginPath);
#endif
    }

    connect(qApp, SIGNAL(aboutToQuit()), SLOT(unloadPlugin()));
}

AutoType::~AutoType()
{
    if (m_executor) {
        delete m_executor;
        m_executor = nullptr;
    }
}

void AutoType::loadPlugin(const QString& pluginPath)
{
    m_pluginLoader->setFileName(pluginPath);

    QObject* pluginInstance = m_pluginLoader->instance();
    if (pluginInstance) {
        m_plugin = qobject_cast<AutoTypePlatformInterface*>(pluginInstance);
        m_executor = nullptr;

        if (m_plugin) {
            if (m_plugin->isAvailable()) {
                m_executor = m_plugin->createExecutor();
                connect(osUtils, &OSUtilsBase::globalShortcutTriggered, this, [this](QString name) {
                    if (name == "autotype") {
                        startGlobalAutoType();
                    }
                });
            } else {
                unloadPlugin();
            }
        }
    }

    if (!m_plugin) {
        qWarning("Unable to load auto-type plugin:\n%s", qPrintable(m_pluginLoader->errorString()));
    }
}

void AutoType::unloadPlugin()
{
    if (m_executor) {
        delete m_executor;
        m_executor = nullptr;
    }

    if (m_plugin) {
        m_plugin->unload();
        m_plugin = nullptr;
    }
}

AutoType* AutoType::instance()
{
    if (!m_instance) {
        m_instance = new AutoType(qApp);
    }

    return m_instance;
}

void AutoType::createTestInstance()
{
    Q_ASSERT(!m_instance);

    m_instance = new AutoType(qApp, true);
}

QStringList AutoType::windowTitles()
{
    if (!m_plugin) {
        return QStringList();
    }

    return m_plugin->windowTitles();
}

void AutoType::raiseWindow()
{
#if defined(Q_OS_MACOS)
    m_plugin->raiseOwnWindow();
#endif
}

bool AutoType::registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers, QString* error)
{
    if (!m_plugin) {
        return false;
    }

    return osUtils->registerGlobalShortcut("autotype", key, modifiers, error);
}

void AutoType::unregisterGlobalShortcut()
{
    osUtils->unregisterGlobalShortcut("autotype");
}

/**
 * Core Autotype function that will execute actions
 */
void AutoType::executeAutoTypeActions(const Entry* entry, QWidget* hideWindow, const QString& sequence, WId window)
{
    if (!m_inAutoType.tryLock()) {
        return;
    }

    // no edit to the sequence beyond this point
    if (!verifyAutoTypeSyntax(sequence)) {
        emit autotypeRejected();
        m_inAutoType.unlock();
        return;
    }

    QList<AutoTypeAction*> actions;
    ListDeleter<AutoTypeAction*> actionsDeleter(&actions);

    if (!parseActions(sequence, entry, actions)) {
        emit autotypeRejected();
        m_inAutoType.unlock();
        return;
    }

    if (hideWindow) {
#if defined(Q_OS_MACOS)
        // Check for accessibility permission
        if (!macUtils()->enableAccessibility()) {
            MessageBox::information(nullptr,
                                    tr("Permission Required"),
                                    tr("KeePassXC requires the Accessibility permission in order to perform entry "
                                       "level Auto-Type. If you already granted permission, you may have to restart "
                                       "KeePassXC."));
            return;
        }

        macUtils()->raiseLastActiveWindow();
        m_plugin->hideOwnWindow();
#else
        getMainWindow()->minimizeOrHide();
#endif
    }

    Tools::wait(qMax(100, config()->get(Config::AutoTypeStartDelay).toInt()));

    // Used only for selected entry auto-type
    if (!window) {
        window = m_plugin->activeWindow();
    }

    QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

    for (AutoTypeAction* action : asConst(actions)) {
        if (m_plugin->activeWindow() != window) {
            qWarning("Active window changed, interrupting auto-type.");
            emit autotypeRejected();
            m_inAutoType.unlock();
            return;
        }

        action->accept(m_executor);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    }

    m_windowForGlobal = 0;
    m_windowTitleForGlobal.clear();

    // emit signal only if autotype performed correctly
    emit autotypePerformed();

    m_inAutoType.unlock();
}

/**
 * Single Autotype entry-point function
 * Look up the Auto-Type sequence for the given entry then perfom Auto-Type in the active window
 */
void AutoType::performAutoType(const Entry* entry, QWidget* hideWindow)
{
    if (!m_plugin) {
        return;
    }

    QList<QString> sequences = autoTypeSequences(entry);
    if (sequences.isEmpty()) {
        return;
    }

    executeAutoTypeActions(entry, hideWindow, sequences.first());
}

/**
 * Extra Autotype entry-point function
 * Perfom Auto-Type of the directly specified sequence in the active window
 */
void AutoType::performAutoTypeWithSequence(const Entry* entry, const QString& sequence, QWidget* hideWindow)
{
    if (!m_plugin) {
        return;
    }

    executeAutoTypeActions(entry, hideWindow, sequence);
}

void AutoType::startGlobalAutoType()
{
    m_windowForGlobal = m_plugin->activeWindow();
    m_windowTitleForGlobal = m_plugin->activeWindowTitle();
#ifdef Q_OS_MACOS
    // Determine if the user has given proper permissions to KeePassXC to perform Auto-Type
    static bool accessibilityChecked = false;
    if (!accessibilityChecked) {
        if (macUtils()->enableAccessibility() && macUtils()->enableScreenRecording()) {
            accessibilityChecked = true;
        } else if (getMainWindow()) {
            // Does not have required permissions to Auto-Type, ignore the event
            MessageBox::information(
                nullptr,
                tr("Permission Required"),
                tr("KeePassXC requires the Accessibility and Screen Recorder permission in order to perform global "
                   "Auto-Type. Screen Recording is necessary to use the window title to find entries. If you "
                   "already granted permission, you may have to restart KeePassXC."));
            return;
        }
    }

    m_windowState = WindowState::Normal;
    if (getMainWindow()) {
        if (getMainWindow()->isMinimized()) {
            m_windowState = WindowState::Minimized;
        }
        if (getMainWindow()->isHidden()) {
            m_windowState = WindowState::Hidden;
        }
    }
#endif

    emit globalAutoTypeTriggered();
}

/**
 * Global Autotype entry-point function
 * Perform global Auto-Type on the active window
 */
void AutoType::performGlobalAutoType(const QList<QSharedPointer<Database>>& dbList)
{
    if (!m_plugin) {
        return;
    }

    if (!m_inGlobalAutoTypeDialog.tryLock()) {
        return;
    }

    if (m_windowTitleForGlobal.isEmpty()) {
        m_inGlobalAutoTypeDialog.unlock();
        return;
    }

    QList<AutoTypeMatch> matchList;
    bool hideExpired = config()->get(Config::AutoTypeHideExpiredEntry).toBool();

    for (const auto& db : dbList) {
        const QList<Entry*> dbEntries = db->rootGroup()->entriesRecursive();
        for (Entry* entry : dbEntries) {
            if (hideExpired && entry->isExpired()) {
                continue;
            }
            const QSet<QString> sequences = autoTypeSequences(entry, m_windowTitleForGlobal).toSet();
            for (const QString& sequence : sequences) {
                if (!sequence.isEmpty()) {
                    matchList << AutoTypeMatch(entry, sequence);
                }
            }
        }
    }

    if (matchList.isEmpty()) {
        if (qobject_cast<QApplication*>(QCoreApplication::instance())) {
            auto* msgBox = new QMessageBox();
            msgBox->setAttribute(Qt::WA_DeleteOnClose);
            msgBox->setWindowTitle(tr("Auto-Type - KeePassXC"));
            msgBox->setText(tr("Couldn't find an entry that matches the window title:")
                                .append("\n\n")
                                .append(m_windowTitleForGlobal));
            msgBox->setIcon(QMessageBox::Information);
            msgBox->setStandardButtons(QMessageBox::Ok);
#ifdef Q_OS_MACOS
            m_plugin->raiseOwnWindow();
            Tools::wait(200);
#endif
            msgBox->exec();
            restoreWindowState();
        }

        m_inGlobalAutoTypeDialog.unlock();
        emit autotypeRejected();
    } else if ((matchList.size() == 1) && !config()->get(Config::Security_AutoTypeAsk).toBool()) {
        executeAutoTypeActions(matchList.first().entry, nullptr, matchList.first().sequence, m_windowForGlobal);
        m_inGlobalAutoTypeDialog.unlock();
    } else {
        auto* selectDialog = new AutoTypeSelectDialog();

        // connect slots, both of which must unlock the m_inGlobalAutoTypeDialog mutex
        connect(selectDialog, SIGNAL(matchActivated(AutoTypeMatch)), SLOT(performAutoTypeFromGlobal(AutoTypeMatch)));
        connect(selectDialog, SIGNAL(rejected()), SLOT(autoTypeRejectedFromGlobal()));

        selectDialog->setMatchList(matchList);
#ifdef Q_OS_MACOS
        m_plugin->raiseOwnWindow();
        Tools::wait(200);
#endif
        selectDialog->show();
        selectDialog->raise();
        // necessary when the main window is minimized
        selectDialog->activateWindow();
    }
}

void AutoType::restoreWindowState()
{
#ifdef Q_OS_MAC
    if (getMainWindow()) {
        if (m_windowState == WindowState::Minimized) {
            getMainWindow()->showMinimized();
        } else if (m_windowState == WindowState::Hidden) {
            getMainWindow()->hideWindow();
        }
    }
#endif
}

void AutoType::performAutoTypeFromGlobal(AutoTypeMatch match)
{
    restoreWindowState();

    m_plugin->raiseWindow(m_windowForGlobal);
    executeAutoTypeActions(match.entry, nullptr, match.sequence, m_windowForGlobal);

    // make sure the mutex is definitely locked before we unlock it
    Q_UNUSED(m_inGlobalAutoTypeDialog.tryLock());
    m_inGlobalAutoTypeDialog.unlock();
}

void AutoType::autoTypeRejectedFromGlobal()
{
    // this slot can be called twice when the selection dialog is deleted,
    // so make sure the mutex is locked before we try unlocking it
    Q_UNUSED(m_inGlobalAutoTypeDialog.tryLock());
    m_inGlobalAutoTypeDialog.unlock();
    m_windowForGlobal = 0;
    m_windowTitleForGlobal.clear();

    restoreWindowState();
    emit autotypeRejected();
}

/**
 * Parse an autotype sequence and resolve its Template/command inside as AutoTypeActions
 */
bool AutoType::parseActions(const QString& actionSequence, const Entry* entry, QList<AutoTypeAction*>& actions)
{
    QString tmpl;
    bool inTmpl = false;
    m_autoTypeDelay = qMax(config()->get(Config::AutoTypeDelay).toInt(), 0);

    QString sequence = actionSequence;
    sequence.replace("{{}", "{LEFTBRACE}");
    sequence.replace("{}}", "{RIGHTBRACE}");

    for (const QChar& ch : sequence) {
        if (inTmpl) {
            if (ch == '{') {
                qWarning("Syntax error in Auto-Type sequence.");
                return false;
            } else if (ch == '}') {
                QList<AutoTypeAction*> autoType = createActionFromTemplate(tmpl, entry);
                if (!autoType.isEmpty()) {
                    actions.append(autoType);
                }
                inTmpl = false;
                tmpl.clear();
            } else {
                tmpl += ch;
            }
        } else if (ch == '{') {
            inTmpl = true;
        } else if (ch == '}') {
            qWarning("Syntax error in Auto-Type sequence.");
            return false;
        } else {
            actions.append(new AutoTypeChar(ch));
        }
    }
    if (m_autoTypeDelay > 0) {
        QList<AutoTypeAction*>::iterator i;
        i = actions.begin();
        while (i != actions.end()) {
            ++i;
            if (i != actions.end()) {
                i = actions.insert(i, new AutoTypeDelay(m_autoTypeDelay));
                ++i;
            }
        }
    }
    return true;
}

/**
 * Convert an autotype Template/command to its AutoTypeAction that will be executed by the plugin executor
 */
QList<AutoTypeAction*> AutoType::createActionFromTemplate(const QString& tmpl, const Entry* entry)
{
    QString tmplName = tmpl;
    int num = -1;
    QList<AutoTypeAction*> list;

    QRegExp delayRegEx("delay=(\\d+)", Qt::CaseInsensitive, QRegExp::RegExp2);
    if (delayRegEx.exactMatch(tmplName)) {
        num = delayRegEx.cap(1).toInt();
        m_autoTypeDelay = std::max(0, std::min(num, 10000));
        return list;
    }

    QRegExp repeatRegEx("(.+) (\\d+)", Qt::CaseInsensitive, QRegExp::RegExp2);
    if (repeatRegEx.exactMatch(tmplName)) {
        tmplName = repeatRegEx.cap(1);
        num = repeatRegEx.cap(2).toInt();

        if (num == 0) {
            return list;
        }
    }

    if (tmplName.compare("tab", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Tab));
    } else if (tmplName.compare("enter", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Enter));
    } else if (tmplName.compare("space", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Space));
    } else if (tmplName.compare("up", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Up));
    } else if (tmplName.compare("down", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Down));
    } else if (tmplName.compare("left", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Left));
    } else if (tmplName.compare("right", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Right));
    } else if (tmplName.compare("insert", Qt::CaseInsensitive) == 0
               || tmplName.compare("ins", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Insert));
    } else if (tmplName.compare("delete", Qt::CaseInsensitive) == 0
               || tmplName.compare("del", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Delete));
    } else if (tmplName.compare("home", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Home));
    } else if (tmplName.compare("end", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_End));
    } else if (tmplName.compare("pgup", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_PageUp));
    } else if (tmplName.compare("pgdown", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_PageDown));
    } else if (tmplName.compare("backspace", Qt::CaseInsensitive) == 0
               || tmplName.compare("bs", Qt::CaseInsensitive) == 0
               || tmplName.compare("bksp", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Backspace));
    } else if (tmplName.compare("break", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Pause));
    } else if (tmplName.compare("capslock", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_CapsLock));
    } else if (tmplName.compare("esc", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Escape));
    } else if (tmplName.compare("help", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Help));
    } else if (tmplName.compare("numlock", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_NumLock));
    } else if (tmplName.compare("ptrsc", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_Print));
    } else if (tmplName.compare("scrolllock", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeKey(Qt::Key_ScrollLock));
    }
    // Qt doesn't know about keypad keys so use the normal ones instead
    else if (tmplName.compare("add", Qt::CaseInsensitive) == 0 || tmplName.compare("+", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('+'));
    } else if (tmplName.compare("subtract", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('-'));
    } else if (tmplName.compare("multiply", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('*'));
    } else if (tmplName.compare("divide", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('/'));
    } else if (tmplName.compare("^", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('^'));
    } else if (tmplName.compare("%", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('%'));
    } else if (tmplName.compare("~", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('~'));
    } else if (tmplName.compare("(", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('('));
    } else if (tmplName.compare(")", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar(')'));
    } else if (tmplName.compare("leftbrace", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('{'));
    } else if (tmplName.compare("rightbrace", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeChar('}'));
    } else {
        QRegExp fnRegexp("f(\\d+)", Qt::CaseInsensitive, QRegExp::RegExp2);
        if (fnRegexp.exactMatch(tmplName)) {
            int fnNo = fnRegexp.cap(1).toInt();
            if (fnNo >= 1 && fnNo <= 16) {
                list.append(new AutoTypeKey(static_cast<Qt::Key>(Qt::Key_F1 - 1 + fnNo)));
            }
        }
    }

    if (!list.isEmpty()) {
        for (int i = 1; i < num; i++) {
            list.append(list.at(0)->clone());
        }
        return list;
    }

    if (tmplName.compare("delay", Qt::CaseInsensitive) == 0 && num > 0) {
        list.append(new AutoTypeDelay(num));
    } else if (tmplName.compare("clearfield", Qt::CaseInsensitive) == 0) {
        list.append(new AutoTypeClearField());
    } else if (tmplName.compare("totp", Qt::CaseInsensitive) == 0) {
        QString totp = entry->totp();
        if (!totp.isEmpty()) {
            for (const QChar& ch : totp) {
                list.append(new AutoTypeChar(ch));
            }
        }
    }

    if (!list.isEmpty()) {
        return list;
    }

    const QString placeholder = QString("{%1}").arg(tmplName);
    const QString resolved = entry->resolvePlaceholder(placeholder);
    if (placeholder != resolved) {
        for (const QChar& ch : resolved) {
            if (ch == '\n') {
                list.append(new AutoTypeKey(Qt::Key_Enter));
            } else if (ch == '\t') {
                list.append(new AutoTypeKey(Qt::Key_Tab));
            } else {
                list.append(new AutoTypeChar(ch));
            }
        }
    }

    return list;
}

/**
 * Retrive the autotype sequences matches for a given windowTitle
 * This returns a list with priority ordering. If you don't want duplicates call .toSet() on it.
 */
QList<QString> AutoType::autoTypeSequences(const Entry* entry, const QString& windowTitle)
{
    QList<QString> sequenceList;
    const Group* group = entry->group();

    if (!group || !entry->autoTypeEnabled()) {
        return sequenceList;
    }

    do {
        if (group->autoTypeEnabled() == Group::Disable) {
            return sequenceList;
        } else if (group->autoTypeEnabled() == Group::Enable) {
            break;
        }
        group = group->parentGroup();

    } while (group);

    if (!windowTitle.isEmpty()) {
        const QList<AutoTypeAssociations::Association> assocList = entry->autoTypeAssociations()->getAll();
        for (const AutoTypeAssociations::Association& assoc : assocList) {
            const QString window = entry->resolveMultiplePlaceholders(assoc.window);
            if (windowMatches(windowTitle, window)) {
                if (!assoc.sequence.isEmpty()) {
                    sequenceList.append(assoc.sequence);
                } else {
                    sequenceList.append(entry->effectiveAutoTypeSequence());
                }
            }
        }

        if (config()->get(Config::AutoTypeEntryTitleMatch).toBool()
            && windowMatchesTitle(windowTitle, entry->resolvePlaceholder(entry->title()))) {
            sequenceList.append(entry->effectiveAutoTypeSequence());
        }

        if (config()->get(Config::AutoTypeEntryURLMatch).toBool()
            && windowMatchesUrl(windowTitle, entry->resolvePlaceholder(entry->url()))) {
            sequenceList.append(entry->effectiveAutoTypeSequence());
        }

        if (sequenceList.isEmpty()) {
            return sequenceList;
        }
    } else {
        sequenceList.append(entry->effectiveAutoTypeSequence());
    }

    return sequenceList;
}

/**
 * Checks if a window title matches a pattern
 */
bool AutoType::windowMatches(const QString& windowTitle, const QString& windowPattern)
{
    if (windowPattern.startsWith("//") && windowPattern.endsWith("//") && windowPattern.size() >= 4) {
        QRegExp regExp(windowPattern.mid(2, windowPattern.size() - 4), Qt::CaseInsensitive, QRegExp::RegExp2);
        return (regExp.indexIn(windowTitle) != -1);
    }
    return WildcardMatcher(windowTitle).match(windowPattern);
}

/**
 * Checks if a window title matches an entry Title
 * The entry title should be Spr-compiled by the caller
 */
bool AutoType::windowMatchesTitle(const QString& windowTitle, const QString& resolvedTitle)
{
    return !resolvedTitle.isEmpty() && windowTitle.contains(resolvedTitle, Qt::CaseInsensitive);
}

/**
 * Checks if a window title matches an entry URL
 * The entry URL should be Spr-compiled by the caller
 */
bool AutoType::windowMatchesUrl(const QString& windowTitle, const QString& resolvedUrl)
{
    if (!resolvedUrl.isEmpty() && windowTitle.contains(resolvedUrl, Qt::CaseInsensitive)) {
        return true;
    }

    QUrl url(resolvedUrl);
    if (url.isValid() && !url.host().isEmpty()) {
        return windowTitle.contains(url.host(), Qt::CaseInsensitive);
    }

    return false;
}

/**
 * Checks if the overall syntax of an autotype sequence is fine
 */
bool AutoType::checkSyntax(const QString& string)
{
    QString allowRepetition = "(?:\\s\\d+)?";
    // the ":" allows custom commands with syntax S:Field
    // exclude BEEP otherwise will be checked as valid
    QString normalCommands = "(?!BEEP\\s)[A-Z:_]*" + allowRepetition;
    QString specialLiterals = "[\\^\\%\\(\\)~\\{\\}\\[\\]\\+]" + allowRepetition;
    QString functionKeys = "(?:F[1-9]" + allowRepetition + "|F1[0-2])" + allowRepetition;
    QString numpad = "NUMPAD\\d" + allowRepetition;
    QString delay = "DELAY=\\d+";
    QString beep = "BEEP\\s\\d+\\s\\d+";
    QString vkey = "VKEY(?:-[EN]X)?\\s\\w+";
    QString customAttributes = "S:(?:[^\\{\\}])+";

    // these chars aren't in parentheses
    QString shortcutKeys = "[\\^\\%~\\+@]";
    // a normal string not in parentheses
    QString fixedStrings = "[^\\^\\%~\\+@\\{\\}]*";
    // clang-format off
    QRegularExpression autoTypeSyntax(
        "^(?:" + shortcutKeys + "|" + fixedStrings + "|\\{(?:" + normalCommands + "|" + specialLiterals + "|"
            + functionKeys
            + "|"
            + numpad
            + "|"
            + delay
            + "|"
            + beep
            + "|"
            + vkey
            + ")\\}|\\{"
            + customAttributes
            + "\\})*$",
        QRegularExpression::CaseInsensitiveOption);
    // clang-format on
    QRegularExpressionMatch match = autoTypeSyntax.match(string);
    return match.hasMatch();
}

/**
 * Checks an autotype sequence for high delay
 */
bool AutoType::checkHighDelay(const QString& string)
{
    // 5 digit numbers(10 seconds) are too much
    QRegularExpression highDelay("\\{DELAY\\s\\d{5,}\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = highDelay.match(string);
    return match.hasMatch();
}

/**
 * Checks an autotype sequence for slow keypress
 */
bool AutoType::checkSlowKeypress(const QString& string)
{
    // 3 digit numbers(100 milliseconds) are too much
    QRegularExpression slowKeypress("\\{DELAY=\\d{3,}\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = slowKeypress.match(string);
    return match.hasMatch();
}

/**
 * Checks an autotype sequence for high repetition command
 */
bool AutoType::checkHighRepetition(const QString& string)
{
    // 3 digit numbers are too much
    QRegularExpression highRepetition("\\{(?!DELAY\\s)\\w+\\s\\d{3,}\\}", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = highRepetition.match(string);
    return match.hasMatch();
}

/**
 * Verify if the syntax of an autotype sequence is correct and doesn't have silly parameters
 */
bool AutoType::verifyAutoTypeSyntax(const QString& sequence)
{
    if (!AutoType::checkSyntax(sequence)) {
        QMessageBox::critical(nullptr, tr("Auto-Type"), tr("The Syntax of your Auto-Type statement is incorrect!"));
        return false;
    } else if (AutoType::checkHighDelay(sequence)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(
            nullptr,
            tr("Auto-Type"),
            tr("This Auto-Type command contains a very long delay. Do you really want to proceed?"));

        if (reply == QMessageBox::No) {
            return false;
        }
    } else if (AutoType::checkSlowKeypress(sequence)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(
            nullptr,
            tr("Auto-Type"),
            tr("This Auto-Type command contains very slow key presses. Do you really want to proceed?"));

        if (reply == QMessageBox::No) {
            return false;
        }
    } else if (AutoType::checkHighRepetition(sequence)) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(nullptr,
                                      tr("Auto-Type"),
                                      tr("This Auto-Type command contains arguments which are "
                                         "repeated very often. Do you really want to proceed?"));

        if (reply == QMessageBox::No) {
            return false;
        }
    }
    return true;
}
