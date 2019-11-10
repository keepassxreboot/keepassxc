/*
 *  Copyright (C) 2019 KeePassXC Team <team@keepassxc.org>
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

#include "KeyboardShortcutsDialog.h"
#include "ui_KeyboardShortcutsDialog.h"

static const QString keyboardShortcuts[27][2] = {
    {"New Database", "Ctrl + Shift + N"},
    {"Open Database", "Ctrl + O"},
    {"Save Database", "Ctrl + S"},
    {"Save Database As", "Ctrl + Shift + S"},
    {"Close Database", "Ctrl + W"},
    {"Lock Databases", "Ctrl + L"},
    {"Quit", "Ctrl + Q"},
    {"New Entry", "Ctrl + N"},
    {"Edit Entry", "Ctrl + E"},
    {"Delete Entry", "Ctrl + D"},
    {"Clone Entry", "Ctrl + K"},
    {"Show TOTP", "Ctrl + Shift + T"},
    {"Copy TOTP", "Ctrl + T"},
    {"Copy Username", "Ctrl + B"},
    {"Copy Password", "Ctrl + C"},
    {"Trigger AutoType", "Ctrl + Shift + V"},
    {"Open URL", "Ctrl + Shift + U"},
    {"Copy URL", "Ctrl + U"},
    {"Show Minimized", "Ctrl + M"},
    {"Hide Window", "Ctrl + Shift + M"},
    {"Select Next Database Tab", "Ctrl + Tab <i>OR</i> Ctrl + Page Down"},
    {"Select Previous Database Tab", "Ctrl + Shift + Tab <i>OR</i> Ctrl + Page Up"},
    {"Toggle Passwords Hidden", "Ctrl + Shift + C"},
    {"Toggle Usernames Hidden", "Ctrl + Shift + B"},
    {"Focus Search", "Ctrl + F"},
    {"Clear Search", "Esc"},
    {"Show Keyboard Shortcuts", "Ctrl + /"}
};

KeyboardShortcutsDialog::KeyboardShortcutsDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::KeyboardShortcutsDialog())
{
    int actionsCount;
    QLabel* leftLabel;
    QLabel* rightLabel;

    m_ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    actionsCount = sizeof(keyboardShortcuts) / sizeof(keyboardShortcuts[0]);

    for(int i = 0; i < actionsCount; i++) {
        leftLabel = new QLabel();
        rightLabel= new QLabel();

        leftLabel->setText(keyboardShortcuts[i][0]);
        rightLabel->setText(keyboardShortcuts[i][1]);

        m_ui->formLayout->addRow(leftLabel, rightLabel);
    }

    resize(minimumSize());

    connect(m_ui->closeButtonBox, SIGNAL(rejected()), SLOT(close()));
}

KeyboardShortcutsDialog::~KeyboardShortcutsDialog()
{
}
