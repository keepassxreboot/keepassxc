/*
 *  Copyright (C) 2026 Aetf <aetf@unlimitedcodeworks.xyz>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "NoteLabel.h"
#include "ui_NoteLabel.h"

#include "gui/Icons.h"

#include <QStyle>

NoteLabel::NoteLabel(QWidget* parent)
    : QWidget(parent)
    , m_ui(new Ui::NoteLabel())
{
    // a note is one line, so the layout centers icon and text on each other
    m_ui->setupUi(this);
    updateIcon();
}

NoteLabel::~NoteLabel() = default;

QString NoteLabel::text() const
{
    return m_ui->textLabel->text();
}

void NoteLabel::setText(const QString& text)
{
    m_ui->textLabel->setText(text);
}

bool NoteLabel::isWarning() const
{
    return m_warning;
}

void NoteLabel::setWarning(bool warning)
{
    m_warning = warning;
    updateIcon();
}

void NoteLabel::updateIcon()
{
    const auto size = style()->pixelMetric(QStyle::PM_SmallIconSize, nullptr, this);
    const auto name = m_warning ? QStringLiteral("dialog-warning") : QStringLiteral("dialog-information");
    m_ui->iconLabel->setPixmap(icons()->icon(name).pixmap(size));
}
