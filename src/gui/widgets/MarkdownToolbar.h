/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#pragma once

#include <QKeySequence>
#include <QToolBar>
#include <functional>

class QTextEdit;

class MarkdownToolbar : public QToolBar
{
    Q_OBJECT
public:
    explicit MarkdownToolbar(QWidget* parent = nullptr);
    void setTarget(QTextEdit* edit);

private:
    void setupUi();
    void wrapSelection(const QString& prefix, const QString& suffix, bool lineWise = false);
    void wrapSelectionWithNumbering();
    void insertLink();
    void changeHeadingLevel(int delta, bool clearAll = false);

    QAction* createAction(const QString& iconName,
                          const QString& tooltip,
                          const QKeySequence& shortcut,
                          const std::function<void()>& callback);

private:
    QTextEdit* m_target = nullptr;

    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
};
