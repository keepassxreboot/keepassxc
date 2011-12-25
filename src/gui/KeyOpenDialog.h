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

#ifndef KEEPASSX_KEYOPENDIALOG_H
#define KEEPASSX_KEYOPENDIALOG_H

#include <QtCore/QScopedPointer>
#include <QtGui/QDialog>

#include "keys/CompositeKey.h"

namespace Ui {
    class KeyOpenDialog;
}

class KeyOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit KeyOpenDialog(const QString& filename, QWidget* parent = 0);
    ~KeyOpenDialog();
    CompositeKey key();

private Q_SLOTS:
    void createKey();
    void togglePassword(bool checked);
    void activatePassword();
    void activateKeyFile();
    void setOkButtonEnabled();
    void browseKeyFile();

private:
    QScopedPointer<Ui::KeyOpenDialog> m_ui;
    CompositeKey m_key;
    QString m_filename;

    Q_DISABLE_COPY(KeyOpenDialog)
};

#endif // KEEPASSX_KEYOPENDIALOG_H
