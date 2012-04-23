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

#ifndef KEEPASSX_DATABASEOPENDIALOG_H
#define KEEPASSX_DATABASEOPENDIALOG_H

#include <QtCore/QScopedPointer>
#include <QtGui/QDialog>

#include "keys/CompositeKey.h"

class Database;
class QFile;

namespace Ui {
    class DatabaseOpenDialog;
}

class DatabaseOpenDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DatabaseOpenDialog(QFile* file, QString filename, QWidget* parent = 0);
    ~DatabaseOpenDialog();
    Database* database();

private Q_SLOTS:
    void openDatabase();
    void togglePassword(bool checked);
    void activatePassword();
    void activateKeyFile();
    void setOkButtonEnabled();
    void browseKeyFile();

private:
    const QScopedPointer<Ui::DatabaseOpenDialog> m_ui;
    Database* m_db;
    QFile* const m_file;
    const QString m_filename;

    Q_DISABLE_COPY(DatabaseOpenDialog)
};

#endif // KEEPASSX_DATABASEOPENDIALOG_H
