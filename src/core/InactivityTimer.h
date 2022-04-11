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

#ifndef KEEPASSX_INACTIVITYTIMER_H
#define KEEPASSX_INACTIVITYTIMER_H

#include <QMutex>
#include <QObject>

class QTimer;

class InactivityTimer : public QObject
{
    Q_OBJECT

public:
    explicit InactivityTimer(QObject* parent = nullptr);
    void setInactivityTimeout(int inactivityTimeout);
    void activate();
    void deactivate();

signals:
    void inactivityDetected();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void timeout();

private:
    QTimer* m_timer;
    bool m_active;
    QMutex m_emitMutx;
};

#endif // KEEPASSX_INACTIVITYTIMER_H
