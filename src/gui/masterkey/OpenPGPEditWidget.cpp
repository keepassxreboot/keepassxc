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

#include "OpenPGPEditWidget.h"
#include "ui_OpenPGPEditWidget.h"

#include "config-keepassx.h"
#include "gui/MainWindow.h"
#include "gui/MessageBox.h"
#include "keys/CompositeKey.h"
#include "keys/OpenPGP.h"

#include <QtConcurrent>

OpenPGPEditWidget::OpenPGPEditWidget(QWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::OpenPGPEditWidget())
{
    setComponentName(tr("PGP Assymetric Database Encryption"));
    setComponentDescription(
        tr("<p>Use an PGP implementation like GnuPG for assymetric database encrytion."
        "With GnuPG, OpenPGP Smart Cards (i.e. Nitrokey Pro) are also supported.</p>"));
}

OpenPGPEditWidget::~OpenPGPEditWidget()
{
}

bool OpenPGPEditWidget::addToCompositeKey(QSharedPointer<CompositeKey> key)
{
    key = key; // TODO (Just remove compile messages for now)
    QSharedPointer<OpenPGPKey> keyPtr;
    //key->addOpenPGPKey(keyPtr);
    return true;
}

bool OpenPGPEditWidget::validate(QString& errorMessage) const
{
    errorMessage = errorMessage; //TODO
    return false;
    return true;
}

QWidget* OpenPGPEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);

    //connect(m_compUi->createKeyFileButton, SIGNAL(clicked()), SLOT(createKeyFile()));
    //connect(m_compUi->browseKeyFileButton, SIGNAL(clicked()), SLOT(browseKeyFile()));

    return m_compEditWidget;
}

void OpenPGPEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
    //m_compUi->keyFileCombo->setFocus();
}


void OpenPGPEditWidget::listAvailableKeys()
{
#ifdef WITH_XC_OPENPGP
    if (!m_compEditWidget) {
        return;
    }
/*
    m_isDetected = false;
    m_compUi->comboChallengeResponse->clear();
    m_compUi->buttonRedetectYubikey->setEnabled(false);
    m_compUi->comboChallengeResponse->setEnabled(false);
    m_compUi->yubikeyProgress->setVisible(true);

    // YubiKey init is slow, detect asynchronously to not block the UI
    QtConcurrent::run(YubiKey::instance(), &YubiKey::detect);*/
#endif
}