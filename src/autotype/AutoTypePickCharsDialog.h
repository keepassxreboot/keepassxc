/*
 *  Copyright (C) 2017 Daniel Meek <https://github.com/danielmeek32>
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

#ifndef KEEPASSX_AUTOTYPEPICKCHARSDIALOG_H
#define KEEPASSX_AUTOTYPEPICKCHARSDIALOG_H

#include <QDialog>

#include "gui/PasswordEdit.h"

class AutoTypePickCharsDialog : public QDialog
{
    Q_OBJECT

public:
    AutoTypePickCharsDialog(QString string, QWidget* parent = nullptr);
    QString chars();

public slots:
    void done(int r) override;

protected slots:
    void reject() override;

private:
    QString m_string;
    PasswordEdit* m_passwordEdit;

private slots:
    void addChar(int index);
};

#endif // KEEPASSX_AUTOTYPEPICKCHARSDIALOG_H
