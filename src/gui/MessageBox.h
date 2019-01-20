/*
 *  Copyright (C) 2013 Felix Geyer <debfx@fobos.de>
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSX_MESSAGEBOX_H
#define KEEPASSX_MESSAGEBOX_H

#include <QMap>
#include <QMessageBox>
#include <QPushButton>

class MessageBox
{
public:
    enum Button : uint64_t
    {
        // Reimplementation of Qt StandardButtons
        NoButton = 0,
        Ok = 1 << 1,
        Open = 1 << 2,
        Save = 1 << 3,
        Cancel = 1 << 4,
        Close = 1 << 5,
        Discard = 1 << 6,
        Apply = 1 << 7,
        Reset = 1 << 8,
        RestoreDefaults = 1 << 9,
        Help = 1 << 10,
        SaveAll = 1 << 11,
        Yes = 1 << 12,
        YesToAll = 1 << 13,
        No = 1 << 14,
        NoToAll = 1 << 15,
        Abort = 1 << 16,
        Retry = 1 << 17,
        Ignore = 1 << 18,

        // KeePassXC Buttons
        Overwrite = 1 << 19,
        Delete = 1 << 20,
        Move = 1 << 21,
        Empty = 1 << 22,
        Remove = 1 << 23,
        Skip = 1 << 24,
        Disable = 1 << 25,
        Merge = 1 << 26,

        // Internal loop markers. Update Last when new KeePassXC button is added
        First = Ok,
        Last = Merge,
    };

    enum Action
    {
        None = 0,
        Raise = 1,
    };

    typedef uint64_t Buttons;

    static void initializeButtonDefs();
    static void setNextAnswer(Button button);

    static Button critical(QWidget* parent,
                           const QString& title,
                           const QString& text,
                           Buttons buttons = MessageBox::Ok,
                           Button defaultButton = MessageBox::NoButton,
                           Action action = MessageBox::None);
    static Button information(QWidget* parent,
                              const QString& title,
                              const QString& text,
                              Buttons buttons = MessageBox::Ok,
                              Button defaultButton = MessageBox::NoButton,
                              Action action = MessageBox::None);
    static Button question(QWidget* parent,
                           const QString& title,
                           const QString& text,
                           Buttons buttons = MessageBox::Ok,
                           Button defaultButton = MessageBox::NoButton,
                           Action action = MessageBox::None);
    static Button warning(QWidget* parent,
                          const QString& title,
                          const QString& text,
                          Buttons buttons = MessageBox::Ok,
                          Button defaultButton = MessageBox::NoButton,
                          Action action = MessageBox::None);

private:
    static Button m_nextAnswer;
    static QMap<QAbstractButton*, Button> m_addedButtonLookup;
    static QMap<Button, std::pair<QString, QMessageBox::ButtonRole>> m_buttonDefs;

    static Button messageBox(QWidget* parent,
                             QMessageBox::Icon icon,
                             const QString& title,
                             const QString& text,
                             Buttons buttons = MessageBox::Ok,
                             Button defaultButton = MessageBox::NoButton,
                             Action action = MessageBox::None);

    static QString stdButtonText(QMessageBox::StandardButton button);
};

#endif // KEEPASSX_MESSAGEBOX_H
