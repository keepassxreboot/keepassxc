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

#include <QtCore/QModelIndex>
#include <QtCore/QScopedPointer>

#include "gui/EditWidget.h"

class AutoTypeAssociations;
class AutoTypeAssociationsModel;
class Database;
class EditWidgetIcons;
class Entry;
class EntryAttachments;
class EntryAttachmentsModel;
class EntryAttributes;
class EntryAttributesModel;
class EntryHistoryModel;
class QButtonGroup;
class QMenu;
class QSortFilterProxyModel;
class QStackedLayout;

namespace Ui {
    class EditEntryWidgetAdvanced;
    class EditEntryWidgetAutoType;
    class EditEntryWidgetMain;
    class EditEntryWidgetHistory;
    class EditEntryWidgetNotes;
    class EditWidget;
}

class EditEntryWidget : public EditWidget
{
    Q_OBJECT

public:
    explicit EditEntryWidget(QWidget* parent = Q_NULLPTR);
    ~EditEntryWidget();

    void loadEntry(Entry* entry, bool create, bool history, const QString& parentName,
                   Database* database);

    static const QColor CorrectSoFarColor;
    static const QColor ErrorColor;

    void createPresetsMenu(QMenu* expirePresetsMenu);
    QString entryTitle() const;
Q_SIGNALS:
    void editFinished(bool accepted);
    void historyEntryActivated(Entry* entry);

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
    void updateAutoTypeEnabled();
    void insertAutoTypeAssoc();
    void removeAutoTypeAssoc();
    void loadCurrentAssoc(const QModelIndex& current);
    void clearCurrentAssoc();
    void applyCurrentAssoc();
    void showHistoryEntry();
    void restoreHistoryEntry();
    void deleteHistoryEntry();
    void deleteAllHistoryEntries();
    void emitHistoryEntryActivated(const QModelIndex& index);
    void histEntryActivated(const QModelIndex& index);
    void updateHistoryButtons(const QModelIndex& current, const QModelIndex& previous);
    void useExpiryPreset(QAction* action);

private:
    bool passwordsEqual();
    void setForms(const Entry* entry, bool restore = false);
    QMenu* createPresetsMenu();

    Entry* m_entry;
    Database* m_database;

    bool m_create;
    bool m_history;
    const QScopedPointer<Ui::EditEntryWidgetMain> m_mainUi;
    const QScopedPointer<Ui::EditEntryWidgetNotes> m_notesUi;
    const QScopedPointer<Ui::EditEntryWidgetAdvanced> m_advancedUi;
    const QScopedPointer<Ui::EditEntryWidgetAutoType> m_autoTypeUi;
    const QScopedPointer<Ui::EditEntryWidgetHistory> m_historyUi;
    QWidget* const m_mainWidget;
    QWidget* const m_notesWidget;
    QWidget* const m_advancedWidget;
    EditWidgetIcons* const m_iconsWidget;
    QWidget* const m_autoTypeWidget;
    QWidget* const m_historyWidget;
    EntryAttachments* const m_entryAttachments;
    EntryAttachmentsModel* const m_attachmentsModel;
    EntryAttributes* const m_entryAttributes;
    EntryAttributesModel* const m_attributesModel;
    EntryHistoryModel* const m_historyModel;
    QSortFilterProxyModel* const m_sortModel;
    QPersistentModelIndex m_currentAttribute;
    AutoTypeAssociations* const m_autoTypeAssoc;
    AutoTypeAssociationsModel* const m_autoTypeAssocModel;
    QButtonGroup* const m_autoTypeDefaultSequenceGroup;
    QButtonGroup* const m_autoTypeWindowSequenceGroup;
    QMenu* m_expirePresetsMenu;

    Q_DISABLE_COPY(EditEntryWidget)
};

#endif // KEEPASSX_EDITENTRYWIDGET_H
