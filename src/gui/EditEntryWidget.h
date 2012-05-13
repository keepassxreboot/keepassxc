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

#include <QtCore/QAbstractItemModel>
#include <QtCore/QScopedPointer>

#include "gui/DialogyWidget.h"

class Database;
class Entry;
class EntryAttachments;
class EntryAttachmentsModel;
class EntryAttributes;
class EntryAttributesModel;
class DefaultIconModel;
class CustomIconModel;
class Metadata;
class QStackedLayout;

namespace Ui {
    class EditEntryWidget;
    class EditEntryWidgetAdvanced;
    class EditEntryWidgetMain;
    class EditEntryWidgetNotes;
    class EditEntryWidgetIcons;
}

class EditEntryWidget : public DialogyWidget
{
    Q_OBJECT

public:
    explicit EditEntryWidget(QWidget* parent = 0);
    ~EditEntryWidget();

    void loadEntry(Entry* entry, bool create, const QString& groupName, Database* database);

    static const QColor normalColor;
    static const QColor correctSoFarColor;
    static const QColor errorColor;

Q_SIGNALS:
    void editFinished(bool accepted);

private Q_SLOTS:
    void saveEntry();
    void cancel();
    void togglePassword(bool checked);
    void setPasswordCheckColors();
    void insertAttribute();
    void editCurrentAttribute();
    void removeCurrentAttribute();
    void updateCurrentAttribute();
    void insertAttachment();
    void saveCurrentAttachment();
    void removeCurrentAttachment();
    void addCustomIcon();
    void removeCustomIcon();
    void updateWidgetsDefaultIcons(bool checked);
    void updateWidgetsCustomIcons(bool checked);
    void updateRadioButtonDefaultIcons();
    void updateRadioButtonCustomIcons();

private:
    bool passwordsEqual();

    Entry* m_entry;
    Database* m_database;

    bool m_create;
    const QScopedPointer<Ui::EditEntryWidget> m_ui;
    const QScopedPointer<Ui::EditEntryWidgetMain> m_mainUi;
    const QScopedPointer<Ui::EditEntryWidgetNotes> m_notesUi;
    const QScopedPointer<Ui::EditEntryWidgetAdvanced> m_advancedUi;
    const QScopedPointer<Ui::EditEntryWidgetIcons> m_iconsUi;
    QWidget* const m_mainWidget;
    QWidget* const m_notesWidget;
    QWidget* const m_advancedWidget;
    QWidget* const m_iconsWidget;
    EntryAttachmentsModel* m_attachmentsModel;
    EntryAttributesModel* m_attributesModel;
    EntryAttachments* m_entryAttachments;
    EntryAttributes* m_entryAttributes;
    QPersistentModelIndex m_currentAttribute;
    DefaultIconModel* m_defaultIconModel;
    CustomIconModel* m_customIconModel;

    Q_DISABLE_COPY(EditEntryWidget)
};

#endif // KEEPASSX_EDITENTRYWIDGET_H
