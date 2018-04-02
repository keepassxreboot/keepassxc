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

namespace Ui
{
    class DetailsWidget;
}

class DetailsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DetailsWidget(QWidget* parent = nullptr);
    ~DetailsWidget();

public slots:
    void setEntry(Entry* selectedEntry);
    void setGroup(Group* selectedGroup);
    void setDatabaseMode(DatabaseWidget::Mode mode);

signals:
    void errorOccurred(const QString& error);

private slots:
    void updateEntryHeaderLine();
    void updateEntryTotp();
    void updateEntryGeneralTab();
    void updateEntryNotesTab();
    void updateEntryAttributesTab();
    void updateEntryAttachmentsTab();
    void updateEntryAutotypeTab();

    void updateGroupHeaderLine();
    void updateGroupGeneralTab();
    void updateGroupNotesTab();

    void stopTotpTimer();
    void deleteTotpTimer();
    void updateTotpLabel();
    void updateTabIndexes();

private:
    void setTabEnabled(QTabWidget* tabWidget, QWidget* widget, bool enabled);

    static QPixmap preparePixmap(const QPixmap& pixmap, int size);
    static QString hierarchy(const Group* group, const QString& title);

    const QScopedPointer<Ui::DetailsWidget> m_ui;
    bool m_locked;
    Entry* m_currentEntry;
    Group* m_currentGroup;
    quint8 m_step;
    QPointer<QTimer> m_totpTimer;
    quint8 m_selectedTabEntry;
    quint8 m_selectedTabGroup;
};

#endif // KEEPASSX_DETAILSWIDGET_H
