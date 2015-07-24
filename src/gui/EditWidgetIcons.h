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

#ifndef KEEPASSX_EDITWIDGETICONS_H
#define KEEPASSX_EDITWIDGETICONS_H

#include <QWidget>

#include "core/Uuid.h"

class Database;
class DefaultIconModel;
class CustomIconModel;

namespace Ui {
    class EditWidgetIcons;
}

struct IconStruct
{
    IconStruct();

    Uuid uuid;
    int number;
};

class EditWidgetIcons : public QWidget
{
    Q_OBJECT

public:
    explicit EditWidgetIcons(QWidget* parent = nullptr);
    ~EditWidgetIcons();

    IconStruct save();
    void load(Uuid currentUuid, Database* database, IconStruct iconStruct);

private Q_SLOTS:
    void addCustomIcon();
    void removeCustomIcon();
    void updateWidgetsDefaultIcons(bool checked);
    void updateWidgetsCustomIcons(bool checked);
    void updateRadioButtonDefaultIcons();
    void updateRadioButtonCustomIcons();

private:
    const QScopedPointer<Ui::EditWidgetIcons> m_ui;
    Database* m_database;
    Uuid m_currentUuid;
    DefaultIconModel* const m_defaultIconModel;
    CustomIconModel* const m_customIconModel;

    Q_DISABLE_COPY(EditWidgetIcons)
};

#endif // KEEPASSX_EDITWIDGETICONS_H
