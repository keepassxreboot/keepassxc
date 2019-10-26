/*
 *  Copyright (C) 2015 David Wu <lightvector@gmail.com>
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

#ifndef KEEPASSX_AUTOTYPEMATCHVIEW_H
#define KEEPASSX_AUTOTYPEMATCHVIEW_H

#include <QTreeView>

#include "core/AutoTypeMatch.h"

#include "gui/entry/AutoTypeMatchModel.h"

class SortFilterHideProxyModel;

class AutoTypeMatchView : public QTreeView
{
    Q_OBJECT

public:
    explicit AutoTypeMatchView(QWidget* parent = nullptr);
    AutoTypeMatch currentMatch();
    void setCurrentMatch(const AutoTypeMatch& match);
    AutoTypeMatch matchFromIndex(const QModelIndex& index);
    void setMatchList(const QList<AutoTypeMatch>& matches);
    void setFirstMatchActive();

signals:
    void matchActivated(AutoTypeMatch match);
    void matchSelectionChanged();
    void matchTextCopied();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void emitMatchActivated(const QModelIndex& index);
    void userNameCopied();
    void passwordCopied();

private:
    AutoTypeMatchModel* const m_model;
    SortFilterHideProxyModel* const m_sortModel;
};

#endif // KEEPASSX_AUTOTYPEMATCHVIEW_H
