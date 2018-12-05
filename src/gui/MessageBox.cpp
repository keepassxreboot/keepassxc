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

#include "MessageBox.h"

MessageBox::Button MessageBox::m_nextAnswer(MessageBox::NoButton);
QMap<QAbstractButton*, MessageBox::Button> MessageBox::m_buttonMap = QMap<QAbstractButton*, MessageBox::Button>();


MessageBox::Button MessageBox::messageBox(QWidget* parent,
                                          QMessageBox::Icon icon,
                                          const QString& title,
                                          const QString& text,
                                          MessageBox::Buttons buttons,
                                          MessageBox::Button defaultButton)
{
    if (m_nextAnswer == MessageBox::NoButton) {
        QMessageBox msgBox(parent);
        msgBox.setIcon(icon);
        msgBox.setWindowTitle(title);
        msgBox.setText(text);

        for (uint64_t b = First; b <= Last; b <<= 1) {
            if (b & buttons) {
                addButton(msgBox, static_cast<Button>(b));
            }
        }

        if (defaultButton != MessageBox::NoButton) {
            QList<QAbstractButton*> defPtrList = m_buttonMap.keys(defaultButton);
            if (defPtrList.count() > 0) {
                msgBox.setDefaultButton(static_cast<QPushButton*>(defPtrList[0]));
            }
        }
        
        msgBox.exec();

        Button returnButton = m_buttonMap[msgBox.clickedButton()];
        m_buttonMap.clear();
        return returnButton;

    } else {
        MessageBox::Button returnButton = m_nextAnswer;
        m_nextAnswer = MessageBox::NoButton;
        return returnButton;
    }
}

MessageBox::Button MessageBox::critical(QWidget* parent,
                                        const QString& title,
                                        const QString& text,
                                        MessageBox::Buttons buttons,
                                        MessageBox::Button defaultButton)
{
    return messageBox(parent, QMessageBox::Critical, title, text, buttons, defaultButton);
}

MessageBox::Button MessageBox::information(QWidget* parent,
                                           const QString& title,
                                           const QString& text,
                                           MessageBox::Buttons buttons,
                                           MessageBox::Button defaultButton)
{
    return messageBox(parent, QMessageBox::Information, title, text, buttons, defaultButton);
}

MessageBox::Button MessageBox::question(QWidget* parent,
                                        const QString& title,
                                        const QString& text,
                                        MessageBox::Buttons buttons,
                                        MessageBox::Button defaultButton)
{
    return messageBox(parent, QMessageBox::Question, title, text, buttons, defaultButton);
}

MessageBox::Button MessageBox::warning(QWidget* parent,
                                       const QString& title,
                                       const QString& text,
                                       MessageBox::Buttons buttons,
                                       MessageBox::Button defaultButton)
{
    return messageBox(parent, QMessageBox::Warning, title, text, buttons, defaultButton);
}

void MessageBox::setNextAnswer(MessageBox::Button button)
{
    m_nextAnswer = button;
}

void MessageBox::addButton(QMessageBox &box, MessageBox::Button button)
{
    QAbstractButton *b;
    QMessageBox buttonHost;
    QPushButton *btn = nullptr;
    switch(button) {

        // Reimplementation of Qt StandardButtons
        case NoButton:
            return;
        case Ok:
            btn = buttonHost.addButton(QMessageBox::Ok);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::AcceptRole);
            break;
        case Open:
            btn = buttonHost.addButton(QMessageBox::Open);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::AcceptRole);
            break;
        case Save:
            btn = buttonHost.addButton(QMessageBox::Save);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::AcceptRole);
            break;
        case Cancel:
            btn = buttonHost.addButton(QMessageBox::Cancel);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::RejectRole);
            break;
        case Close:
            btn = buttonHost.addButton(QMessageBox::Close);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::RejectRole);
            break;
        case Discard:
            btn = buttonHost.addButton(QMessageBox::Discard);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::DestructiveRole);
            break;
        case Apply:
            btn = buttonHost.addButton(QMessageBox::Apply);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::ApplyRole);
            break;
        case Reset:
            btn = buttonHost.addButton(QMessageBox::Reset);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::ResetRole);
            break;
        case RestoreDefaults:
            btn = buttonHost.addButton(QMessageBox::RestoreDefaults);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::ResetRole);
            break;
        case Help:
            btn = buttonHost.addButton(QMessageBox::Help);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::HelpRole);
            break;
        case SaveAll:
            btn = buttonHost.addButton(QMessageBox::SaveAll);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::AcceptRole);
            break;
        case Yes:
            btn = buttonHost.addButton(QMessageBox::Yes);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::YesRole);
            break;
        case YesToAll:
            btn = buttonHost.addButton(QMessageBox::YesToAll);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::YesRole);
            break;
        case No:
            btn = buttonHost.addButton(QMessageBox::No);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::NoRole);
            break;
        case NoToAll:
            btn = buttonHost.addButton(QMessageBox::NoToAll);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::NoRole);
            break;
        case Abort:
            btn = buttonHost.addButton(QMessageBox::Abort);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::RejectRole);
            break;
        case Retry:
            btn = buttonHost.addButton(QMessageBox::Retry);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::AcceptRole);
            break;
        case Ignore:
            btn = buttonHost.addButton(QMessageBox::Ignore);
            b = box.addButton(btn->text(), QMessageBox::ButtonRole::AcceptRole);
            break;

        // KeePassXC Buttons
        case Overwrite:
            b = box.addButton(QMessageBox::tr("Overwrite"), QMessageBox::ButtonRole::DestructiveRole);
            break;
        case Delete:
            b = box.addButton(QMessageBox::tr("Delete"), QMessageBox::ButtonRole::DestructiveRole);
            break;
        case Move:
            b = box.addButton(QMessageBox::tr("Move"), QMessageBox::ButtonRole::AcceptRole);
            break;
        case Empty:
            b = box.addButton(QMessageBox::tr("Empty"), QMessageBox::ButtonRole::DestructiveRole);
            break;
        case Remove:
            b = box.addButton(QMessageBox::tr("Remove"), QMessageBox::ButtonRole::DestructiveRole);
            break;
        case Skip:
            b = box.addButton(QMessageBox::tr("Skip"), QMessageBox::ButtonRole::RejectRole);
            break;
    }
    m_buttonMap.insert(b, button);
}
