/*
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_REPORTSWIDGETHIBP_H
#define KEEPASSXC_REPORTSWIDGETHIBP_H

#include "core/HibpDownloader.h"
#include "gui/entry/EntryModel.h"

#include <QPair>
#include <QSet>
#include <QWidget>

class Database;
class Entry;
class Group;
class QStandardItemModel;

namespace Ui
{
    class ReportsWidgetHibp;
}

class ReportsWidgetHibp : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsWidgetHibp(QWidget* parent = nullptr);
    ~ReportsWidgetHibp();

    void loadSettings(QSharedPointer<Database> db);
    void saveSettings();
    void refreshAfterEdit();

protected:
    void showEvent(QShowEvent* event) override;

signals:
    void entryActivated(Entry*);

public slots:
    void emitEntryActivated(const QModelIndex&);
    void checkFinished(const QString&, int);
    void checkFailed(const QString&, const QString&);

private:
    void makeHibpTable();
    void startValidation();

    QScopedPointer<Ui::ReportsWidgetHibp> m_ui;
    QScopedPointer<QStandardItemModel> m_referencesModel;
    QSharedPointer<Database> m_db;

    bool m_checkStarted = false; // Have we run the initialization already?
    HibpDownloader m_downloader; // This performs the actual HIBP online query
    int m_qMax = 0; // Max number of items in the queue (for progress bar)
    QMap<QString, int> m_pwPwned; // Passwords we found to have been pwned (value is pwn count)
    QString m_error; // Error message if download failed, else empty
    QList<const Entry*> m_rowToEntry; // List index is table row
    const Entry* m_edEntry = nullptr; // The entry we're currently editing
    QString m_edPw; // The old password of the entry we're editing
};

#endif // KEEPASSXC_REPORTSWIDGETHIBP_H
