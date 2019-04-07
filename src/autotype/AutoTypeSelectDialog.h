/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_AUTOTYPESELECTDIALOG_H
#define KEEPASSX_AUTOTYPESELECTDIALOG_H

#include <QAbstractItemModel>
#include <QDialog>
#include <QHash>

#include "autotype/AutoTypeFilterLineEdit.h"
#include "core/AutoTypeMatch.h"

class AutoTypeSelectView;

class AutoTypeSelectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AutoTypeSelectDialog(QWidget* parent = nullptr);
    void setMatchList(const QList<AutoTypeMatch>& matchList);

signals:
    void matchActivated(AutoTypeMatch match);

public slots:
    void done(int r) override;
    void reject() override;

private slots:
    void emitMatchActivated(const QModelIndex& index);
    void matchRemoved();
    void filterList(QString filterString);
    void moveSelectionUp();
    void moveSelectionDown();
    void activateCurrentIndex();

private:
    AutoTypeSelectView* const m_view;
    AutoTypeFilterLineEdit* const m_filterLineEdit;
    bool m_matchActivatedEmitted;
    bool m_rejected;
};

#endif // KEEPASSX_AUTOTYPESELECTDIALOG_H
