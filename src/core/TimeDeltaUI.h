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

#ifndef KEEPASSXC_TIMEDELTAUI_H
#define KEEPASSXC_TIMEDELTAUI_H

#include <QObject>
#include <map>
#include "TimeDelta.h"

class QSpinBox;
class QComboBox;

class TimeDeltaUI : public QObject
{
    Q_OBJECT

public:
    enum Unit { Hours, Days, Weeks, Months, Years };

    TimeDeltaUI() = delete;
    TimeDeltaUI(QSpinBox* valueSpinBox, QComboBox* unitComboBox);

    void initialize(int value, Unit initialUnit);
    TimeDelta toTimeDelta();

private slots:
    void valueChanged(int value);

private:
    using UnitTexts = std::map<Unit, const char*>;
    static UnitTexts s_unitTexts;

    QSpinBox* m_valueSpinBox;
    QComboBox* m_unitComboBox;
};

#endif // KEEPASSXC_TIMEDELTAUI_H
