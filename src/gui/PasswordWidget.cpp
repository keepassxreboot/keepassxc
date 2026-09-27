/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "PasswordWidget.h"
#include "ui_PasswordWidget.h"

#include "core/Config.h"
#include "core/PasswordHealth.h"
#include "gui/Font.h"
#include "gui/Icons.h"
#include "gui/PasswordGeneratorWidget.h"
#include "gui/osutils/OSUtils.h"
#include "gui/styles/StateColorPalette.h"

#include <QAccessible>
#include <QApplication>
#include <QEvent>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>

PasswordWidget::PasswordWidget(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::PasswordWidget())
{
    m_ui->setupUi(this);
    setFocusProxy(m_ui->passwordEdit);
    m_ui->passwordEdit->installEventFilter(this);
    m_defaultAccessibleDescription = m_ui->passwordEdit->accessibleDescription();

    const QIcon errorIcon = icons()->icon("dialog-error");
    m_errorAction = m_ui->passwordEdit->addAction(errorIcon, QLineEdit::TrailingPosition);
    m_errorAction->setVisible(false);
    m_errorAction->setToolTip(tr("Passwords do not match"));

    const QIcon correctIcon = icons()->icon("dialog-ok");
    m_correctAction = m_ui->passwordEdit->addAction(correctIcon, QLineEdit::TrailingPosition);
    m_correctAction->setVisible(false);
    m_correctAction->setToolTip(tr("Passwords match so far"));

    setEchoMode(QLineEdit::Password);

    // Prevent conflicts with global Mac shortcuts (force Control on all platforms)
#ifdef Q_OS_MAC
    constexpr auto modifier = Qt::MetaModifier;
#else
    constexpr auto modifier = Qt::ControlModifier;
#endif

    m_toggleVisibleAction = new QAction(
        icons()->onOffIcon("password-show", false),
        tr("Toggle Password (%1)").arg(QKeySequence(modifier + Qt::Key_H).toString(QKeySequence::NativeText)),
        this);
    m_toggleVisibleAction->setCheckable(true);
    m_toggleVisibleAction->setShortcut(modifier + Qt::Key_H);
    m_toggleVisibleAction->setShortcutContext(Qt::WidgetShortcut);
    m_ui->passwordEdit->addAction(m_toggleVisibleAction, QLineEdit::TrailingPosition);
    connect(m_toggleVisibleAction, &QAction::triggered, this, &PasswordWidget::setShowPassword);

    m_passwordGeneratorAction = new QAction(
        icons()->icon("password-generator"),
        tr("Generate Password (%1)").arg(QKeySequence(modifier + Qt::Key_G).toString(QKeySequence::NativeText)),
        this);
    m_passwordGeneratorAction->setShortcut(modifier + Qt::Key_G);
    m_passwordGeneratorAction->setShortcutContext(Qt::WidgetShortcut);
    m_ui->passwordEdit->addAction(m_passwordGeneratorAction, QLineEdit::TrailingPosition);
    m_passwordGeneratorAction->setVisible(false);

    // QLineEdit::addAction() renders each action as an internal
    // QLineEditIconButton (a QToolButton parented to the line edit), but that
    // button defaults to Qt::NoFocus -- it responds to a mouse click but is
    // skipped entirely by Tab and never receives keyboard focus. Confirmed
    // live via UIA: every other checkbox in a dialog using this widget
    // reports "focusable"; this action's button doesn't. That leaves the
    // toggle/generate controls reachable only by already knowing their
    // Ctrl+H / Ctrl+G shortcuts, which a keyboard-only or screen-reader user
    // exploring the form by Tab has no way to discover. Qt::TabFocus (not
    // StrongFocus) is enough here since mouse click-to-activate already
    // works regardless of focus policy; this only adds Tab reachability.
    for (QAction* action : {m_toggleVisibleAction.data(), m_passwordGeneratorAction.data()}) {
        for (QToolButton* button : m_ui->passwordEdit->findChildren<QToolButton*>()) {
            if (button->defaultAction() == action) {
                button->setFocusPolicy(Qt::TabFocus);
                if (action == m_toggleVisibleAction) {
                    button->setAccessibleName(tr("Toggle password visibility"));
                    button->setAccessibleDescription(
                        tr("Shortcut: %1")
                            .arg(QKeySequence(action->shortcut()).toString(QKeySequence::NativeText)));
                } else if (action == m_passwordGeneratorAction) {
                    button->setAccessibleName(tr("Generate password"));
                    button->setAccessibleDescription(
                        tr("Shortcut: %1")
                            .arg(QKeySequence(action->shortcut()).toString(QKeySequence::NativeText)));
                }
                break;
            }
        }
    }

    m_capslockAction =
        new QAction(icons()->icon("dialog-warning", true, StateColorPalette().color(StateColorPalette::Error)),
                    tr("Warning: Caps Lock enabled!"),
                    this);
    m_ui->passwordEdit->addAction(m_capslockAction, QLineEdit::LeadingPosition);
    m_capslockAction->setVisible(false);

    // Reset the password strength bar, hidden by default
    updatePasswordStrength("");
    m_ui->qualityProgressBar->setVisible(false);

    connect(m_ui->passwordEdit, &QLineEdit::textChanged, this, [this](const QString& pwd) {
        QString resolvedPwd = pwd;

        // Making a request to dereference the password
        emit requestPlaceholderResolution(pwd, resolvedPwd);

        updatePasswordStrength(resolvedPwd);
        emit textChanged(pwd);
    });
}

PasswordWidget::~PasswordWidget()
{
}

void PasswordWidget::setAccessibleName(const QString& name)
{
    // Forward to the focus proxy: this is the widget a screen reader actually
    // reports when the user tabs into this control.
    m_ui->passwordEdit->setAccessibleName(name);
    QWidget::setAccessibleName(name);
}

QString PasswordWidget::accessibleName() const
{
    return m_ui->passwordEdit->accessibleName();
}

void PasswordWidget::setQualityVisible(bool state)
{
    m_ui->qualityProgressBar->setVisible(state);
}

QString PasswordWidget::text()
{
    return m_ui->passwordEdit->text();
}

void PasswordWidget::setText(const QString& text)
{
    m_ui->passwordEdit->setText(text);
}

void PasswordWidget::setEchoMode(QLineEdit::EchoMode mode)
{
    m_ui->passwordEdit->setEchoMode(mode);
}

void PasswordWidget::clear()
{
    m_ui->passwordEdit->clear();
}

void PasswordWidget::setClearButtonEnabled(bool enabled)
{
    m_ui->passwordEdit->setClearButtonEnabled(enabled);
}

void PasswordWidget::selectAll()
{
    m_ui->passwordEdit->selectAll();
}

void PasswordWidget::setReadOnly(bool state)
{
    m_ui->passwordEdit->setReadOnly(state);
}

void PasswordWidget::setRepeatPartner(PasswordWidget* repeatPartner)
{
    m_repeatPasswordWidget = repeatPartner;
    m_repeatPasswordWidget->setParentPasswordEdit(this);

    connect(m_ui->passwordEdit, SIGNAL(textChanged(QString)), m_repeatPasswordWidget, SLOT(updateRepeatStatus()));
}

void PasswordWidget::setParentPasswordEdit(PasswordWidget* parent)
{
    m_parentPasswordWidget = parent;

    // These actions are keyboard-focusable. If either internal QLineEdit action
    // button currently has focus, hiding it would remove the focused widget from
    // the focus chain. Return focus to the password editor before hiding them.
    if (auto* focusedWidget = QApplication::focusWidget();
        focusedWidget && m_ui->passwordEdit->isAncestorOf(focusedWidget)) {
        for (QToolButton* button : m_ui->passwordEdit->findChildren<QToolButton*>()) {
            if (button == focusedWidget
                && (button->defaultAction() == m_toggleVisibleAction
                    || button->defaultAction() == m_passwordGeneratorAction)) {
                m_ui->passwordEdit->setFocus(Qt::OtherFocusReason);
                break;
            }
        }
    }

    // Hide actions
    m_toggleVisibleAction->setVisible(false);
    m_passwordGeneratorAction->setVisible(false);
    connect(m_ui->passwordEdit, SIGNAL(textChanged(QString)), this, SLOT(updateRepeatStatus()));
}

void PasswordWidget::enablePasswordGenerator()
{
    if (!m_passwordGeneratorAction->isVisible()) {
        m_passwordGeneratorAction->setVisible(true);
        connect(m_passwordGeneratorAction, &QAction::triggered, this, &PasswordWidget::popupPasswordGenerator);
    }
}

void PasswordWidget::setShowPassword(bool show)
{
    setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    m_toggleVisibleAction->setIcon(icons()->onOffIcon("password-show", show));
    m_toggleVisibleAction->setChecked(show);

    if (m_repeatPasswordWidget) {
        m_repeatPasswordWidget->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    }
}

bool PasswordWidget::isPasswordVisible() const
{
    return m_ui->passwordEdit->echoMode() == QLineEdit::Normal;
}

void PasswordWidget::popupPasswordGenerator()
{
    auto generator = PasswordGeneratorWidget::popupGenerator(this);
    generator->setPasswordVisible(isPasswordVisible());
    generator->setPasswordLength(text().length());

    connect(generator, SIGNAL(appliedPassword(QString)), SLOT(setText(QString)));
    if (m_repeatPasswordWidget) {
        connect(generator, SIGNAL(appliedPassword(QString)), m_repeatPasswordWidget, SLOT(setText(QString)));
    }
}

void PasswordWidget::updateRepeatStatus()
{
    static const auto stylesheetTemplate = QStringLiteral("QLineEdit { background: %1; }");
    if (!m_parentPasswordWidget) {
        return;
    }

    const auto otherPassword = m_parentPasswordWidget->text();
    const auto password = text();
    QString statusText;
    if (otherPassword != password) {
        bool isCorrect = false;
        StateColorPalette statePalette;
        QColor color = statePalette.color(StateColorPalette::ColorRole::Error);
        if (!password.isEmpty() && otherPassword.startsWith(password)) {
            color = statePalette.color(StateColorPalette::ColorRole::Incomplete);
            isCorrect = true;
        }
        setStyleSheet(stylesheetTemplate.arg(color.name()));
        m_correctAction->setVisible(isCorrect);
        m_errorAction->setVisible(!isCorrect);
        statusText = isCorrect ? m_correctAction->toolTip() : m_errorAction->toolTip();
    } else {
        m_correctAction->setVisible(false);
        m_errorAction->setVisible(false);
        setStyleSheet("");
        statusText = password.isEmpty() ? QString() : tr("Passwords match");
    }

    // The match state above is otherwise conveyed only by background color and a
    // trailing icon (m_correctAction/m_errorAction) -- neither perceivable by a
    // screen reader user typing in this field. accessibleDescription is set from
    // the fixed strings above, never from password content, so this cannot leak
    // what was typed. Set it on the focus proxy (m_ui->passwordEdit), since that's
    // the object a screen reader actually reports on (see setAccessibleName()
    // above), and fire Alert the same way MessageWidget::showMessage() does so the
    // change is announced immediately without moving keyboard focus.
    if (statusText != m_ui->passwordEdit->accessibleDescription()) {
        m_ui->passwordEdit->setAccessibleDescription(statusText.isEmpty() ? m_defaultAccessibleDescription : statusText);
        if (!statusText.isEmpty()) {
            QAccessibleEvent alertEvent(m_ui->passwordEdit, QAccessible::Alert);
            QAccessible::updateAccessibility(&alertEvent);
            // Alert alone never reaches Windows UI Automation as an event
            // (confirmed against qtbase's qwindowsuiaaccessibility.cpp --
            // only a system sound). QAccessibleAnnouncementEvent (Qt 6.8+)
            // is what actually raises a UIA notification.
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            QAccessibleAnnouncementEvent announcementEvent(m_ui->passwordEdit, statusText);
            QAccessible::updateAccessibility(&announcementEvent);
#endif
        }
    }
}

bool PasswordWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_ui->passwordEdit) {
        auto type = event->type();
        if (isVisible() && (type == QEvent::KeyPress || type == QEvent::KeyRelease || type == QEvent::FocusIn)) {
            checkCapslockState();
        }
        if (type == QEvent::FocusIn || type == QEvent::FocusOut || type == QEvent::Hide) {
            osUtils->setUserInputProtection(type == QEvent::FocusIn);
        }
    }
    // Continue with normal operations
    return false;
}

void PasswordWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // use a monospace font for the password field
    QFont passwordFont = Font::fixedFont();
    passwordFont.setLetterSpacing(QFont::PercentageSpacing, 110);
    m_ui->passwordEdit->setFont(passwordFont);
}

void PasswordWidget::checkCapslockState()
{
    if (m_parentPasswordWidget) {
        return;
    }

    bool newCapslockState = osUtils->isCapslockEnabled();
    if (newCapslockState != m_capslockState) {
        m_capslockState = newCapslockState;
        m_capslockAction->setVisible(newCapslockState);

        // Force repaint to avoid rendering glitches of QLineEdit contents
        repaint();

        if (newCapslockState) {
            QTimer::singleShot(
                150, [this] { QToolTip::showText(mapToGlobal(rect().bottomLeft()), m_capslockAction->text()); });
            m_ui->passwordEdit->setAccessibleDescription(tr("Warning: Caps Lock enabled!"));
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            QAccessibleAnnouncementEvent announcementEvent(m_ui->passwordEdit, tr("Warning: Caps Lock enabled!"));
            QAccessible::updateAccessibility(&announcementEvent);
#endif
        } else {
            // The tooltip can disappear independently of the Caps Lock state.
            // Keep the screen-reader state synchronized even when there is no
            // tooltip left to hide.
            m_ui->passwordEdit->setAccessibleDescription(m_defaultAccessibleDescription);
            if (QToolTip::isVisible()) {
                QToolTip::hideText();
            }
        }
    }
}

void PasswordWidget::updatePasswordStrength(const QString& password)
{
    if (password.isEmpty()) {
        m_ui->qualityProgressBar->setValue(0);
        m_ui->qualityProgressBar->setToolTip((tr("")));
        m_ui->qualityProgressBar->setAccessibleDescription(tr("Password quality: Not available"));
        return;
    }

    PasswordHealth health(password);

    m_ui->qualityProgressBar->setValue(std::min(int(health.entropy()), m_ui->qualityProgressBar->maximum()));

    QString style = m_ui->qualityProgressBar->styleSheet();
    QRegularExpression re("(QProgressBar::chunk\\s*\\{.*?background-color:)[^;]+;",
                          QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    style.replace(re, "\\1 %1;");

    StateColorPalette qualityPalette;

    switch (health.quality()) {
    case PasswordHealth::Quality::Bad:
    case PasswordHealth::Quality::Poor:
        m_ui->qualityProgressBar->setStyleSheet(
            style.arg(qualityPalette.color(StateColorPalette::HealthCritical).name()));

        m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Poor", "Password quality")));

        break;

    case PasswordHealth::Quality::Weak:
        m_ui->qualityProgressBar->setStyleSheet(style.arg(qualityPalette.color(StateColorPalette::HealthBad).name()));

        m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Weak", "Password quality")));

        break;
    case PasswordHealth::Quality::Good:
        m_ui->qualityProgressBar->setStyleSheet(style.arg(qualityPalette.color(StateColorPalette::HealthOk).name()));

        m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Good", "Password quality")));

        break;
    case PasswordHealth::Quality::Excellent:

        m_ui->qualityProgressBar->setStyleSheet(
            style.arg(qualityPalette.color(StateColorPalette::HealthExcellent).name()));

        m_ui->qualityProgressBar->setToolTip(tr("Quality: %1").arg(tr("Excellent", "Password quality")));

        break;
    }

    QString qualityText;
    switch (health.quality()) {
    case PasswordHealth::Quality::Bad:
    case PasswordHealth::Quality::Poor:
        qualityText = tr("Poor", "Password quality");
        break;
    case PasswordHealth::Quality::Weak:
        qualityText = tr("Weak", "Password quality");
        break;
    case PasswordHealth::Quality::Good:
        qualityText = tr("Good", "Password quality");
        break;
    case PasswordHealth::Quality::Excellent:
        qualityText = tr("Excellent", "Password quality");
        break;
    }
    m_ui->qualityProgressBar->setAccessibleDescription(
        tr("Password quality: %1.").arg(qualityText));
}
