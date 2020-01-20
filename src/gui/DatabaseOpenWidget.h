/*
 *  Copyright (C) 2011 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_DATABASEOPENWIDGET_H
#define KEEPASSX_DATABASEOPENWIDGET_H

#include <QScopedPointer>

#include "gui/DialogyWidget.h"
#include "keys/CompositeKey.h"

class Database;
class QFile;

namespace Ui
{
    class DatabaseOpenWidget;
}

class DatabaseOpenWidget : public DialogyWidget
{
    Q_OBJECT

public:
    explicit DatabaseOpenWidget(QWidget* parent = nullptr);
    ~DatabaseOpenWidget();
    void load(const QString& filename);
    void clearForms();
    void enterKey(const QString& pw, const QString& keyFile);
    QSharedPointer<Database> database();

public slots:
    void pollYubikey();

signals:
    void dialogFinished(bool accepted);

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    QSharedPointer<CompositeKey> databaseKey();

protected slots:
    virtual void openDatabase();
    void reject();

private slots:
    void browseKeyFile();
    void clearKeyFileEdit();
    void handleKeyFileComboEdited();
    void handleKeyFileComboChanged();
    void yubikeyDetected(int slot, bool blocking);
    void yubikeyDetectComplete();
    void noYubikeyFound();
    void openHardwareKeyHelp();
    void openKeyFileHelp();

protected:
    const QScopedPointer<Ui::DatabaseOpenWidget> m_ui;
    QSharedPointer<Database> m_db;
    QString m_filename;
    bool m_retryUnlockWithEmptyPassword = false;

private:
    bool m_yubiKeyBeingPolled = false;
    bool m_keyFileComboEdited = false;
    bool m_isOpeningDatabase = false;
    Q_DISABLE_COPY(DatabaseOpenWidget)
};

#endif // KEEPASSX_DATABASEOPENWIDGET_H
