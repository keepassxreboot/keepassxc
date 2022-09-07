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

#include "gui/DatabaseWidget.h"

class SignalMultiplexer;

namespace Ui
{
    class SearchWidget;
}

class PopupHelpWidget;

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);
    ~SearchWidget() override;

    Q_DISABLE_COPY(SearchWidget)

    void connectSignals(SignalMultiplexer& mx);
    void setCaseSensitive(bool state);
    void setLimitGroup(bool state);

protected:
    // Filter key presses in the search field
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void search(const QString& text);
    void searchCanceled();
    void caseSensitiveChanged(bool state);
    void limitGroupChanged(bool state);
    void escapePressed();
    void copyPressed();
    void downPressed();
    void enterPressed();
    void lostFocus();
    void saveSearch(const QString& text);

public slots:
    void databaseChanged(DatabaseWidget* dbWidget = nullptr);
    void focusSearch();
    void clearSearch();

private slots:
    void startSearchTimer();
    void startSearch();
    void updateCaseSensitive();
    void updateLimitGroup();
    void toggleHelp();
    void showSearchMenu();
    void resetSearchClearTimer();

private:
    const QScopedPointer<Ui::SearchWidget> m_ui;
    PopupHelpWidget* m_helpWidget;
    QTimer* m_searchTimer;
    QTimer* m_clearSearchTimer;
    QAction* m_actionCaseSensitive;
    QAction* m_actionLimitGroup;
    QMenu* m_searchMenu;
};

#endif // SEARCHWIDGET_H
