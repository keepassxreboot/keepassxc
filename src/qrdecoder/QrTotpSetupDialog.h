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

#ifndef KEEPASSXC_QRDECODER_QRTOTPSETUPDIALOG_H
#define KEEPASSXC_QRDECODER_QRTOTPSETUPDIALOG_H

#include <QDialog>

class Entry;
class QTabWidget;
class TotpSetupDialog;

namespace QrDecoder
{
    class QrTotpWidget;

    class QrTotpSetupDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit QrTotpSetupDialog(QWidget* parent = nullptr, Entry* entry = nullptr);

    signals:
        void totpUpdated();

    private:
        QTabWidget* m_tabWidget;
        TotpSetupDialog* m_totpSetupDialog;
        QrTotpWidget* m_qrTotpWidget;
    };
} // namespace QrDecoder

#endif // KEEPASSXC_QRDECODER_QRTOTPSETUPDIALOG_H