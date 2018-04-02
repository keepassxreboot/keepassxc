/*
 *  Copyright (C) 2016 Jonathan White <support@dmapps.us>
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

#ifndef KEEPASSX_SEARCHWIDGET_H
#define KEEPASSX_SEARCHWIDGET_H

#include <QTimer>
#include <QWidget>

#include "core/SignalMultiplexer.h"
#include "gui/DatabaseWidget.h"

namespace Ui
{
    class SearchWidget;
}

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = 0);
    ~SearchWidget();

    void connectSignals(SignalMultiplexer& mx);
    void setCaseSensitive(bool state);
    void setLimitGroup(bool state);

protected:
    bool eventFilter(QObject* obj, QEvent* event);

signals:
    void search(const QString& text);
    void caseSensitiveChanged(bool state);
    void limitGroupChanged(bool state);
    void escapePressed();
    void copyPressed();
    void downPressed();
    void enterPressed();

public slots:
    void databaseChanged(DatabaseWidget* dbWidget = 0);

private slots:
    void startSearchTimer();
    void startSearch();
    void updateCaseSensitive();
    void updateLimitGroup();
    void searchFocus();

private:
    const QScopedPointer<Ui::SearchWidget> m_ui;
    QTimer* m_searchTimer;
    QAction* m_actionCaseSensitive;
    QAction* m_actionLimitGroup;

    Q_DISABLE_COPY(SearchWidget)
};

#endif // SEARCHWIDGET_H
