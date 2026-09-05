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
#include <QDebug>
#include <QRegularExpression>
#if defined(Q_OS_WIN32)
#include <QScopeGuard>
#endif

#include "config-keepassx.h"

#include "autotype/AutoTypePlatform.h"
#include "autotype/AutoTypeSelectDialog.h"
#include "autotype/PickcharsDialog.h"
#include "autotype/test/AutoTypeTest.h"
#include "core/Config.h"
#include "core/Global.h"
#include "core/Tools.h"
#include "core/Totp.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "gui/osutils/OSUtils.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
#ifdef WITH_X11
#include "autotype/xcb/AutoTypeXCB.h"
#endif
#include "autotype/wayland/AutoTypeWayland.h"
#elif defined(Q_OS_MACOS)
#include "autotype/mac/AutoTypeMac.h"
#elif defined(Q_OS_WIN32)
#include "autotype/windows/AutoTypeWindows.h"
#endif

namespace
{
    // Basic Auto-Type placeholder associations
    const QHash<QString, Qt::Key> g_placeholderToKey = {{"tab", Qt::Key_Tab},
                                                        {"enter", Qt::Key_Enter},
                                                        {"space", Qt::Key_Space},
                                                        {"up", Qt::Key_Up},
                                                        {"down", Qt::Key_Down},
                                                        {"left", Qt::Key_Left},
                                                        {"right", Qt::Key_Right},
                                                        {"insert", Qt::Key_Insert},
                                                        {"ins", Qt::Key_Insert},
                                                        {"delete", Qt::Key_Delete},
                                                        {"del", Qt::Key_Delete},
                                                        {"home", Qt::Key_Home},
                                                        {"end", Qt::Key_End},
                                                        {"pgup", Qt::Key_PageUp},
                                                        {"pgdown", Qt::Key_PageDown},
                                                        {"pgdn", Qt::Key_PageDown},
                                                        {"backspace", Qt::Key_Backspace},
                                                        {"bs", Qt::Key_Backspace},
                                                        {"bksp", Qt::Key_Backspace},
                                                        {"break", Qt::Key_Pause},
                                                        {"capslock", Qt::Key_CapsLock},
                                                        {"esc", Qt::Key_Escape},
                                                        {"help", Qt::Key_Help},
                                                        {"numlock", Qt::Key_NumLock},
                                                        {"ptrsc", Qt::Key_Print},
                                                        {"scrolllock", Qt::Key_ScrollLock},
                                                        {"win", Qt::Key_Meta},
                                                        {"lwin", Qt::Key_Meta},
                                                        {"rwin", Qt::Key_Meta},
                                                        {"apps", Qt::Key_Menu},
                                                        {"help", Qt::Key_Help},
                                                        {"add", Qt::Key_Plus},
                                                        {"subtract", Qt::Key_Minus},
                                                        {"multiply", Qt::Key_Asterisk},
                                                        {"divide", Qt::Key_Slash},
                                                        {"leftbrace", Qt::Key_BraceLeft},
                                                        {"{", Qt::Key_unknown},
                                                        {"rightbrace", Qt::Key_BraceRight},
                                                        {"}", Qt::Key_unknown},
                                                        {"leftparen", Qt::Key_ParenLeft},
                                                        {"(", Qt::Key_unknown},
                                                        {"rightparen", Qt::Key_ParenRight},
                                                        {")", Qt::Key_unknown},
                                                        {"[", Qt::Key_unknown},
                                                        {"]", Qt::Key_unknown},
                                                        {"+", Qt::Key_unknown},
                                                        {"%", Qt::Key_unknown},
                                                        {"^", Qt::Key_unknown},
                                                        {"~", Qt::Key_unknown},
                                                        {"#", Qt::Key_unknown},
                                                        {"numpad0", Qt::Key_0},
                                                        {"numpad1", Qt::Key_1},
                                                        {"numpad2", Qt::Key_2},
                                                        {"numpad3", Qt::Key_3},
                                                        {"numpad4", Qt::Key_4},
                                                        {"numpad5", Qt::Key_5},
                                                        {"numpad6", Qt::Key_6},
                                                        {"numpad7", Qt::Key_7},
                                                        {"numpad8", Qt::Key_8},
                                                        {"numpad9", Qt::Key_9},
                                                        {"f1", Qt::Key_F1},
                                                        {"f2", Qt::Key_F2},
                                                        {"f3", Qt::Key_F3},
                                                        {"f4", Qt::Key_F4},
                                                        {"f5", Qt::Key_F5},
                                                        {"f6", Qt::Key_F6},
                                                        {"f7", Qt::Key_F7},
                                                        {"f8", Qt::Key_F8},
                                                        {"f9", Qt::Key_F9},
                                                        {"f10", Qt::Key_F10},
                                                        {"f11", Qt::Key_F11},
                                                        {"f12", Qt::Key_F12},
                                                        {"f13", Qt::Key_F13},
                                                        {"f14", Qt::Key_F14},
                                                        {"f15", Qt::Key_F15},
                                                        {"f16", Qt::Key_F16}};
    constexpr int s_minWaitDelay = 100; // 100 ms
    constexpr int s_maxWaitDelay = 10000; // 10 seconds
} // namespace

AutoType* AutoType::m_instance = nullptr;

AutoType::AutoType(QObject* parent, bool test)
    : QObject(parent)
    , m_platform(nullptr)
    , m_windowState(WindowState::Normal)
    , m_windowForGlobal(0)
    , m_lastMatch(nullptr, QString())
    , m_lastMatchRetypeTimer(nullptr)
{
    // configure timer to reset last match
    m_lastMatchRetypeTimer.setSingleShot(true);
    connect(&m_lastMatchRetypeTimer, &QTimer::timeout, this, [this] {
        m_lastMatch = {nullptr, QString()};
        emit autotypeRetypeTimeout();
    });

    if (test) {
        m_platform = new AutoTypePlatformTest();
    } else {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
        const auto platformName = QApplication::platformName();
#ifdef WITH_X11
        if (platformName == "xcb") {
            if (config()->get(Config::AutoTypePreferDesktopPortals).toBool()) {
                m_platform = new AutoTypePlatformWayland();
                if (!m_platform->isAvailable()) {
                    delete m_platform;
                    m_platform = new AutoTypePlatformX11();
                }
            } else {
                m_platform = new AutoTypePlatformX11();
            }
        } else {
            m_platform = new AutoTypePlatformWayland();
        }
#else
        Q_UNUSED(platformName);
        m_platform = new AutoTypePlatformWayland();
#endif
#elif defined(Q_OS_MACOS)
        m_platform = new AutoTypePlatformMac();
#elif defined(Q_OS_WIN32)
        m_platform = new AutoTypePlatformWin();
#endif
    }

    if (m_platform) {
        if (!m_platform->isAvailable()) {
            delete m_platform;
            m_platform = nullptr;
        }
    }

    if (!m_platform) {
        qWarning("Unable to initialize an available auto-type platform.");
    }

    connect(osUtils,
            &OSUtilsBase::globalShortcutTriggered,
            this,
            [this](const QString& name, const QString& initialSearch) {
                if (name == "autotype") {
                    startGlobalAutoType(initialSearch);
                }
            });
    connect(this, SIGNAL(autotypeFinished()), SLOT(resetAutoTypeState()));
    connect(this, &AutoType::autotypeFinished, this, [this] {
        if (m_platform) {
            m_platform->finishAutoType();
        }
    });
    connect(qApp, SIGNAL(aboutToQuit()), SLOT(unload()));
}

AutoType::~AutoType()
{
    unload();
}

bool AutoType::usesDesktopPortal() const
{
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
    return dynamic_cast<AutoTypePlatformWayland*>(m_platform) != nullptr;
#else
    return false;
#endif
}

void AutoType::unload()
{
    if (m_platform) {
        delete m_platform;
        m_platform = nullptr;
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
    if (!m_platform) {
        return {};
    }

    return m_platform->windowTitles();
}

void AutoType::raiseWindow()
{
#if defined(Q_OS_MACOS)
    if (m_platform) {
        m_platform->raiseOwnWindow();
    }
#endif
}

bool AutoType::registerGlobalShortcut(Qt::Key key, Qt::KeyboardModifiers modifiers, QString* error)
{
    if (!m_platform) {
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
void AutoType::executeAutoTypeActions(const Entry* entry,
                                      const QString& sequence,
                                      WId window,
                                      AutoTypeExecutor::Mode mode)
{
    if (!m_platform) {
        qWarning() << "Auto-Type platform not available, cannot perform Auto-Type.";
        return;
    }

    QString error;
    auto actions = parseSequence(sequence, entry, error);

    if (!error.isEmpty()) {
        auto errorMsg = tr("The requested Auto-Type sequence cannot be used due to an error:");
        errorMsg.append(QString("\n%1\n%2").arg(sequence, error));
        if (getMainWindow()) {
            MessageBox::critical(getMainWindow(), tr("Auto-Type Error"), errorMsg);
        }
        qWarning() << errorMsg;
        emit autotypeFinished();
        return;
    }

    if (!m_inAutoType.tryLock()) {
        return;
    }

    // Explicitly hide the main window if no target window is specified
    if (window == 0) {
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
        m_platform->hideOwnWindow();
#else
        if (getMainWindow()) {
            getMainWindow()->minimizeOrHide();
        }
#endif
    } else {
        // Restore window state (macOS only) then raise the target window
        restoreWindowState();
        QCoreApplication::processEvents();
        m_platform->raiseWindow(window);
    }

    // Restore executor mode
    m_platform->executor().mode = mode;

    // Initial Auto-Type delay to allow window to come to foreground
    Tools::wait(qBound(s_minWaitDelay, config()->get(Config::AutoTypeStartDelay).toInt(), s_maxWaitDelay));

    // Grab the current active window after everything settles
    if (window == 0) {
        window = m_platform->activeWindow();
    }

    // Scoped: on Windows the helper is torn down before the lock is released and
    // autotypeFinished() fires, so a synchronously started next sequence is not
    // torn down by this guard.
    {
#if defined(Q_OS_WIN32)
        // The first point at which the target is known on both paths.
        m_platform->beginSequence(window);
        // Guarded: the loop pumps events and can be left through a nested loop or a quit.
        bool ended = false;
        const QScopeGuard endSequence = qScopeGuard([this, &ended] {
            if (!ended) {
                m_platform->endSequence();
            }
        });
#endif

        for (const auto& action : asConst(actions)) {
            // Cancel Auto-Type if the active window changed
            if (m_platform->activeWindow() != window) {
                qWarning("Active window changed, interrupting auto-type.");
                break;
            }

            bool failed = false;
            constexpr int max_retries = 5;
            for (int i = 1; i <= max_retries; i++) {
                auto result = action->exec(m_platform->executor());

                QCoreApplication::processEvents(QEventLoop::AllEvents, 10);

                if (result.isOk()) {
                    break;
                }

                if (!result.canRetry() || i == max_retries) {
                    if (getMainWindow()) {
                        MessageBox::critical(getMainWindow(), tr("Auto-Type Error"), result.errorString());
                    }
                    failed = true;
                    break;
                }

                // Retry wait delay
                Tools::wait(100);
            }

            // Last action failed to complete, cancel the rest of the sequence
            if (failed) {
                break;
            }
        }

#if defined(Q_OS_WIN32)
        // Explicit so the result can be shown; the guard covers the other exits.
        ended = true;
        const auto endResult = m_platform->endSequence();
        if (!endResult.isOk() && getMainWindow()) {
            MessageBox::critical(getMainWindow(), tr("Auto-Type Error"), endResult.errorString());
        }
#endif
    }

    m_inAutoType.unlock();
    emit autotypeFinished();
}

/**
 * Single Autotype entry-point function
 * Look up the Auto-Type sequence for the given entry then perform Auto-Type in the active window
 */
void AutoType::performAutoType(const Entry* entry)
{
    if (!m_platform) {
        return;
    }

    auto sequences = entry->autoTypeSequences();
    if (!sequences.isEmpty()) {
        executeAutoTypeActions(entry, sequences.first());
    }
}

/**
 * Extra Autotype entry-point function
 * Perform Auto-Type of the directly specified sequence in the active window
 */
void AutoType::performAutoTypeWithSequence(const Entry* entry, const QString& sequence)
{
    if (!m_platform) {
        return;
    }

    executeAutoTypeActions(entry, sequence);
}

void AutoType::startGlobalAutoType(const QString& search)
{
    if (!m_platform) {
        return;
    }

    // Never Auto-Type into KeePassXC itself
    if (getMainWindow() && (qApp->activeWindow() || qApp->activeModalWidget())) {
        return;
    }

    m_windowForGlobal = m_platform->activeWindow();
    m_windowTitleForGlobal = m_platform->activeWindowTitle();
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

    emit globalAutoTypeTriggered(search);
}

/**
 * Global Autotype entry-point function
 * Perform global Auto-Type on the active window
 */
void AutoType::performGlobalAutoType(const QList<QSharedPointer<Database>>& dbList, const QString& search)
{
    if (!m_platform || !m_inGlobalAutoTypeDialog.tryLock()) {
        return;
    }

    QList<AutoTypeMatch> matchList;
    // Generate entry/sequence match list if there is a valid window title
    if (!m_windowTitleForGlobal.isEmpty()) {
        bool hideExpired = config()->get(Config::AutoTypeHideExpiredEntry).toBool();
        for (const auto& db : dbList) {
            const QList<Entry*> dbEntries = db->rootGroup()->entriesRecursive();
            for (auto entry : dbEntries) {
                auto group = entry->group();
                if (!group || !group->resolveAutoTypeEnabled() || !entry->autoTypeEnabled()) {
                    continue;
                }

                if (hideExpired && entry->isExpired()) {
                    continue;
                }
                const QSet<QString> sequences = Tools::asSet(entry->autoTypeSequences(m_windowTitleForGlobal));
                for (const auto& sequence : sequences) {
                    matchList << AutoTypeMatch(entry, sequence);
                }
            }
        }
    } else {
        qWarning() << "Auto-Type: Window title was empty from the operating system";
    }

    // Show the selection dialog if we always ask, have multiple matches, no matches, or the window title was empty
    if (getMainWindow()
        && (config()->get(Config::Security_AutoTypeAsk).toBool() || matchList.size() > 1 || matchList.isEmpty()
            || m_windowTitleForGlobal.isEmpty())) {
        // Close any open modal windows that would interfere with the process
        getMainWindow()->closeModalWindow();

        auto* selectDialog = new AutoTypeSelectDialog();
        selectDialog->setMatches(matchList, dbList, m_lastMatch);

        if (!search.isEmpty()) {
            selectDialog->setSearchString(search);
        }

        connect(getMainWindow(), &MainWindow::databaseLocked, selectDialog, &AutoTypeSelectDialog::reject);
        connect(
            selectDialog,
            &AutoTypeSelectDialog::matchActivated,
            this,
            [this](const AutoTypeMatch& match, bool virtualMode) {
                m_lastMatch = match;
                m_lastMatchRetypeTimer.start(config()->get(Config::GlobalAutoTypeRetypeTime).toInt() * 1000);
                executeAutoTypeActions(match.first,
                                       match.second,
                                       m_windowForGlobal,
                                       virtualMode ? AutoTypeExecutor::Mode::VIRTUAL : AutoTypeExecutor::Mode::NORMAL);
            },
            Qt::QueuedConnection);
        connect(selectDialog, &QDialog::rejected, this, [this] {
            restoreWindowState();
            emit autotypeFinished();
        });

#ifdef Q_OS_MACOS
        m_platform->raiseOwnWindow();
        Tools::wait(50);
#endif
        selectDialog->show();
        selectDialog->raise();
        selectDialog->activateWindow();
        m_platform->prepareAutoType();
    } else if (!matchList.isEmpty()) {
        // Only one match and not asking, do it!
        executeAutoTypeActions(matchList.first().first, matchList.first().second, m_windowForGlobal);
    } else {
        // We should never get here
        emit autotypeFinished();
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

void AutoType::resetAutoTypeState()
{
    m_windowForGlobal = 0;
    m_windowTitleForGlobal.clear();
    Q_UNUSED(m_inGlobalAutoTypeDialog.tryLock());
    m_inGlobalAutoTypeDialog.unlock();
}

/**
 * Parse an autotype sequence into a list of AutoTypeActions.
 * If error is provided then syntax checking will be performed.
 */
QList<QSharedPointer<AutoTypeAction>>
AutoType::parseSequence(const QString& entrySequence, const Entry* entry, QString& error, bool syntaxOnly)
{
    if (!entry) {
        error = tr("Invalid entry provided");
        return {};
    }

    const int maxTypeDelay = 500;
    const int maxRepetition = 100;

    int currentTypingDelay = qBound(0, config()->get(Config::AutoTypeDelay).toInt(), maxTypeDelay);
    // Take into account the initial delay which is added before any actions are performed
    int cumulativeDelay = qBound(s_minWaitDelay, config()->get(Config::AutoTypeStartDelay).toInt(), s_maxWaitDelay);

    // Initial actions include start delay and initial inter-key delay
    QList<QSharedPointer<AutoTypeAction>> actions;
    actions << QSharedPointer<AutoTypeBegin>::create();
    actions << QSharedPointer<AutoTypeDelay>::create(currentTypingDelay, true);

    // Replace escaped braces with a template for easier regex
    QString sequence = entrySequence;
    sequence.replace("{{}", "{LEFTBRACE}");
    sequence.replace("{}}", "{RIGHTBRACE}");

    // Quick test for bracket syntax
    if (sequence.count("{") != sequence.count("}")) {
        error = tr("Bracket imbalance detected, found extra { or }");
        return {};
    }

    // Group 1 = modifier key (opt)
    // Group 2 = full placeholder
    // Group 3 = inner placeholder (allows nested placeholders)
    // Group 4 = repeat / delay time (opt)
    // Group 5 = character
    QRegularExpression regex("([+%^#]*)(?:({((?>[^{}]+?|(?2))+?)(?:\\s+(\\d+))?})|(.))");
    auto results = regex.globalMatch(sequence);
    while (results.hasNext()) {
        auto match = results.next();

        // Parse modifier keys
        Qt::KeyboardModifiers modifiers;
        if (match.captured(1).contains('+')) {
            modifiers |= Qt::ShiftModifier;
        }
        if (match.captured(1).contains('^')) {
            modifiers |= Qt::ControlModifier;
        }
        if (match.captured(1).contains('%')) {
            modifiers |= Qt::AltModifier;
        }
        if (match.captured(1).contains('#')) {
            modifiers |= Qt::MetaModifier;
        }

        const auto fullPlaceholder = match.captured(2);
        auto placeholder = match.captured(3).toLower();
        auto repeat = 1;
        if (!match.captured(4).isEmpty()) {
            repeat = match.captured(4).toInt();
        }
        auto character = match.captured(5);

        if (placeholder.isEmpty()) {
            if (!character.isEmpty()) {
                // Type a single character with modifier
                actions << QSharedPointer<AutoTypeKey>::create(character[0], modifiers);
            }
            continue;
        }

        if (g_placeholderToKey.contains(placeholder)) {
            // Basic key placeholder, allow repeat
            if (repeat > maxRepetition) {
                error = tr("Too many repetitions detected, max is %1: %2").arg(maxRepetition).arg(fullPlaceholder);
                return {};
            }
            QSharedPointer<AutoTypeKey> action;
            if (g_placeholderToKey[placeholder] == Qt::Key_unknown) {
                action = QSharedPointer<AutoTypeKey>::create(placeholder[0], modifiers);
            } else {
                action = QSharedPointer<AutoTypeKey>::create(g_placeholderToKey[placeholder], modifiers);
            }
            for (int i = 1; i <= repeat && i <= maxRepetition; ++i) {
                actions << action;
            }
        } else if (placeholder.startsWith("delay=")) {
            // Change keypress delay
            int delay = placeholder.replace("delay=", "").toInt();
            if (delay > maxTypeDelay) {
                error = tr("Very slow key press detected, max is %1: %2").arg(maxTypeDelay).arg(fullPlaceholder);
                return {};
            }
            actions << QSharedPointer<AutoTypeDelay>::create(qBound(0, delay, maxTypeDelay), true);
        } else if (placeholder == "delay") {
            // Mid typing delay (wait), repeat represents the desired delay in milliseconds
            if (repeat > s_maxWaitDelay) {
                error = tr("Very long delay detected, max is %1: %2").arg(s_maxWaitDelay).arg(fullPlaceholder);
                return {};
            }
            cumulativeDelay += repeat;
            actions << QSharedPointer<AutoTypeDelay>::create(qBound(0, repeat, s_maxWaitDelay));
        } else if (placeholder == "clearfield") {
            // Platform-specific field clearing
            actions << QSharedPointer<AutoTypeClearField>::create();
        } else if (placeholder == "totp" || placeholder == "timeotp") {
            if (entry->hasValidTotp()) {
                // Calculate TOTP at the time of typing including delays
                bool isValid = false;
                auto time =
                    Clock::currentSecondsSinceEpoch() + (cumulativeDelay + currentTypingDelay * actions.count()) / 1000;
                auto totp = Totp::generateTotp(entry->totpSettings(), &isValid, time);
                for (const auto& ch : totp) {
                    actions << QSharedPointer<AutoTypeKey>::create(ch);
                }
            } else if (entry->hasTotp()) {
                // Entry has TOTP configured but invalid settings
                error = tr("Entry has invalid TOTP settings");
                return {};
            }
        } else if (placeholder.startsWith("pickchars")) {
            // Reset to the original capture to preserve case
            placeholder = match.captured(3);

            auto attribute = EntryAttributes::PasswordKey;
            if (placeholder.contains(":")) {
                attribute = placeholder.section(":", 1);
                if (!entry->attributes()->hasKey(attribute)) {
                    error = tr("Entry does not have attribute for PICKCHARS: %1").arg(attribute);
                    return {};
                }
            }

            // Bail out if we are just syntax checking
            if (syntaxOnly) {
                continue;
            }

            // Show pickchars dialog for the desired attribute
            auto password = entry->resolvePlaceholder(entry->attribute(attribute));
            if (!password.isEmpty()) {
                PickcharsDialog pickcharsDialog(password);
                if (pickcharsDialog.exec() == QDialog::Accepted && !pickcharsDialog.selectedChars().isEmpty()) {
                    for (const auto& ch : pickcharsDialog.selectedChars()) {
                        actions << QSharedPointer<AutoTypeKey>::create(ch);
                        if (pickcharsDialog.pressTab()) {
                            actions << QSharedPointer<AutoTypeKey>::create(g_placeholderToKey["tab"]);
                        }
                    }
                    if (pickcharsDialog.pressTab()) {
                        // Remove extra tab
                        actions.removeLast();
                    }
                }
            }
        } else if (placeholder.startsWith("t-conv:")) {
            // Reset to the original capture to preserve case
            placeholder = match.captured(3);
            auto resolved = entry->resolveConversionPlaceholder(placeholder, &error);
            if (!error.isEmpty()) {
                return {};
            }
            for (const QChar& ch : resolved) {
                actions << QSharedPointer<AutoTypeKey>::create(ch);
            }
        } else if (placeholder.startsWith("t-replace-rx:")) {
            // Reset to the original capture to preserve case
            placeholder = match.captured(3);
            auto resolved = entry->resolveRegexPlaceholder(placeholder, &error);
            if (!error.isEmpty()) {
                return {};
            }
            for (const QChar& ch : resolved) {
                actions << QSharedPointer<AutoTypeKey>::create(ch);
            }
        } else if (placeholder.startsWith("mode=")) {
            auto mode = AutoTypeExecutor::Mode::NORMAL;
            if (placeholder.endsWith("virtual")) {
                mode = AutoTypeExecutor::Mode::VIRTUAL;
            }
            actions << QSharedPointer<AutoTypeMode>::create(mode);
        } else if (placeholder.startsWith("beep") || placeholder.startsWith("vkey")
                   || placeholder.startsWith("appactivate") || placeholder.startsWith("c:")) {
            // Ignore these commands
        } else {
            // Attempt to resolve an entry attribute
            auto resolved = entry->resolvePlaceholder(fullPlaceholder);
            if (resolved != fullPlaceholder) {
                // Attribute resolved, add characters to action list
                for (const QChar& ch : resolved) {
                    if (ch == '\n') {
                        actions << QSharedPointer<AutoTypeKey>::create(g_placeholderToKey["enter"]);
                    } else if (ch == '\t') {
                        actions << QSharedPointer<AutoTypeKey>::create(g_placeholderToKey["tab"]);
                    } else {
                        actions << QSharedPointer<AutoTypeKey>::create(ch);
                    }
                }
            } else {
                // Invalid placeholder, issue error and stop processing
                error = tr("Invalid placeholder: %1").arg(fullPlaceholder);
                return {};
            }
        }
    }

    return actions;
}

/**
 * Verify if the syntax of an autotype sequence is correct and doesn't have invalid parameters
 */
bool AutoType::verifyAutoTypeSyntax(const QString& sequence, const Entry* entry, QString& error)
{
    error.clear();
    if (!sequence.isEmpty()) {
        parseSequence(sequence, entry, error, true);
    }
    return error.isEmpty();
}
