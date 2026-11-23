/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_TPMPINDIALOG_H
#define KEEPASSXC_TPMPINDIALOG_H

#include <QDialog>

#include <botan/secmem.h>

class QCheckBox;
class QLineEdit;
class QLabel;

/**
 * Prompt for the PIN/password of an authValue-protected TPM2 key, with an
 * option to remember it until the database is locked or closed.
 */
class TpmPinDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @param keptForQuickUnlock the PIN is remembered for Quick Unlock anyway (setting), so the choice is fixed
     */
    explicit TpmPinDialog(const QString& keyName,
                          const QString& message,
                          bool keptForQuickUnlock = false,
                          QWidget* parent = nullptr);
    ~TpmPinDialog() override;

    Botan::secure_vector<char> pin() const;
    bool remember() const;

    /**
     * @return false if the user cancelled
     */
    static bool getPin(QWidget* parent,
                       const QString& keyName,
                       const QString& message,
                       Botan::secure_vector<char>& pin,
                       bool& remember,
                       bool keptForQuickUnlock = false);

private:
    QLabel* m_label;
    QLabel* m_message;
    QLineEdit* m_edit;
    QCheckBox* m_remember;
};

#endif // KEEPASSXC_TPMPINDIALOG_H
