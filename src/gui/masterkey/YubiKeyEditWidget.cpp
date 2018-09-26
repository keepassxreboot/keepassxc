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

#include "YubiKeyEditWidget.h"
#include "ui_YubiKeyEditWidget.h"
#include "gui/MessageBox.h"
#include "gui/MainWindow.h"
#include "keys/CompositeKey.h"
#include "keys/YkChallengeResponseKey.h"
#include "config-keepassx.h"

#include <QtConcurrent>

YubiKeyEditWidget::YubiKeyEditWidget(QWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::YubiKeyEditWidget())
{
    setComponentName(tr("YubiKey Challenge-Response"));
    setComponentDescription(tr("<p>If you own a <a href=\"https://www.yubico.com/\">YubiKey</a>, you can use it "
                               "for additional security.</p><p>The YubiKey requires one of its slots to be programmed as "
                               "<a href=\"https://www.yubico.com/products/services-software/personalization-tools/challenge-response/\">"
                               "HMAC-SHA1 Challenge-Response</a>.</p>"));
}

YubiKeyEditWidget::~YubiKeyEditWidget()
{
}

bool YubiKeyEditWidget::addToCompositeKey(QSharedPointer<CompositeKey> key)
{
    QSharedPointer<YkChallengeResponseKey> keyPtr;
    if (!createCrKey(keyPtr, false)) {
        return false;
    }
    key->addChallengeResponseKey(keyPtr);

    return true;
}

bool YubiKeyEditWidget::validate(QString& errorMessage) const
{
    QSharedPointer<YkChallengeResponseKey> keyPtr;
    if (!createCrKey(keyPtr)) {
        errorMessage = tr("No YubiKey detected, please ensure it's plugged in.");
        return false;
    }

    return true;
}

QWidget* YubiKeyEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);

    QSizePolicy sp = m_compUi->yubikeyProgress->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_compUi->yubikeyProgress->setSizePolicy(sp);
    m_compUi->yubikeyProgress->setVisible(false);

#ifdef WITH_XC_YUBIKEY
    connect(m_compUi->buttonRedetectYubikey, SIGNAL(clicked()), SLOT(pollYubikey()));

    connect(YubiKey::instance(), SIGNAL(detected(int, bool)), SLOT(yubikeyDetected(int, bool)), Qt::QueuedConnection);
    connect(YubiKey::instance(), SIGNAL(notFound()), SLOT(noYubikeyFound()), Qt::QueuedConnection);

    pollYubikey();
#endif

    return m_compEditWidget;
}

void YubiKeyEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
    m_compUi->comboChallengeResponse->setFocus();
}

void YubiKeyEditWidget::pollYubikey()
{
#ifdef WITH_XC_YUBIKEY
    if (!m_compEditWidget) {
        return;
    }
    m_compUi->buttonRedetectYubikey->setEnabled(false);
    m_compUi->comboChallengeResponse->setEnabled(false);
    m_compUi->comboChallengeResponse->clear();
    m_compUi->yubikeyProgress->setVisible(true);

    // YubiKey init is slow, detect asynchronously to not block the UI
    QtConcurrent::run(YubiKey::instance(), &YubiKey::detect);
#endif
}

void YubiKeyEditWidget::yubikeyDetected(int slot, bool blocking)
{
#ifdef WITH_XC_YUBIKEY
    if (!m_compEditWidget) {
        return;
    }
    YkChallengeResponseKey yk(slot, blocking);
    m_compUi->comboChallengeResponse->clear();
    // add detected YubiKey to combo box and encode blocking mode in LSB, slot number in second LSB
    m_compUi->comboChallengeResponse->addItem(yk.getName(), QVariant((slot << 1u) | blocking));
    m_compUi->comboChallengeResponse->setEnabled(true);
    m_compUi->buttonRedetectYubikey->setEnabled(true);
    m_compUi->yubikeyProgress->setVisible(false);
    m_isDetected = true;
#else
    Q_UNUSED(slot);
    Q_UNUSED(blocking);
#endif
}

void YubiKeyEditWidget::noYubikeyFound()
{
#ifdef WITH_XC_YUBIKEY
    if (!m_compEditWidget) {
        return;
    }
    m_compUi->comboChallengeResponse->clear();
    m_compUi->comboChallengeResponse->setEnabled(false);
    m_compUi->comboChallengeResponse->addItem(tr("No YubiKey inserted."));
    m_compUi->buttonRedetectYubikey->setEnabled(true);
    m_compUi->yubikeyProgress->setVisible(false);
    m_isDetected = false;
#endif
}

bool YubiKeyEditWidget::createCrKey(QSharedPointer<YkChallengeResponseKey>& key, bool testChallenge) const
{
    Q_ASSERT(m_compEditWidget);
    if (!m_isDetected || !m_compEditWidget) {
        return false;
    }

    int selectionIndex = m_compUi->comboChallengeResponse->currentIndex();
    int comboPayload = m_compUi->comboChallengeResponse->itemData(selectionIndex).toInt();

    if (0 == comboPayload) {
        return false;
    }

    auto blocking = static_cast<bool>(comboPayload & 1u);
    int slot = comboPayload >> 1u;
    key.reset(new YkChallengeResponseKey(slot, blocking));
    if (testChallenge) {
        return key->challenge(QByteArray("0000"));
    }
    return true;
}
