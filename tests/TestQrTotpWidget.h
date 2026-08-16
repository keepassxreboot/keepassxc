/*
 *  Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TESTQRTOTPWIDGET_H
#define KEEPASSXC_TESTQRTOTPWIDGET_H

#include <QObject>
#include <QString>

class TestQrTotpWidget : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testInitialState();
    void testApplyValidUri();
    void testApplyInvalidUri();
    void testPasteValidUri();
    void testPasteInvalidText();
    void testKeyboardPasteValidUri();

private:
    QString m_clipboardText;
};

#endif // KEEPASSXC_TESTQRTOTPWIDGET_H
