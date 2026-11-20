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

#ifndef KEEPASSXC_SHAREENTRYWHISPERDIALOG_H
#define KEEPASSXC_SHAREENTRYWHISPERDIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include <QUrl>

class Entry;
class QCheckBox;
class QLabel;
class QPushButton;
class WhisperClient;
namespace Ui
{
    class ShareEntryWhisperDialog;
}

class ShareEntryWhisperDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShareEntryWhisperDialog(const Entry* entry, QWidget* parent = nullptr);
    ~ShareEntryWhisperDialog() override;

    static bool confirmConsent(QWidget* parent);

private slots:
    void updateShareButtonState();
    void onShareClicked();
    void onShareSucceeded(const QUrl& shareLink);
    void onShareFailed(const QString& errorMessage);
    void onCopyClicked();

private:
    void setupHeader();
    void setupFields();
    void setupControls();
    void connectSignals();
    void setHeader(const QPixmap& icon, const QString& title, const QString& subtitle);
    static void setupField(QCheckBox* checkBox, QLabel* preview, const QString& value, const QString& shownValue);
    QList<QCheckBox*> fieldCheckBoxes() const;

    QScopedPointer<Ui::ShareEntryWhisperDialog> m_ui;
    const Entry* m_entry;
    WhisperClient* m_client;
    QPushButton* m_shareButton = nullptr;
    QUrl m_shareUrl;
};

#endif // KEEPASSXC_SHAREENTRYWHISPERDIALOG_H
