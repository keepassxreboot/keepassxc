/*
 *  Copyright (C) 2010 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_EDITENTRYWIDGET_H
#define KEEPASSX_EDITENTRYWIDGET_H

#include <QtCore/QScopedPointer>
#include <QtGui/QWidget>

class Entry;
class QListWidget;
class QStackedLayout;

namespace Ui {
    class EditEntryWidget;
    class EditEntryWidgetMain;
    class EditEntryWidgetNotes;
}

class EditEntryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditEntryWidget(QWidget* parent = 0);
    ~EditEntryWidget();

    void loadEntry(Entry* entry);

Q_SIGNALS:
    void editFinished();

private Q_SLOTS:
    void saveEntry();
    void cancel();

private:
    Entry* m_entry;

    QScopedPointer<Ui::EditEntryWidget> m_ui;
    QScopedPointer<Ui::EditEntryWidgetMain> m_mainUi;
    QScopedPointer<Ui::EditEntryWidgetNotes> m_notesUi;
    QWidget* m_mainWidget;
    QWidget* m_notesWidget;

    Q_DISABLE_COPY(EditEntryWidget)
};

#endif // KEEPASSX_EDITENTRYWIDGET_H
