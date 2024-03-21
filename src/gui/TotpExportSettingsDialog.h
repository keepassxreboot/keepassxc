/*
 *  Copyright (C) 2017 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_TotpExportSettingsDialog_H
#define KEEPASSX_TotpExportSettingsDialog_H

#include <QDialog>

#include "core/Database.h"
#include "gui/DatabaseWidget.h"

class QVBoxLayout;
class SquareSvgWidget;
class QLabel;
class QDialogButtonBox;

class TotpExportSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TotpExportSettingsDialog(DatabaseWidget* parent = nullptr, Entry* entry = nullptr);
    ~TotpExportSettingsDialog() override;

private slots:
    void copyToClipboard();
    void autoClose();

private:
    int m_secTillClose;
    QString m_totpUri;
    QTimer* m_timer;

    QVBoxLayout* m_verticalLayout;
    QStackedWidget* m_totpSvgContainerWidget;
    SquareSvgWidget* m_totpSvgWidget;
    QLabel* m_countDown;
    QLabel* m_warningLabel;
    QDialogButtonBox* m_buttonBox;
};

#endif // KEEPASSX_TOTPEXPORTSETTINGSDIALOG_H
