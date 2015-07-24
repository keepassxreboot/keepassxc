/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
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

const QColor PasswordEdit::CorrectSoFarColor = QColor(255, 205, 15);
const QColor PasswordEdit::ErrorColor = QColor(255, 125, 125);

PasswordEdit::PasswordEdit(QWidget* parent)
    : QLineEdit(parent)
    , m_basePasswordEdit(nullptr)
{
}

void PasswordEdit::enableVerifyMode(PasswordEdit* basePasswordEdit)
{
    m_basePasswordEdit = basePasswordEdit;

    updateStylesheet();
    connect(m_basePasswordEdit, SIGNAL(textChanged(QString)), SLOT(updateStylesheet()));
    connect(this, SIGNAL(textChanged(QString)), SLOT(updateStylesheet()));

    connect(m_basePasswordEdit, SIGNAL(showPasswordChanged(bool)), SLOT(setShowPassword(bool)));
}

void PasswordEdit::setShowPassword(bool show)
{
    setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    updateStylesheet();
    Q_EMIT showPasswordChanged(show);
}

bool PasswordEdit::passwordsEqual() const
{
    return text() == m_basePasswordEdit->text();
}

void PasswordEdit::updateStylesheet()
{
    QString stylesheet("QLineEdit { ");

    if (echoMode() == QLineEdit::Normal) {
#ifdef Q_OS_MAC
        // Qt on Mac OS doesn't seem to know the generic monospace family (tested with 4.8.6)
        stylesheet.append("font-family: monospace,Menlo,Monaco; ");
#else
        stylesheet.append("font-family: monospace; ");
#endif
    }

    if (m_basePasswordEdit && !passwordsEqual()) {
        stylesheet.append("background: %1; ");

        if (m_basePasswordEdit->text().startsWith(text())) {
            stylesheet = stylesheet.arg(CorrectSoFarColor.name());
        }
        else {
            stylesheet = stylesheet.arg(ErrorColor.name());
        }
    }

    stylesheet.append("}");
    setStyleSheet(stylesheet);
}
