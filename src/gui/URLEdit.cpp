/*
 *  Copyright (C) 2014 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "URLEdit.h"

#include <QRegularExpression>

#include "core/Config.h"
#include "core/FilePath.h"
#include "core/Tools.h"
#include "gui/Font.h"

const QColor URLEdit::ErrorColor = QColor(255, 125, 125);

URLEdit::URLEdit(QWidget* parent)
    : QLineEdit(parent)
{
    const QIcon errorIcon = filePath()->icon("status", "dialog-error");
    m_errorAction = addAction(errorIcon, QLineEdit::TrailingPosition);
    m_errorAction->setVisible(false);
    m_errorAction->setToolTip(tr("Invalid URL"));

    updateStylesheet();
}

void URLEdit::enableVerifyMode()
{
    updateStylesheet();

    connect(this, SIGNAL(textChanged(QString)), SLOT(updateStylesheet()));
}

void URLEdit::updateStylesheet()
{
    const QString stylesheetTemplate("QLineEdit { background: %1; }");

    if (!Tools::checkUrlValid(text())) {
        setStyleSheet(stylesheetTemplate.arg(ErrorColor.name()));
        m_errorAction->setVisible(true);
    } else {
        m_errorAction->setVisible(false);
        setStyleSheet("");
    }
}
