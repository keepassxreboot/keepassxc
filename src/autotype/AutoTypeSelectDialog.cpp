/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2020 KeePassXC Team <team@keepassxc.org>
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

#include "AutoTypeSelectDialog.h"

#include <QApplication>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QScreen>
#else
#include <QDesktopWidget>
#endif
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

#include "autotype/AutoTypeSelectView.h"
#include "core/AutoTypeMatch.h"
#include "core/Config.h"
#include "gui/Icons.h"
#include "gui/entry/AutoTypeMatchModel.h"

AutoTypeSelectDialog::AutoTypeSelectDialog(QWidget* parent)
    : QDialog(parent)
    , m_view(new AutoTypeSelectView(this))
    , m_filterLineEdit(new AutoTypeFilterLineEdit(this))
    , m_matchActivatedEmitted(false)
    , m_rejected(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    // Places the window on the active (virtual) desktop instead of where the main window is.
    setAttribute(Qt::WA_X11BypassTransientForHint);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Auto-Type - KeePassXC"));
    setWindowIcon(icons()->applicationIcon());

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QRect screenGeometry = QApplication::screenAt(QCursor::pos())->availableGeometry();
#else
    QRect screenGeometry = QApplication::desktop()->availableGeometry(QCursor::pos());
#endif
    QSize size = config()->get(Config::GUI_AutoTypeSelectDialogSize).toSize();
    size.setWidth(qMin(size.width(), screenGeometry.width()));
    size.setHeight(qMin(size.height(), screenGeometry.height()));
    resize(size);

    // move dialog to the center of the screen
    QPoint screenCenter = screenGeometry.center();
    move(screenCenter.x() - (size.width() / 2), screenCenter.y() - (size.height() / 2));

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* descriptionLabel = new QLabel(tr("Select entry to Auto-Type:"), this);
    layout->addWidget(descriptionLabel);

    // clang-format off
    connect(m_view, SIGNAL(activated(QModelIndex)), SLOT(emitMatchActivated(QModelIndex)));
    connect(m_view, SIGNAL(clicked(QModelIndex)), SLOT(emitMatchActivated(QModelIndex)));
    connect(m_view->model(), SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(matchRemoved()));
    connect(m_view, SIGNAL(rejected()), SLOT(reject()));
    connect(m_view, SIGNAL(matchTextCopied()), SLOT(reject()));
    // clang-format on

    QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(m_view->model());
    if (proxy) {
        proxy->setFilterKeyColumn(-1);
        proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    layout->addWidget(m_view);

    connect(m_filterLineEdit, SIGNAL(textChanged(QString)), SLOT(filterList(QString)));
    connect(m_filterLineEdit, SIGNAL(returnPressed()), SLOT(activateCurrentIndex()));
    connect(m_filterLineEdit, SIGNAL(keyUpPressed()), SLOT(moveSelectionUp()));
    connect(m_filterLineEdit, SIGNAL(keyDownPressed()), SLOT(moveSelectionDown()));
    connect(m_filterLineEdit, SIGNAL(escapeReleased()), SLOT(reject()));

    m_filterLineEdit->setPlaceholderText(tr("Search..."));
    layout->addWidget(m_filterLineEdit);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttonBox);

    m_filterLineEdit->setFocus();
}

void AutoTypeSelectDialog::setMatchList(const QList<AutoTypeMatch>& matchList)
{
    m_view->setMatchList(matchList);

    m_view->header()->resizeSections(QHeaderView::ResizeToContents);
}

void AutoTypeSelectDialog::done(int r)
{
    config()->set(Config::GUI_AutoTypeSelectDialogSize, size());

    QDialog::done(r);
}

void AutoTypeSelectDialog::reject()
{
    m_rejected = true;

    QDialog::reject();
}

void AutoTypeSelectDialog::emitMatchActivated(const QModelIndex& index)
{
    // make sure we don't emit the signal twice when both activated() and clicked() are triggered
    if (m_matchActivatedEmitted) {
        return;
    }
    m_matchActivatedEmitted = true;

    AutoTypeMatch match = m_view->matchFromIndex(index);
    accept();
    emit matchActivated(match);
}

void AutoTypeSelectDialog::matchRemoved()
{
    if (m_rejected) {
        return;
    }

    if (m_view->model()->rowCount() == 0 && m_filterLineEdit->text().isEmpty()) {
        reject();
    }
}

void AutoTypeSelectDialog::filterList(QString filterString)
{
    QSortFilterProxyModel* proxy = qobject_cast<QSortFilterProxyModel*>(m_view->model());
    if (proxy) {
        proxy->setFilterWildcard(filterString);
        if (!m_view->currentIndex().isValid()) {
            m_view->setCurrentIndex(m_view->model()->index(0, 0));
        }
    }
}

void AutoTypeSelectDialog::moveSelectionUp()
{
    auto current = m_view->currentIndex();
    auto previous = current.sibling(current.row() - 1, 0);

    if (previous.isValid()) {
        m_view->setCurrentIndex(previous);
    }
}

void AutoTypeSelectDialog::moveSelectionDown()
{
    auto current = m_view->currentIndex();
    auto next = current.sibling(current.row() + 1, 0);

    if (next.isValid()) {
        m_view->setCurrentIndex(next);
    }
}

void AutoTypeSelectDialog::activateCurrentIndex()
{
    emitMatchActivated(m_view->currentIndex());
}
