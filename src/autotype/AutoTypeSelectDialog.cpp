/*
 *  Copyright (C) 2012 Felix Geyer <debfx@fobos.de>
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

#include "AutoTypeSelectDialog.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>

#include "autotype/AutoTypeSelectView.h"
#include "core/AutoTypeMatch.h"
#include "core/Config.h"
#include "core/FilePath.h"
#include "gui/entry/AutoTypeMatchModel.h"

AutoTypeSelectDialog::AutoTypeSelectDialog(QWidget* parent)
    : QDialog(parent)
    , m_view(new AutoTypeSelectView(this))
    , m_matchActivatedEmitted(false)
    , m_rejected(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    // Places the window on the active (virtual) desktop instead of where the main window is.
    setAttribute(Qt::WA_X11BypassTransientForHint);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Auto-Type - KeePassXC"));
    setWindowIcon(filePath()->applicationIcon());

    QRect screenGeometry = QApplication::desktop()->availableGeometry(QCursor::pos());
    QSize size = config()->get("GUI/AutoTypeSelectDialogSize", QSize(600, 250)).toSize();
    size.setWidth(qMin(size.width(), screenGeometry.width()));
    size.setHeight(qMin(size.height(), screenGeometry.height()));
    resize(size);

    // move dialog to the center of the screen
    QPoint screenCenter = screenGeometry.center();
    move(screenCenter.x() - (size.width() / 2), screenCenter.y() - (size.height() / 2));

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* descriptionLabel = new QLabel(tr("Select entry to Auto-Type:"), this);
    layout->addWidget(descriptionLabel);

    connect(m_view, SIGNAL(activated(QModelIndex)), SLOT(emitMatchActivated(QModelIndex)));
    connect(m_view, SIGNAL(clicked(QModelIndex)), SLOT(emitMatchActivated(QModelIndex)));
    connect(m_view->model(), SIGNAL(rowsRemoved(QModelIndex, int, int)), SLOT(matchRemoved()));
    connect(m_view, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(m_view);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
    layout->addWidget(buttonBox);
}

void AutoTypeSelectDialog::setMatchList(const QList<AutoTypeMatch>& matchList)
{
    m_view->setMatchList(matchList);

    m_view->header()->resizeSections(QHeaderView::ResizeToContents);
}

void AutoTypeSelectDialog::done(int r)
{
    config()->set("GUI/AutoTypeSelectDialogSize", size());

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

    if (m_view->model()->rowCount() == 0) {
        reject();
    }
}
