/*
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

#ifndef KEEPASSX_DETAILSWIDGET_H
#define KEEPASSX_DETAILSWIDGET_H

#include "gui/DatabaseWidget.h"
#include <QWidget>
#include <QTimer>

namespace Ui {
    class DetailsWidget;
}

class DetailsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsWidget(QWidget* parent = nullptr);
    ~DetailsWidget();

    enum StackedWidgetIndex
    {
        EntryPreview = 0,
        GroupPreview = 1,
    };

    enum TabWidgetIndex
    {
        GeneralTab = 0,
        AttributesTab = 1,
        GroupNotesTab = 1,
        AttachmentsTab = 2,
        NotesTab = 3,
        AutotypeTab = 4,
    };

signals:
    void errorOccurred(const QString& error);

private slots:
    void getSelectedEntry(Entry* selectedEntry);
    void getSelectedGroup(Group* selectedGroup);
    void showTotp(bool visible);
    void updateTotp();
    void hideDetails();
    void setDatabaseMode(DatabaseWidget::Mode mode);
    void updateTabIndex(int index);

private:
    const QScopedPointer<Ui::DetailsWidget> m_ui;
    bool m_locked;
    Entry* m_currentEntry;
    Group* m_currentGroup;
    quint8 m_step;
    QPointer<QTimer> m_timer = nullptr;
    QWidget* m_attributesTabWidget;
    QWidget* m_attachmentsTabWidget;
    QWidget* m_autotypeTabWidget;
    quint8 m_selectedTabEntry;
    quint8 m_selectedTabGroup;
    QString shortUrl(QString url);
    QString shortPassword(QString password);
};

#endif // KEEPASSX_DETAILSWIDGET_H
