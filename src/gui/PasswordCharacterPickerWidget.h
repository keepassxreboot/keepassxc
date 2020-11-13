/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
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

#ifndef KEEPASSX_PASSWORDCHARACTERPICKERWIDGET_H
#define KEEPASSX_PASSWORDCHARACTERPICKERWIDGET_H

#include <QComboBox>
#include <QLabel>
#include <QWidget>

#include "core/PassphraseGenerator.h"
#include "core/PasswordGenerator.h"

namespace Ui
{
    class PasswordCharacterPickerWidget;
}

class PasswordCharacterPickerWidget : public QWidget
{
    Q_OBJECT

public:
    PasswordCharacterPickerWidget(QWidget* parent, QString&& password);
    ~PasswordCharacterPickerWidget();

    static PasswordCharacterPickerWidget* popupPicker(QWidget* parent, QString password);

signals:
    void closed();

private slots:
    void numberButtonPressed();
    void copyResult();
    void clearResult();

private:
    const QScopedPointer<Ui::PasswordCharacterPickerWidget> m_ui;
    QString m_password;
    std::vector<QString> m_passwordCharacters;
};

#endif // KEEPASSX_PASSWORDCHARACTERPICKERWIDGET_H
