/*
 *  Copyright (C) 2023 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_BROWSERPASSKEYSCONFIRMATIONDIALOG_H
#define KEEPASSXC_BROWSERPASSKEYSCONFIRMATIONDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QTimer>

class Entry;

namespace Ui
{
    class BrowserPasskeysConfirmationDialog;
}

class BrowserPasskeysConfirmationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BrowserPasskeysConfirmationDialog(QWidget* parent = nullptr);
    ~BrowserPasskeysConfirmationDialog() override;

    void registerCredential(const QString& username,
                            const QString& relyingParty,
                            const QList<Entry*>& existingEntries,
                            int timeout);
    void authenticateCredential(const QList<Entry*>& entries, const QString& relyingParty, int timeout);
    Entry* getSelectedEntry() const;
    bool isPasskeyUpdated() const;

private slots:
    void updatePasskey();
    void updateProgressBar();
    void updateSeconds();

private:
    void startCounter(int timeout);
    void updateTimeoutLabel();
    void updateEntriesToTable(const QList<Entry*>& entries);

private:
    QScopedPointer<Ui::BrowserPasskeysConfirmationDialog> m_ui;
    QList<Entry*> m_entries;
    QTimer m_timer;
    int m_counter;
    bool m_passkeyUpdated;
};

#endif // KEEPASSXC_BROWSERPASSKEYSCONFIRMATIONDIALOG_H
