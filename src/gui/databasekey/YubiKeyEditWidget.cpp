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

#include "ui_KeyComponentWidget.h"
#include "ui_YubiKeyEditWidget.h"

#include "core/AsyncTask.h"
#include "gui/Icons.h"
#include "keys/ChallengeResponseKey.h"
#include "keys/CompositeKey.h"
#include "keys/drivers/YubiKeyInterfaceUSB.h"

#include <botan/mem_ops.h>

YubiKeyEditWidget::YubiKeyEditWidget(QWidget* parent)
    : KeyComponentWidget(parent)
    , m_compUi(new Ui::YubiKeyEditWidget())
    , m_deviceListener(new DeviceListener(this))
{
    initComponent();
    connect(YubiKey::instance(), SIGNAL(detectComplete(bool)), SLOT(hardwareKeyResponse(bool)), Qt::QueuedConnection);
    connect(m_deviceListener, &DeviceListener::devicePlugged, this, [&](bool, void*, void*) { pollYubikey(); });
}

YubiKeyEditWidget::~YubiKeyEditWidget() = default;

bool YubiKeyEditWidget::addToCompositeKey(QSharedPointer<CompositeKey> key)
{
    if (!m_isDetected || !m_compEditWidget) {
        return false;
    }

    int selectionIndex = m_compUi->comboChallengeResponse->currentIndex();
    auto slot = m_compUi->comboChallengeResponse->itemData(selectionIndex).value<YubiKeySlot>();
    auto crKey = QSharedPointer<ChallengeResponseKey>::create(slot);
    if (crKey->needsPin() && m_pendingRemember && !m_pendingPin.empty()) {
        crKey->setCachedPin(m_pendingPin);
    }
    Botan::secure_scrub_memory(m_pendingPin.data(), m_pendingPin.capacity());
    m_pendingPin.clear();
    m_pendingRemember = false;
    key->addChallengeResponseKey(crKey);
    return true;
}

bool YubiKeyEditWidget::validate(QString& errorMessage) const
{
    if (!m_isDetected) {
        errorMessage = tr("Could not find any hardware keys!");
        return false;
    }

    // Perform a test challenge response
    int selectionIndex = m_compUi->comboChallengeResponse->currentIndex();
    auto slot = m_compUi->comboChallengeResponse->itemData(selectionIndex).value<YubiKeySlot>();
    bool valid = false;
    if (YubiKey::tpmSlotNeedsPin(slot)) {
        // Keep asking until the PIN works, the user cancels, or the TPM reports something other than a wrong PIN
        // (e.g. lockout). The TPM's own lockout protection limits the number of tries.
        QString message = AsyncTask::runAndWaitForFuture([&slot] { return YubiKey::instance()->pinNotice(slot); });
        while (!valid) {
            Botan::secure_vector<char> pin;
            bool remember = false;
            if (!YubiKey::requestPin(slot, message, pin, remember)) {
                errorMessage = tr("TPM2 PIN/password entry was cancelled.");
                return false;
            }
            auto result = YubiKey::ChallengeResult::YCR_ERROR;
            valid = AsyncTask::runAndWaitForFuture(
                [&slot, &pin, &result] { return YubiKey::instance()->testChallenge(slot, nullptr, &pin, &result); });
            if (valid) {
                m_pendingPin = pin;
                m_pendingRemember = remember;
            } else if (result == YubiKey::ChallengeResult::YCR_AUTHFAILED) {
                message = YubiKey::instance()->errorMessage();
            } else {
                errorMessage = YubiKey::instance()->errorMessage();
                return false;
            }
        }
    } else {
        valid = AsyncTask::runAndWaitForFuture([&slot] { return YubiKey::instance()->testChallenge(slot); });
    }
    if (!valid) {
        // TPM2 keys know what went wrong (for example a PIN is needed); say that instead of the generic text
        errorMessage = YubiKey::isTpmSlot(slot) ? YubiKey::instance()->errorMessage() : QString();
        if (errorMessage.isEmpty()) {
            errorMessage = tr("Selected hardware key slot does not support challenge-response!");
        }
    }
    return valid;
}

QWidget* YubiKeyEditWidget::componentEditWidget()
{
    m_compEditWidget = new QWidget();
    m_compUi->setupUi(m_compEditWidget);

    QSizePolicy sp = m_compUi->yubikeyProgress->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    m_compUi->yubikeyProgress->setSizePolicy(sp);
    m_compUi->yubikeyProgress->setVisible(false);

    return m_compEditWidget;
}

void YubiKeyEditWidget::showEvent(QShowEvent* event)
{
    KeyComponentWidget::showEvent(event);

#ifdef Q_OS_WIN
    m_deviceListener->registerHotplugCallback(true,
                                              true,
                                              YubiKeyInterfaceUSB::YUBICO_USB_VID,
                                              DeviceListener::MATCH_ANY,
                                              &DeviceListenerWin::DEV_CLS_KEYBOARD);
    m_deviceListener->registerHotplugCallback(true,
                                              true,
                                              YubiKeyInterfaceUSB::ONLYKEY_USB_VID,
                                              DeviceListener::MATCH_ANY,
                                              &DeviceListenerWin::DEV_CLS_KEYBOARD);
#else
    m_deviceListener->registerHotplugCallback(true, true, YubiKeyInterfaceUSB::YUBICO_USB_VID);
    m_deviceListener->registerHotplugCallback(true, true, YubiKeyInterfaceUSB::ONLYKEY_USB_VID);
#endif
}

void YubiKeyEditWidget::hideEvent(QHideEvent* event)
{
    KeyComponentWidget::hideEvent(event);
    m_deviceListener->deregisterAllHotplugCallbacks();
}

void YubiKeyEditWidget::initComponentEditWidget(QWidget* widget)
{
    Q_UNUSED(widget);
    Q_ASSERT(m_compEditWidget);
    m_compUi->comboChallengeResponse->setFocus();
    m_compUi->refreshHardwareKeys->setIcon(icons()->icon("yubikey-refresh", true));
    connect(m_compUi->refreshHardwareKeys, &QPushButton::clicked, this, &YubiKeyEditWidget::pollYubikey);
    pollYubikey();
}

void YubiKeyEditWidget::initComponent()
{
    // These need to be set in total for each credential type for translation purposes
    m_ui->groupBox->setTitle(tr("Challenge-Response"));
    m_ui->addButton->setText(tr("Add Challenge-Response"));
    m_ui->changeButton->setText(tr("Change Challenge-Response"));
    m_ui->removeButton->setText(tr("Remove Challenge-Response"));
    m_ui->changeOrRemoveLabel->setText(tr("Challenge-Response set, click to change or remove"));

    m_ui->componentDescription->setText(
        tr("<p>If you own a <a href=\"https://www.yubico.com/\">YubiKey</a> or "
           "<a href=\"https://onlykey.io\">OnlyKey</a>, you can use it for additional security.</p>"
           "<p>The key requires one of its slots to be programmed with "
           "<a href=\"https://keepassxc.org/docs/#faq-yubikey-howto\">"
           "Challenge-Response</a>.</p>"));
}

void YubiKeyEditWidget::pollYubikey()
{
    if (!m_compEditWidget) {
        return;
    }

    m_isDetected = false;
    m_compUi->comboChallengeResponse->clear();
    m_compUi->comboChallengeResponse->addItem(tr("Detecting hardware keys…"));
    m_compUi->comboChallengeResponse->setEnabled(false);
    m_compUi->yubikeyProgress->setVisible(true);
    m_compUi->refreshHardwareKeys->setEnabled(false);

    YubiKey::instance()->findValidKeysAsync();
}

void YubiKeyEditWidget::hardwareKeyResponse(bool found)
{
    if (!m_compEditWidget) {
        return;
    }

    m_compUi->comboChallengeResponse->clear();
    m_compUi->refreshHardwareKeys->setEnabled(true);

    if (!found) {
        m_compUi->yubikeyProgress->setVisible(false);
        const auto unusableTpmKeys = YubiKey::instance()->unusableTpmKeysHint();
        if (YubiKey::instance()->connectedKeys() > 0) {
            m_compUi->comboChallengeResponse->addItem(
                tr("Hardware keys found, but no slots are configured")
                + (unusableTpmKeys.isEmpty() ? QString() : QStringLiteral(". ") + unusableTpmKeys));
        } else {
            m_compUi->comboChallengeResponse->addItem(unusableTpmKeys.isEmpty() ? tr("No hardware keys detected")
                                                                                 : unusableTpmKeys);
        }
        m_isDetected = false;
        return;
    }

    const auto foundKeys = YubiKey::instance()->foundKeys();
    for (auto i = foundKeys.cbegin(); i != foundKeys.cend(); ++i) {
        // add detected YubiKey to combo box and encode blocking mode in LSB, slot number in second LSB
        m_compUi->comboChallengeResponse->addItem(i.value(), QVariant::fromValue(i.key()));
    }

    m_isDetected = true;
    m_compUi->yubikeyProgress->setVisible(false);
    m_compUi->comboChallengeResponse->setEnabled(true);
}
