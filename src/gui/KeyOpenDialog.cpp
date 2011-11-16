/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#include "KeyOpenDialog.h"
#include "ui_KeyOpenDialog.h"

#include "keys/PasswordKey.h"

KeyOpenDialog::KeyOpenDialog(QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::KeyOpenDialog())
{
    m_ui->setupUi(this);

    connect(m_ui->buttonTogglePassword, SIGNAL(toggled(bool)), SLOT(togglePassword(bool)));
    connect(m_ui->editPassword, SIGNAL(textChanged(QString)), SLOT(activatePassword()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(createKey()));
}

KeyOpenDialog::~KeyOpenDialog()
{
}

CompositeKey KeyOpenDialog::key()
{
    return m_key;
}

void KeyOpenDialog::createKey()
{
    if (m_ui->checkPassword->isChecked()) {
        m_key.addKey(PasswordKey(m_ui->editPassword->text()));
    }

    if (m_ui->checkKeyFile->isChecked()) {
        // TODO read key file
    }
}

void KeyOpenDialog::togglePassword(bool checked)
{
    m_ui->editPassword->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
}

void KeyOpenDialog::activatePassword()
{
    m_ui->checkPassword->setChecked(true);
}
