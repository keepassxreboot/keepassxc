/*
 *  Copyright (C) 2022 KeePassXC Team <team@keepassxc.org>
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

#include "TimeDeltaUI.h"

#include <QSpinBox>
#include <QComboBox>
#include <QtGlobal>

TimeDeltaUI::UnitTexts TimeDeltaUI::s_unitTexts = {
    { Unit::Hours, QT_TR_N_NOOP("hour(s)") },
    { Unit::Days, QT_TR_N_NOOP("day(s)") },
    { Unit::Weeks, QT_TR_N_NOOP("week(s)") },
    { Unit::Months, QT_TR_N_NOOP("month(s)") },
    { Unit::Years, QT_TR_N_NOOP("year(s)") }
};

TimeDeltaUI::TimeDeltaUI(QSpinBox* valueSpinBox, QComboBox* unitComboBox)
    : m_valueSpinBox(valueSpinBox)
    , m_unitComboBox(unitComboBox)
{
    connect(m_valueSpinBox, SIGNAL(valueChanged(int)), SLOT(valueChanged(int)));
}

void TimeDeltaUI::initialize(int value, Unit initialUnit)
{
    bool blocked = m_unitComboBox->blockSignals(true);
    m_unitComboBox->clear();
    for (const auto& elm : s_unitTexts) {
        auto unit = tr(elm.second, nullptr, value);
        m_unitComboBox->addItem(unit, elm.first);
    }
    auto idx = m_unitComboBox->findData(initialUnit);
    m_unitComboBox->setCurrentIndex(idx);
    m_unitComboBox->blockSignals(blocked);
    m_unitComboBox->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);

    m_valueSpinBox->setValue(value);
}

TimeDelta TimeDeltaUI::toTimeDelta()
{
    int value = m_valueSpinBox->value();
    auto unit = m_unitComboBox->currentData();

    if (Unit::Hours == unit) {
        return TimeDelta::fromHours(value);
    }

    if (Unit::Days == unit) {
        return TimeDelta::fromDays(value);
    }

    if (Unit::Weeks == unit) {
        return TimeDelta::fromDays(value * 7);
    }

    if (Unit::Months == unit) {
        return TimeDelta::fromMonths(value);
    }

    if (Unit::Years == unit) {
        return TimeDelta::fromYears(value);
    }

    Q_ASSERT(false);
    return TimeDelta::fromHours(0);
}

void TimeDeltaUI::valueChanged(int value) {
    // re-translate all units to fix the plural if the new value requires it
    for (const auto& elm : s_unitTexts) {
        auto idx = m_unitComboBox->findData(elm.first);
        Q_ASSERT(idx >= 0);
        auto unit = tr(elm.second, nullptr, value);
        m_unitComboBox->setItemText(idx, unit);
    }
}
