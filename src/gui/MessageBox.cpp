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

QMap<QAbstractButton*, MessageBox::Button> MessageBox::m_addedButtonLookup =
    QMap<QAbstractButton*, MessageBox::Button>();

QMap<MessageBox::Button, std::pair<QString, QMessageBox::ButtonRole>> MessageBox::m_buttonDefs =
    QMap<MessageBox::Button, std::pair<QString, QMessageBox::ButtonRole>>();

void MessageBox::initializeButtonDefs()
{
    m_buttonDefs = QMap<Button, std::pair<QString, QMessageBox::ButtonRole>>{
        // Reimplementation of Qt StandardButtons
        {Ok, {stdButtonText(QMessageBox::Ok), QMessageBox::ButtonRole::AcceptRole}},
        {Open, {stdButtonText(QMessageBox::Open), QMessageBox::ButtonRole::AcceptRole}},
        {Save, {stdButtonText(QMessageBox::Save), QMessageBox::ButtonRole::AcceptRole}},
        {Cancel, {stdButtonText(QMessageBox::Cancel), QMessageBox::ButtonRole::RejectRole}},
        {Close, {stdButtonText(QMessageBox::Close), QMessageBox::ButtonRole::RejectRole}},
        {Discard, {stdButtonText(QMessageBox::Discard), QMessageBox::ButtonRole::DestructiveRole}},
        {Apply, {stdButtonText(QMessageBox::Apply), QMessageBox::ButtonRole::ApplyRole}},
        {Reset, {stdButtonText(QMessageBox::Reset), QMessageBox::ButtonRole::ResetRole}},
        {RestoreDefaults, {stdButtonText(QMessageBox::RestoreDefaults), QMessageBox::ButtonRole::ResetRole}},
        {Help, {stdButtonText(QMessageBox::Help), QMessageBox::ButtonRole::HelpRole}},
        {SaveAll, {stdButtonText(QMessageBox::SaveAll), QMessageBox::ButtonRole::AcceptRole}},
        {Yes, {stdButtonText(QMessageBox::Yes), QMessageBox::ButtonRole::YesRole}},
        {YesToAll, {stdButtonText(QMessageBox::YesToAll), QMessageBox::ButtonRole::YesRole}},
        {No, {stdButtonText(QMessageBox::No), QMessageBox::ButtonRole::NoRole}},
        {NoToAll, {stdButtonText(QMessageBox::NoToAll), QMessageBox::ButtonRole::NoRole}},
        {Abort, {stdButtonText(QMessageBox::Abort), QMessageBox::ButtonRole::RejectRole}},
        {Retry, {stdButtonText(QMessageBox::Retry), QMessageBox::ButtonRole::AcceptRole}},
        {Ignore, {stdButtonText(QMessageBox::Ignore), QMessageBox::ButtonRole::AcceptRole}},

        // KeePassXC Buttons
        {Overwrite, {QMessageBox::tr("Overwrite"), QMessageBox::ButtonRole::AcceptRole}},
        {Delete, {QMessageBox::tr("Delete"), QMessageBox::ButtonRole::AcceptRole}},
        {Move, {QMessageBox::tr("Move"), QMessageBox::ButtonRole::AcceptRole}},
        {Empty, {QMessageBox::tr("Empty"), QMessageBox::ButtonRole::AcceptRole}},
        {Remove, {QMessageBox::tr("Remove"), QMessageBox::ButtonRole::AcceptRole}},
        {Skip, {QMessageBox::tr("Skip"), QMessageBox::ButtonRole::AcceptRole}},
        {Disable, {QMessageBox::tr("Disable"), QMessageBox::ButtonRole::AcceptRole}},
        {Merge, {QMessageBox::tr("Merge"), QMessageBox::ButtonRole::AcceptRole}},
    };
}

QString MessageBox::stdButtonText(QMessageBox::StandardButton button)
{
    QMessageBox buttonHost;
    return buttonHost.addButton(button)->text();
}

MessageBox::Button MessageBox::messageBox(QWidget* parent,
                                          QMessageBox::Icon icon,
                                          const QString& title,
                                          const QString& text,
                                          MessageBox::Buttons buttons,
                                          MessageBox::Button defaultButton,
                                          MessageBox::Action action)
{
    if (m_nextAnswer == MessageBox::NoButton) {
        QMessageBox msgBox(parent);
        msgBox.setIcon(icon);
        msgBox.setWindowTitle(title);
        msgBox.setText(text);

        for (uint64_t b = First; b <= Last; b <<= 1) {
            if (b & buttons) {
                QString text = m_buttonDefs[static_cast<Button>(b)].first;
                QMessageBox::ButtonRole role = m_buttonDefs[static_cast<Button>(b)].second;

                auto buttonPtr = msgBox.addButton(text, role);
                m_addedButtonLookup.insert(buttonPtr, static_cast<Button>(b));
            }
        }

        if (defaultButton != MessageBox::NoButton) {
            QList<QAbstractButton*> defPtrList = m_addedButtonLookup.keys(defaultButton);
            if (defPtrList.count() > 0) {
                msgBox.setDefaultButton(static_cast<QPushButton*>(defPtrList[0]));
            }
        }

        if (action == MessageBox::Raise) {
            msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
            msgBox.activateWindow();
            msgBox.raise();
        }
        msgBox.exec();

        Button returnButton = m_addedButtonLookup[msgBox.clickedButton()];
        m_addedButtonLookup.clear();
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
                                        MessageBox::Button defaultButton,
                                        MessageBox::Action action)
{
    return messageBox(parent, QMessageBox::Critical, title, text, buttons, defaultButton, action);
}

MessageBox::Button MessageBox::information(QWidget* parent,
                                           const QString& title,
                                           const QString& text,
                                           MessageBox::Buttons buttons,
                                           MessageBox::Button defaultButton,
                                           MessageBox::Action action)
{
    return messageBox(parent, QMessageBox::Information, title, text, buttons, defaultButton, action);
}

MessageBox::Button MessageBox::question(QWidget* parent,
                                        const QString& title,
                                        const QString& text,
                                        MessageBox::Buttons buttons,
                                        MessageBox::Button defaultButton,
                                        MessageBox::Action action)
{
    return messageBox(parent, QMessageBox::Question, title, text, buttons, defaultButton, action);
}

MessageBox::Button MessageBox::warning(QWidget* parent,
                                       const QString& title,
                                       const QString& text,
                                       MessageBox::Buttons buttons,
                                       MessageBox::Button defaultButton,
                                       MessageBox::Action action)
{
    return messageBox(parent, QMessageBox::Warning, title, text, buttons, defaultButton, action);
}

void MessageBox::setNextAnswer(MessageBox::Button button)
{
    m_nextAnswer = button;
}
