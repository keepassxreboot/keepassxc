/*
 *  Copyright (C) 2017 Daniel Meek <https://github.com/danielmeek32>
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

#include "AutoTypePickCharsDialog.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalMapper>
#include <QToolButton>
#include <QVBoxLayout>

#include "core/Config.h"
#include "core/FilePath.h"

AutoTypePickCharsDialog::AutoTypePickCharsDialog(QString string, QWidget* parent)
    : QDialog(parent)
    , m_string(string)
    , m_passwordEdit(new PasswordEdit(this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    // Places the window on the active (virtual) desktop instead of where the main window is.
    setAttribute(Qt::WA_X11BypassTransientForHint);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setWindowTitle(tr("Auto-Type - KeePassXC"));
    setWindowIcon(filePath()->applicationIcon());
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* descriptionLabel = new QLabel(tr("Select characters:"), this);
    layout->addWidget(descriptionLabel);

    // create buttons for choosing characters
    int count = m_string.length();
    int rowLength = count;
    if (count > 10) {
        // split long rows
        if (count % 2 == 0 && count / 2 <= 10) {
            // split in half if it's short enough
            rowLength = count / 2;
        }
        else if ((count + 1) % 2 == 0 && (count + 1) / 2 <= 10) {
            // split in half if it's short enough
            rowLength = (count + 1) / 2;
        }
        else {
            // split into rows of 10 buttons each
            rowLength = 10;
        }
    }
    QGridLayout* buttonLayout = new QGridLayout();
    QSignalMapper* signalMapper = new QSignalMapper(this);
    for (int i = 0; i < count; i++) {
        QPushButton* button = new QPushButton(QString::number(i + 1), this);
        button->setMaximumWidth(qMin(button->width(), button->height() * 2));
        connect(button, SIGNAL(clicked(bool)), signalMapper, SLOT(map()));
        signalMapper->setMapping(button, i);
        buttonLayout->addWidget(button, i / rowLength, i % rowLength);
    }
    connect(signalMapper, SIGNAL(mapped(int)), SLOT(addChar(int)));
    layout->addLayout(buttonLayout);

    // create layout for character entry field
    QGridLayout* passwordLayout = new QGridLayout();
    QToolButton* togglePasswordButton = new QToolButton(this);
    togglePasswordButton->setCheckable(true);
    togglePasswordButton->setIcon(filePath()->onOffIcon("actions", "password-show"));
    connect(togglePasswordButton, SIGNAL(toggled(bool)), m_passwordEdit, SLOT(setShowPassword(bool)));
    togglePasswordButton->setChecked(config()->get("security/passwordscleartext").toBool());
    passwordLayout->addWidget(m_passwordEdit, 0, 0);
    passwordLayout->addWidget(togglePasswordButton, 0, 1);
    layout->addLayout(passwordLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, Qt::Horizontal, this);
    connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
    layout->addWidget(buttonBox);

    // set window size
    adjustSize();
    setFixedSize(size());

    // move dialog to the center of the screen
    QRect screenGeometry = QApplication::desktop()->availableGeometry(QCursor::pos());
    QPoint screenCenter = screenGeometry.center();
    move(screenCenter.x() - (width() / 2), screenCenter.y() - (height() / 2));
}

QString AutoTypePickCharsDialog::chars()
{
    return m_passwordEdit->text();
}

void AutoTypePickCharsDialog::done(int r)
{
    config()->set("GUI/AutoTypePickCharsDialogSize", size());

    QDialog::done(r);
}

void AutoTypePickCharsDialog::reject()
{
    m_passwordEdit->setText("");

    QDialog::reject();
}

void AutoTypePickCharsDialog::addChar(int index)
{
     m_passwordEdit->insert(m_string.at(index));
}
