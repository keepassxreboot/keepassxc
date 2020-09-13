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

#ifndef KEEPASSX_EDITGROUPWIDGET_H
#define KEEPASSX_EDITGROUPWIDGET_H

#include <QComboBox>
#include <QScopedPointer>
#include <QScrollArea>

#include "core/Group.h"
#include "gui/EditWidget.h"

class CustomData;
class EditWidgetIcons;
class EditWidgetProperties;

namespace Ui
{
    class EditGroupWidgetMain;
    class EditWidget;
} // namespace Ui

class IEditGroupPage
{
public:
    virtual ~IEditGroupPage()
    {
    }
    virtual QString name() = 0;
    virtual QIcon icon() = 0;
    virtual QWidget* createWidget() = 0;
    virtual void set(QWidget* widget, Group* tempoaryGroup, QSharedPointer<Database> database) = 0;
    virtual void assign(QWidget* widget) = 0;
};

class EditGroupWidget : public EditWidget
{
    Q_OBJECT

public:
    explicit EditGroupWidget(QWidget* parent = nullptr);
    ~EditGroupWidget();

    void loadGroup(Group* group, bool create, const QSharedPointer<Database>& database);
    void clear();

    void addEditPage(IEditGroupPage* page);

signals:
    void editFinished(bool accepted);
    void messageEditEntry(QString, MessageWidget::MessageType);
    void messageEditEntryDismiss();

private slots:
    void apply();
    void save();
    void cancel();

private:
    void addTriStateItems(QComboBox* comboBox, bool inheritValue);
    int indexFromTriState(Group::TriState triState);
    Group::TriState triStateFromIndex(int index);
    void setupModifiedTracking();

    const QScopedPointer<Ui::EditGroupWidgetMain> m_mainUi;

    QPointer<QScrollArea> m_editGroupWidgetMain;
    QPointer<EditWidgetIcons> m_editGroupWidgetIcons;
    QPointer<EditWidgetProperties> m_editWidgetProperties;

    QScopedPointer<Group> m_temporaryGroup;
    QPointer<Group> m_group;
    QSharedPointer<Database> m_db;

    class ExtraPage;
    QList<ExtraPage> m_extraPages;

    Q_DISABLE_COPY(EditGroupWidget)
};

#endif // KEEPASSX_EDITGROUPWIDGET_H
