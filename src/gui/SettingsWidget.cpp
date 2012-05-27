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

#include "SettingsWidget.h"
#include "ui_SettingsWidgetSecurity.h"

SettingsWidget::SettingsWidget(QWidget* parent)
    : EditWidget(parent)
    , m_secWidget(new QWidget())
    , m_secUi(new Ui::SettingsWidgetSecurity())
{
    setHeadline(tr("Application Settings"));

    m_secUi->setupUi(m_secWidget);
    add(tr("Security"), m_secWidget);

    connect(this, SIGNAL(accepted()), SLOT(saveSettings()));

    connect(m_secUi->clearClipboardCheckBox, SIGNAL(toggled(bool)),
            m_secUi->clearClipboardSpinBox, SLOT(setEnabled(bool)));
}

SettingsWidget::~SettingsWidget()
{
}

void SettingsWidget::loadSettings()
{
    setCurrentRow(0);
}

void SettingsWidget::saveSettings()
{
}
