/*
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

#include "KeyFileEditWidget.h"
#include "ui_KeyFileEditWidget.h"
#include <gui/dbsettings/DatabaseSettingsWidget.h>

#include "gui/FileDialog.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "keys/CompositeKey.h"
#include "keys/FileKey.h"

KeyFileEditWidget::KeyFileEditWidget(DatabaseSettingsWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::KeyFileEditWidget())
    , m_parent(parent)
{
    setComponentName(tr("Key File"));
    setComponentDescription(tr("<p>You can add a key file containing random bytes for additional security.</p>"
                               "<p>You must keep it secret and never lose it or you will be locked out!</p>"));
}

KeyFileEditWidget::~KeyFileEditWidget()
{
}

bool KeyFileEditWidget::addToCompositeKey(QSharedPointer<CompositeKey> key)
{
    auto fileKey = QSharedPointer<FileKey>::create();
    QString fileKeyName = m_compUi->keyFileCombo->currentText();
    if (!fileKey->load(fileKeyName, nullptr)) {
        return false;
    }

    if (fileKey->type() != FileKey::KeePass2XMLv2 && fileKey->type() != FileKey::Hashed) {
        QMessageBox::warning(getMainWindow(),
                             tr("Old key file format"),
                             tr("You selected a key file in an old format which KeePassXC<br>"
                                "may stop supporting in the future.<br><br>"
                                "Please consider generating a new key file instead."),
                             QMessageBox::Ok);
    }

    key->addKey(fileKey);
    return true;
}

bool KeyFileEditWidget::validate(QString& errorMessage) const
{
    FileKey fileKey;
    QString fileKeyError;
    QString fileKeyName = m_compUi->keyFileCombo->currentText();
    if (!fileKey.load(fileKeyName, &fileKeyError)) {
        errorMessage = tr("Error loading the key file '%1'\nMessage: %2").arg(fileKeyName, fileKeyError);
        return false;
    }
    return true;
}

QWidget* KeyFileEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);

    connect(m_compUi->createKeyFileButton, SIGNAL(clicked()), SLOT(createKeyFile()));
    connect(m_compUi->browseKeyFileButton, SIGNAL(clicked()), SLOT(browseKeyFile()));

    return m_compEditWidget;
}

void KeyFileEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
    m_compUi->keyFileCombo->setFocus();
}

void KeyFileEditWidget::createKeyFile()
{
    Q_ASSERT(m_compEditWidget);
    if (!m_compEditWidget) {
        return;
    }
    QString filters = QString("%1 (*.keyx; *.key);;%2 (*)").arg(tr("Key files"), tr("All files"));
    QString fileName = fileDialog()->getSaveFileName(this, tr("Create Key File..."), QString(), filters);

    if (!fileName.isEmpty()) {
        QString errorMsg;
        bool created = FileKey::create(fileName, &errorMsg);
        if (!created) {
            MessageBox::critical(getMainWindow(),
                                 tr("Error creating key file"),
                                 tr("Unable to create key file: %1").arg(errorMsg),
                                 QMessageBox::Button::Ok);
        } else {
            m_compUi->keyFileCombo->setEditText(fileName);
        }
    }
}

void KeyFileEditWidget::browseKeyFile()
{
    Q_ASSERT(m_compEditWidget);
    if (!m_compEditWidget) {
        return;
    }
    QString filters = QString("%1 (*.keyx; *.key);;%2 (*)").arg(tr("Key files"), tr("All files"));
    QString fileName = fileDialog()->getOpenFileName(this, tr("Select a key file"), QString(), filters);

    if (QFileInfo(fileName).canonicalFilePath() == m_parent->getDatabase()->canonicalFilePath()) {
        MessageBox::critical(getMainWindow(),
                             tr("Invalid Key File"),
                             tr("You cannot use the current database as its own keyfile. Please choose a different "
                                "file or generate a new key file."));
        return;
    } else if (fileName.endsWith(".kdbx", Qt::CaseInsensitive)) {
        auto response =
            MessageBox::warning(getMainWindow(),
                                tr("Suspicious Key File"),
                                tr("The chosen key file looks like a password database file. A key file must be a "
                                   "static file that never changes or you will lose access to your database "
                                   "forever.\nAre you sure you want to continue with this file?"),
                                MessageBox::Continue | MessageBox::Cancel,
                                MessageBox::Cancel);
        if (response != MessageBox::Continue) {
            return;
        }
    }

    if (!fileName.isEmpty()) {
        m_compUi->keyFileCombo->setEditText(fileName);
    }
}
