/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
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

#include "PasswordEdit.h"

#include "core/Config.h"
#include "gui/Font.h"

const QColor PasswordEdit::CorrectSoFarColor = QColor(255, 205, 15);
const QColor PasswordEdit::ErrorColor = QColor(255, 125, 125);

PasswordEdit::PasswordEdit(QWidget* parent)
    : QLineEdit(parent)
    , m_basePasswordEdit(nullptr)
{
    setEchoMode(QLineEdit::Password);
    updateStylesheet();

    // use a monospace font for the password field
    QFont passwordFont = Font::fixedFont();
    passwordFont.setLetterSpacing(QFont::PercentageSpacing, 110);
    setFont(passwordFont);
}

void PasswordEdit::enableVerifyMode(PasswordEdit* basePasswordEdit)
{
    m_basePasswordEdit = basePasswordEdit;

    updateStylesheet();

    connect(m_basePasswordEdit, SIGNAL(textChanged(QString)), SLOT(autocompletePassword(QString)));
    connect(m_basePasswordEdit, SIGNAL(textChanged(QString)), SLOT(updateStylesheet()));
    connect(this, SIGNAL(textChanged(QString)), SLOT(updateStylesheet()));

    connect(m_basePasswordEdit, SIGNAL(showPasswordChanged(bool)), SLOT(setShowPassword(bool)));
}

void PasswordEdit::setShowPassword(bool show)
{
    setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    // if I have a parent, I'm the child
    if (m_basePasswordEdit) {
        if (config()->get("security/passwordsrepeat").toBool()) {
            setEnabled(!show);
            setReadOnly(show);
            setText(m_basePasswordEdit->text());
        } else {
            // This fix a bug when the QLineEdit is disabled while switching config
            if (isEnabled() == false) {
                setEnabled(true);
                setReadOnly(false);
            }
        }
    }
    updateStylesheet();
    emit showPasswordChanged(show);
}

bool PasswordEdit::passwordsEqual() const
{
    return text() == m_basePasswordEdit->text();
}

void PasswordEdit::updateStylesheet()
{
    QString stylesheet("QLineEdit { ");

    if (m_basePasswordEdit && !passwordsEqual()) {
        stylesheet.append("background: %1; ");

        if (m_basePasswordEdit->text().startsWith(text())) {
            stylesheet = stylesheet.arg(CorrectSoFarColor.name());
        } else {
            stylesheet = stylesheet.arg(ErrorColor.name());
        }
    }

    stylesheet.append("}");
    setStyleSheet(stylesheet);
}

void PasswordEdit::autocompletePassword(QString password)
{
    if (config()->get("security/passwordsrepeat").toBool() && echoMode() == QLineEdit::Normal) {
        setText(password);
    }
}
