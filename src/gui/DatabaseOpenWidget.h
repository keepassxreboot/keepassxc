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

#ifndef KEEPASSX_DATABASEOPENWIDGET_H
#define KEEPASSX_DATABASEOPENWIDGET_H

#include <QScopedPointer>

#include "gui/DialogyWidget.h"
#include "keys/CompositeKey.h"

class Database;
class QFile;

namespace Ui {
    class DatabaseOpenWidget;
}

class DatabaseOpenWidget : public DialogyWidget
{
    Q_OBJECT

public:
    explicit DatabaseOpenWidget(QWidget* parent = nullptr);
    ~DatabaseOpenWidget();
    void load(const QString& filename);
    void enterKey(const QString& pw, const QString& keyFile);
    Database* database();

Q_SIGNALS:
    void editFinished(bool accepted);

protected:
    CompositeKey databaseKey();

protected Q_SLOTS:
    virtual void openDatabase();
    void reject();

private Q_SLOTS:
    void activatePassword();
    void activateKeyFile();
    void browseKeyFile();

protected:
    const QScopedPointer<Ui::DatabaseOpenWidget> m_ui;
    Database* m_db;
    QString m_filename;

private:
    Q_DISABLE_COPY(DatabaseOpenWidget)
};

#endif // KEEPASSX_DATABASEOPENWIDGET_H
