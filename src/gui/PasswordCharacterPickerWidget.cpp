/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#include "PasswordCharacterPickerWidget.h"
#include "ui_PasswordCharacterPickerWidget.h"

#include <QDir>
#include <QKeyEvent>
#include <QLineEdit>
#include <QtCore/QTextBoundaryFinder>

#include "Font.h"
#include "core/Resources.h"
#include "gui/Clipboard.h"

PasswordCharacterPickerWidget::PasswordCharacterPickerWidget(QWidget* parent, QString&& password)
    : QWidget(parent)
    , m_ui(new Ui::PasswordCharacterPickerWidget())
    , m_password(std::move(password))
{
    // split password into unicode graphemes
    {
        QTextBoundaryFinder tbf(QTextBoundaryFinder::Grapheme, m_password);
        int start = 0;
        while (tbf.toNextBoundary() != -1) {
            int end = tbf.position();
            m_passwordCharacters.push_back(m_password.mid(start, end - start));
            start = end;
        }
    }

    m_ui->setupUi(this);

    QFont passwordFont = Font::fixedFont();
    passwordFont.setLetterSpacing(QFont::PercentageSpacing, 110);
    m_ui->passwordLine->setFont(passwordFont);

    connect(m_ui->buttonClose, SIGNAL(clicked()), SIGNAL(closed()));
    connect(m_ui->buttonCopy, SIGNAL(clicked()), SLOT(copyResult()));
    connect(m_ui->buttonClear, SIGNAL(clicked()), SLOT(clearResult()));

    for (int i = 0; i < m_password.length(); i++) {
        auto* button = new QPushButton(std::to_string(i + 1).c_str());
        m_ui->numberContainer->addWidget(button);
        connect(button, SIGNAL(clicked()), SLOT(numberButtonPressed()));
    }
}

PasswordCharacterPickerWidget::~PasswordCharacterPickerWidget()
{
}

void PasswordCharacterPickerWidget::numberButtonPressed()
{
    QPushButton* pressed = reinterpret_cast<QPushButton*>(QObject::sender());
    m_ui->passwordLine->setText(m_ui->passwordLine->text()
                                + m_passwordCharacters[std::stoi(pressed->text().toStdString()) - 1]);
}

void PasswordCharacterPickerWidget::copyResult()
{
    clipboard()->setText(m_ui->passwordLine->text());
}

void PasswordCharacterPickerWidget::clearResult()
{
    m_ui->passwordLine->clear();
}

PasswordCharacterPickerWidget* PasswordCharacterPickerWidget::popupPicker(QWidget* parent, QString password)
{
    auto picker = new PasswordCharacterPickerWidget(parent, std::move(password));
    picker->setWindowModality(Qt::ApplicationModal);
    picker->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    connect(picker, SIGNAL(closed()), picker, SLOT(deleteLater()));

    picker->show();
    picker->raise();
    picker->activateWindow();
    picker->adjustSize();

    return picker;
}
