/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
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

#include <utility>

#include "AutoTypeExtLibvirt.h"
#include "core/Config.h"

const QHash<uint, uint> AutoTypeExtLibvirt::m_lowerSymbolKeyMapping = {
    {0x20, 0x20}, // space
    {0x27, 0xde}, // '
    {0x2c, 0xbc}, // ,
    {0x2d, 0xbd}, // -
    {0x2e, 0xbe}, // .
    {0x2f, 0xbf}, // /
    {0x3b, 0xba}, // ;
    {0x3d, 0xbb}, // =
    {0x5b, 0xdb}, // [
    {0x5c, 0xdc}, // backslash
    {0x5d, 0xdd}, // ]
    {0x60, 0xc0}, // `
};

const QHash<uint, uint> AutoTypeExtLibvirt::m_upperSymbolKeyMapping = {
    {0x21, 0x31}, // !
    {0x22, 0xde}, // "
    {0x23, 0x33}, // #
    {0x24, 0x34}, // $
    {0x25, 0x35}, // %
    {0x26, 0x37}, // &
    {0x28, 0x39}, // (
    {0x29, 0x30}, // )
    {0x2a, 0x38}, // *
    {0x2b, 0xbb}, // +
    {0x3a, 0xba}, // :
    {0x3c, 0xbc}, // <
    {0x3e, 0xbe}, // >
    {0x3f, 0xbf}, // ?
    {0x40, 0x32}, // @
    {0x5e, 0x36}, // ^
    {0x5f, 0xbd}, // _
    {0x7b, 0xdb}, // {
    {0x7c, 0xdc}, // |
    {0x7d, 0xdd}, // }
    {0x7e, 0xc0}, // ~
};

const QHash<Qt::Key, uint> AutoTypeExtLibvirt::m_keyToKeyCodeMapping = {
    {Qt::Key_Tab,        0x09},
    {Qt::Key_Enter,      0x0d},
    {Qt::Key_Space,      0x20},
    {Qt::Key_Left,       0x25},
    {Qt::Key_Up,         0x26},
    {Qt::Key_Right,      0x27},
    {Qt::Key_Down,       0x28},
    {Qt::Key_Insert,     0x2d},
    {Qt::Key_Delete,     0x2e},
    {Qt::Key_Home,       0x24},
    {Qt::Key_End,        0x23},
    {Qt::Key_PageUp,     0x21},
    {Qt::Key_PageDown,   0x22},
    {Qt::Key_Backspace,  0x08},
    {Qt::Key_Pause,      0x13},
    {Qt::Key_CapsLock,   0x14},
    {Qt::Key_Escape,     0x1b},
    {Qt::Key_Help,       0x2f},
    {Qt::Key_NumLock,    0x90},
    {Qt::Key_Print,      0x2a},
    {Qt::Key_ScrollLock, 0x91},
    {Qt::Key_Shift,      0xa0},
    {Qt::Key_Control,    0xa2},
    {Qt::Key_Alt,        0xa4},
    {Qt::Key_F1,         0x70},
    {Qt::Key_F2,         0x71},
    {Qt::Key_F3,         0x72},
    {Qt::Key_F4,         0x73},
    {Qt::Key_F5,         0x74},
    {Qt::Key_F6,         0x75},
    {Qt::Key_F7,         0x76},
    {Qt::Key_F8,         0x77},
    {Qt::Key_F9,         0x78},
    {Qt::Key_F10,        0x79},
    {Qt::Key_F11,        0x7a},
    {Qt::Key_F12,        0x7b},
    {Qt::Key_F13,        0x7c},
    {Qt::Key_F14,        0x7d},
    {Qt::Key_F15,        0x7e},
    {Qt::Key_F16,        0x7f},
};

const QHash<Qt::KeyboardModifier, uint> AutoTypeExtLibvirt::m_modifierToKeyCodeMapping = {
    {Qt::KeyboardModifier::ShiftModifier,   0xa0},
    {Qt::KeyboardModifier::ControlModifier, 0x11},
    {Qt::KeyboardModifier::AltModifier,     0xa4},
    {Qt::KeyboardModifier::MetaModifier,    0x5b},
};

const QVector<uint> AutoTypeExtLibvirt::m_deadKeys = {
    0x27, // '
    0x22, // "
    0x5e, // ^
    0x60, // `
    0x7e, // ~
};

const QRegExp AutoTypeTargetLibvirt::m_patternLinuxOperatingSystemId = QRegExp(
    "^(ubuntu|debian|centos|fedora|redhat|opensuse|archlinux|alpinelinux|system76)$"
);

const QRegExp AutoTypeTargetLibvirt::m_patternLinuxOperatingSystemUrl = QRegExp(
    "http://(ubuntu|debian|centos|fedora|redhat|opensuse|archlinux|alpinelinux|system76)"
);

AutoTypeExtLibvirt::AutoTypeExtLibvirt()
{
    m_libvirtConnection = virConnectOpen(nullptr);
    if (m_libvirtConnection == nullptr) {
        qWarning("Failed connecting to the default libvirt URI");
    }
}

void AutoTypeExtLibvirt::unload()
{
    virConnectClose(m_libvirtConnection);
    m_libvirtConnection = nullptr;
}

bool AutoTypeExtLibvirt::isAvailable()
{
    return m_libvirtConnection != nullptr;
}

bool AutoTypeExtLibvirt::isTargetSelectionRequired()
{
    return true;
}

AutoTypeTargetMap AutoTypeExtLibvirt::availableTargets()
{
    auto targetMap = AutoTypeTargetMap();

    if (m_libvirtConnection == nullptr) {
        qWarning("No open libvirt connection available");
        return targetMap;
    }

    virDomainPtr* domainList;

    int domainCount = virConnectListAllDomains(m_libvirtConnection, &domainList, VIR_CONNECT_LIST_DOMAINS_RUNNING);

    for (int i = 0; i < domainCount; i++) {
        virDomainPtr currentDomain = domainList[i];

        auto name = QString(virDomainGetName(currentDomain));

        char uuidBuffer[VIR_UUID_STRING_BUFLEN];

        if (virDomainGetUUIDString(currentDomain, uuidBuffer) < 0) {
            qWarning("Failed getting uuid for libvirt domain %s", qPrintable(name));
            virDomainFree(currentDomain);
            continue;
        }

        targetMap.append(QSharedPointer<AutoTypeTargetLibvirt>::create(QString(uuidBuffer), name, currentDomain));
    }

    return targetMap;
}

TargetedAutoTypeExecutor* AutoTypeExtLibvirt::createExecutor()
{
    return new AutoTypeExecutorLibvirt(this);
}

QVector<uint> AutoTypeExtLibvirt::charToKeyCodes(const QChar& character, OperatingSystem targetOperatingSystem)
{
    ushort unicodeChar = character.unicode();

    // Numbers are equally mapped to the Win32 keycode table
    if (unicodeChar >= 0x30 && unicodeChar <= 0x39) {
        return {unicodeChar};
    }

    // Upper case letters are equally mapped to the Win32 keycode table, but require a LSHIFT prefix
    if ((unicodeChar >= 0x41 && unicodeChar <= 0x5a)) {
        return {0xa0, unicodeChar};
    }

    // Lowercase letters need a -0x20 offset
    if ((unicodeChar >= 0x61 && unicodeChar <= 0x7a)) {
        return {unicodeChar - 0x20u};
    }

    QVector<uint> keyCodes;

    if (m_lowerSymbolKeyMapping.contains(unicodeChar)) {
        keyCodes = {m_lowerSymbolKeyMapping[unicodeChar]};
    }

    if (m_upperSymbolKeyMapping.contains(unicodeChar)) {
        // prepend LSHIFT
        keyCodes = {0xa0, m_upperSymbolKeyMapping[unicodeChar]};
    }

    if (keyCodes.empty()) {
        return {};
    }

    if (shouldHandleDeadKeys(targetOperatingSystem) && m_deadKeys.contains(unicodeChar)) {
        // append space
        keyCodes += 0x20;
    }

    return keyCodes;
}

QVector<uint> AutoTypeExtLibvirt::keyToKeyCodes(Qt::Key key)
{
    if (m_keyToKeyCodeMapping.contains(key)) {
        return {m_keyToKeyCodeMapping[key]};
    }
    return {};
}

QVector<uint> AutoTypeExtLibvirt::modifiersToKeyCodes(Qt::KeyboardModifiers modifiers)
{
    QVector<uint> keyCodes;

    for (Qt::KeyboardModifier modifier : m_modifierToKeyCodeMapping.keys()) {
        if (modifiers.testFlag(modifier)) {
            keyCodes += m_modifierToKeyCodeMapping[modifier];
        }
    }

    return keyCodes;
}

void AutoTypeExtLibvirt::sendKeyCodesToTarget(QSharedPointer<AutoTypeTargetLibvirt> target,
                                              QVector<uint> keyCodes)
{
    if (target->getDomain() == nullptr) {
        qWarning("Target has no domain object set");
        return;
    }

    uint keyCodeBuffer[keyCodes.size()];
    for (int i = 0; i < keyCodes.size(); i++) {
        keyCodeBuffer[i] = keyCodes[i];
    }

    virDomainSendKey(target->getDomain(), VIR_KEYCODE_SET_WIN32, 5, keyCodeBuffer, keyCodes.size(), 0);
}

bool AutoTypeExtLibvirt::shouldHandleDeadKeys(OperatingSystem operatingSystem)
{
    if (operatingSystem == OperatingSystem::Windows && config()->get(Config::AutoTypeLibvirtDeadKeysWindows).toBool()) {
        return true;
    }
    if (config()->get(Config::AutoTypeLibvirtDeadKeysOther).toBool()) {
        return true;
    }
    return false;
}

AutoTypeExecutorLibvirt::AutoTypeExecutorLibvirt(AutoTypeExtLibvirt* plugin)
    : m_plugin(plugin)
{
}

AutoTypeAction::Result AutoTypeExecutorLibvirt::execBegin(const AutoTypeBegin* action,
                                                          QSharedPointer<AutoTypeTarget> target)
{
    Q_UNUSED(target);
    Q_UNUSED(action);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorLibvirt::execType(AutoTypeKey* action, QSharedPointer<AutoTypeTarget> target)
{
    auto libvirtTarget = target.staticCast<AutoTypeTargetLibvirt>();

    QVector<uint> keycodes = m_plugin->modifiersToKeyCodes(action->modifiers);

    if (action->key != Qt::Key_unknown) {
        keycodes += m_plugin->keyToKeyCodes(action->key);
    } else {
        keycodes += m_plugin->charToKeyCodes(action->character, libvirtTarget->getOperatingSystem());
    }

    m_plugin->sendKeyCodesToTarget(libvirtTarget, keycodes);

    Tools::sleep(execDelayMs);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorLibvirt::execClearField(AutoTypeClearField* action,
                                                               QSharedPointer<AutoTypeTarget> target)
{
    Q_UNUSED(action);
    execType(new AutoTypeKey(Qt::Key_Home, Qt::ControlModifier), target);
    execType(new AutoTypeKey(Qt::Key_End, Qt::ControlModifier | Qt::ShiftModifier), target);
    execType(new AutoTypeKey(Qt::Key_Backspace), target);

    return AutoTypeAction::Result::Ok();
}

AutoTypeTargetLibvirt::AutoTypeTargetLibvirt(QString identifier, QString presentableName, virDomainPtr domain)
    : AutoTypeTarget(std::move(identifier), std::move(presentableName))
    , m_domain(domain)
{
    m_operatingSystem = detectOperatingSystem();
}

AutoTypeTargetLibvirt::~AutoTypeTargetLibvirt()
{
    if (m_domain != nullptr) {
        virDomainFree(m_domain);
    }
}

virDomainPtr AutoTypeTargetLibvirt::getDomain()
{
    return m_domain;
}

OperatingSystem AutoTypeTargetLibvirt::getOperatingSystem()
{
    return m_operatingSystem;
}

OperatingSystem AutoTypeTargetLibvirt::detectOperatingSystem()
{
    if (m_domain == nullptr) {
        qWarning("No domain set in AutoTypeTargetLibvirt object");
        return OperatingSystem::Unknown;
    }

    // TODO: "os.id" values are mostly educated guesses.
    // Confirmed os.id values:
    //  - ubuntu

    int guestInfoParamCount = 0;
    virTypedParameterPtr guestInfoParams = nullptr;

    // Primary attempt through qemu-guest-agent provided data (if available)

    if (virDomainGetGuestInfo(m_domain, VIR_DOMAIN_GUEST_INFO_OS, &guestInfoParams, &guestInfoParamCount, 0) >= 0) {
        virTypedParameterPtr osInfoParam = virTypedParamsGet(guestInfoParams, guestInfoParamCount, "os.id");

        if (osInfoParam != nullptr) {
            QString osName = QString(osInfoParam->field);

            virTypedParamsFree(guestInfoParams, guestInfoParamCount);

            if (osName == "windows") {
                return OperatingSystem::Windows;
            } else if (osName == "macosx") {
                return OperatingSystem::MacOSX;
            } else if (osName.contains(m_patternLinuxOperatingSystemId)) {
                return OperatingSystem::Linux;
            } else {
                return OperatingSystem::Unknown;
            }
        }
    }

    virTypedParamsFree(guestInfoParams, guestInfoParamCount);

    // Fallback using metadata in the domain XML

    QString osInfoXml = QString(virDomainGetMetadata(
        m_domain,
        VIR_DOMAIN_METADATA_ELEMENT,
        "http://libosinfo.org/xmlns/libvirt/domain/1.0",
        0
    ));

    if (osInfoXml.contains("http://microsoft.com")) {
        return OperatingSystem::Windows;
    } else if (osInfoXml.contains("http://apple.com")) {
        return OperatingSystem::MacOSX;
    } else if (osInfoXml.contains(m_patternLinuxOperatingSystemUrl)) {
        return OperatingSystem::Linux;
    }

    return OperatingSystem::Unknown;
}
