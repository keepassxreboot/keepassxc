/*
 *  Copyright (C) 2021 Team KeePassXC <team@keepassxc.org>
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

#include "PickcharsDialog.h"
#include "ui_PickcharsDialog.h"

#include "gui/Icons.h"

#include <QPushButton>
#include <QShortcut>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#include <QScreen>
#else
#include <QDesktopWidget>
#endif

PickcharsDialog::PickcharsDialog(const QString& string, QWidget* parent)
    : QDialog(parent)
    , m_ui(new Ui::PickcharsDialog())
{
    if (string.isEmpty()) {
        reject();
    }

    // Places the window on the active (virtual) desktop instead of where the main window is.
    setAttribute(Qt::WA_X11BypassTransientForHint);
    setWindowFlags((windowFlags() | Qt::WindowStaysOnTopHint | Qt::MSWindowsFixedSizeDialogHint)
                   & ~Qt::WindowContextHelpButtonHint);
    setWindowIcon(icons()->applicationIcon());

    m_ui->setupUi(this);

    // Increase max columns with longer passwords for better display
    int width = 10;
    if (string.length() >= 100) {
        width = 20;
    } else if (string.length() >= 60) {
        width = 15;
    }

    int count = 0;
    for (const auto& ch : string) {
        auto btn = new QPushButton(QString::number(count + 1));
        btn->setProperty("char", ch);
        btn->setProperty("count", count);
        connect(btn, &QPushButton::clicked, this, &PickcharsDialog::charSelected);
        m_ui->charsGrid->addWidget(btn, count / width, count % width);
        m_lastSelected = count;
        ++count;
    }
    // Prevent stretched buttons
    if (m_ui->charsGrid->rowCount() == 1 && m_ui->charsGrid->columnCount() < 5) {
        m_ui->charsGrid->addItem(new QSpacerItem(5, 5, QSizePolicy::MinimumExpanding), count / width, count % width);
    }
    m_ui->charsGrid->itemAtPosition(0, 0)->widget()->setFocus();

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Navigate grid layout using up/down/left/right motion
    new QShortcut(Qt::Key_Up, this, SLOT(upPressed()));
    new QShortcut(Qt::Key_Down, this, SLOT(downPressed()));
    // Remove last selected character
    auto shortcut = new QShortcut(Qt::Key_Backspace, this);
    connect(shortcut, &QShortcut::activated, this, [this] {
        auto text = m_ui->selectedChars->text();
        m_ui->selectedChars->setText(text.left(text.size() - 1));
    });
    // Submit the form
    shortcut = new QShortcut(Qt::CTRL + Qt::Key_S, this);
    connect(shortcut, &QShortcut::activated, this, [this] { accept(); });
}

void PickcharsDialog::upPressed()
{
    auto focus = focusWidget();
    if (!focus) {
        return;
    }

    auto count = focus->property("count");
    if (count.isValid()) {
        // Lower bound not checked by QGridLayout::itemAt https://bugreports.qt.io/browse/QTBUG-91261
        auto upCount = count.toInt() - m_ui->charsGrid->columnCount();
        if (upCount >= 0) {
            m_ui->charsGrid->itemAt(upCount)->widget()->setFocus();
        }
    } else if (focus == m_ui->selectedChars) {
        // Move back to the last selected button
        auto item = m_ui->charsGrid->itemAt(m_lastSelected);
        if (item) {
            item->widget()->setFocus();
        }
    } else if (focus == m_ui->pressTab) {
        m_ui->selectedChars->setFocus();
    }
}

void PickcharsDialog::downPressed()
{
    auto focus = focusWidget();
    if (!focus) {
        return;
    }

    auto count = focus->property("count");
    if (count.isValid()) {
        auto item = m_ui->charsGrid->itemAt(count.toInt() + m_ui->charsGrid->columnCount());
        if (item) {
            item->widget()->setFocus();
        } else {
            // Store the currently selected button and move to the line edit
            m_lastSelected = count.toInt();
            m_ui->selectedChars->setFocus();
        }
    } else if (focus == m_ui->selectedChars) {
        m_ui->pressTab->setFocus();
    }
}

QString PickcharsDialog::selectedChars()
{
    return m_ui->selectedChars->text();
}

bool PickcharsDialog::pressTab()
{
    return m_ui->pressTab->isChecked();
}

void PickcharsDialog::charSelected()
{
    auto btn = qobject_cast<QPushButton*>(sender());
    if (!btn) {
        return;
    }

    m_ui->selectedChars->setText(m_ui->selectedChars->text() + btn->property("char").toChar());
}

void PickcharsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    // Center on active screen
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    auto screen = QApplication::screenAt(QCursor::pos());
    if (!screen) {
        // screenAt can return a nullptr, default to the primary screen
        screen = QApplication::primaryScreen();
    }
    QRect screenGeometry = screen->availableGeometry();
#else
    QRect screenGeometry = QApplication::desktop()->availableGeometry(QCursor::pos());
#endif
    move(screenGeometry.center().x() - (size().width() / 2), screenGeometry.center().y() - (size().height() / 2));
}
