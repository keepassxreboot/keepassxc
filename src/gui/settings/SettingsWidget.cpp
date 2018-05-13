/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

SettingsWidget::SettingsWidget(QWidget* parent)
    : QWidget(parent)
{
}

SettingsWidget::~SettingsWidget()
{
}

/**
 * Switch between simple mode (the default) and advanced mode.
 * Subclasses which implement an advanced mode, need to override this method,
 * \link advancedMode() and \link hasAdvancedMode.
 *
 * When overriding this method, make sure to also emit the
 * \link advancedModeChanged() signal.
 *
 * @param advanced whether advanced mode is enabled
 */
void SettingsWidget::setAdvancedMode(bool advanced)
{
    if (hasAdvancedMode() && advanced != advancedMode()) {
        m_advancedMode = advanced;
        emit advancedModeChanged(advanced);
    }
}

/**
 * @return true if advanced mode is on (default: false)
 */
bool SettingsWidget::advancedMode() const
{
    return m_advancedMode;
}
