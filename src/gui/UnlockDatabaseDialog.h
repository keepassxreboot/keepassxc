/*
 *  Copyright (C) 2016 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_AUTOTYPEUNLOCKDIALOG_H
#define KEEPASSX_AUTOTYPEUNLOCKDIALOG_H

#include <QDialog>

//#include <gui/DatabaseTabWidget.h>

#include "core/Global.h"

class UnlockDatabaseWidget;
class Database;

class UnlockDatabaseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit UnlockDatabaseDialog(QWidget* parent = nullptr);
    void setFilePath(const QString& filePath);
    void clearForms();
    Database* database();

signals:
    void unlockDone(bool);

public slots:
    void complete(bool r);

private:
    UnlockDatabaseWidget* const m_view;
};

#endif // KEEPASSX_AUTOTYPEUNLOCKDIALOG_H
