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

void AutoTypeExtLibvirt::sendKeyCodesToTarget(const QSharedPointer<AutoTypeTargetLibvirt>& target, QList<uint> keyCodes)
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

AutoTypeExecutorLibvirt::AutoTypeExecutorLibvirt(AutoTypeExtLibvirt* plugin)
    : m_plugin(plugin)
{
}

QList<uint> AutoTypeExtLibvirt::charToKeyCodeGroup(const QChar& character, OperatingSystem targetOperatingSystem)
{
    ushort uCharacter = character.unicode();

    // Numbers are equally mapped to the Win32 keycode table
    if (uCharacter >= 0x30 && uCharacter <= 0x39) {
        return {uCharacter};
    }

    // Upper case letters are equally mapped to the Win32 keycode table, but require a LSHIFT prefix
    if ((uCharacter >= 0x41 && uCharacter <= 0x5a)) {
        return {0xa0, uCharacter};
    }

    // Lowercase letters need a -0x20 offset
    if ((uCharacter >= 0x61 && uCharacter <= 0x7a)) {
        return {uCharacter - 0x20u};
    }

    QMap<uint, uint> specialCharWithoutShiftMapping(
        {
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
        }
    );

    QMap<uint, uint> specialCharWithShiftMapping(
        {
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
        }
    );

    QList<uint> keyCodes;

    if (specialCharWithoutShiftMapping.contains(uCharacter)) {
        keyCodes = {specialCharWithoutShiftMapping[uCharacter]};
    }

    if (specialCharWithShiftMapping.contains(uCharacter)) {
        // prepend LSHIFT
        keyCodes = {0xa0, specialCharWithShiftMapping[uCharacter]};
    }

    if (keyCodes.empty()) {
        return {};
    }

    QList<uint> deadKeys = {
        0x27, // '
        0x22, // "
        0x5e, // ^
        0x60, // `
        0x7e, // ~
    };

    bool shouldHandleDeadKeys = false;

    if (targetOperatingSystem == OperatingSystem::Windows) {
        if (config()->get(Config::AutoTypeLibvirtDeadKeysWindows).toBool()) {
            shouldHandleDeadKeys = true;
        }
    } else {
        if (config()->get(Config::AutoTypeLibvirtDeadKeysOther).toBool()) {
            shouldHandleDeadKeys = true;
        }
    }

    if (shouldHandleDeadKeys && deadKeys.contains(uCharacter)) {
        // append space
        keyCodes.append(0x20);
    }

    return keyCodes;
}

uint AutoTypeExtLibvirt::keyToKeyCode(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Tab:
        return 0x09;
    case Qt::Key_Enter:
        return 0x0d;
    case Qt::Key_Space:
        return 0x20;
    case Qt::Key_Left:
        return 0x25;
    case Qt::Key_Up:
        return 0x26;
    case Qt::Key_Right:
        return 0x27;
    case Qt::Key_Down:
        return 0x28;
    case Qt::Key_Insert:
        return 0x2d;
    case Qt::Key_Delete:
        return 0x2e;
    case Qt::Key_Home:
        return 0x24;
    case Qt::Key_End:
        return 0x23;
    case Qt::Key_PageUp:
        return 0x21;
    case Qt::Key_PageDown:
        return 0x22;
    case Qt::Key_Backspace:
        return 0x08;
    case Qt::Key_Pause:
        return 0x13;
    case Qt::Key_CapsLock:
        return 0x14;
    case Qt::Key_Escape:
        return 0x1b;
    case Qt::Key_Help:
        return 0x2f;
    case Qt::Key_NumLock:
        return 0x90;
    case Qt::Key_Print:
        return 0x2a;
    case Qt::Key_ScrollLock:
        return 0x91;
    case Qt::Key_Shift:
        return 0xa0;
    case Qt::Key_Control:
        return 0xa2;
    case Qt::Key_Alt:
        return 0xa4;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F16) {
            return 0x70 + (key - Qt::Key_F1);
        } else {
            return 0;
        }
    }
}

AutoTypeAction::Result AutoTypeExecutorLibvirt::execBegin(const QSharedPointer<AutoTypeTarget>& target,
                                                          const AutoTypeBegin* action)
{
    Q_UNUSED(target);
    Q_UNUSED(action);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorLibvirt::execType(const QSharedPointer<AutoTypeTarget>& target,
                                                         AutoTypeKey* action)
{
    QList<uint> keyCodeGroups;
    auto libvirtTarget = target.staticCast<AutoTypeTargetLibvirt>();

    if (action->key != Qt::Key_unknown) {
        keyCodeGroups = {m_plugin->keyToKeyCode(action->key)};
    } else {
        keyCodeGroups = m_plugin->charToKeyCodeGroup(action->character, libvirtTarget->getOperatingSystem());
    }

    m_plugin->sendKeyCodesToTarget(libvirtTarget, keyCodeGroups);

    Tools::sleep(execDelayMs);
    return AutoTypeAction::Result::Ok();
}

AutoTypeAction::Result AutoTypeExecutorLibvirt::execClearField(const QSharedPointer<AutoTypeTarget>& target,
                                                               AutoTypeClearField* action)
{
    Q_UNUSED(action);

    timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 25 * 1000 * 1000;

    auto libvirtTarget = target.staticCast<AutoTypeTargetLibvirt>();

    m_plugin->sendKeyCodesToTarget(libvirtTarget, {m_plugin->keyToKeyCode(Qt::Key_Home)});
    nanosleep(&ts, nullptr);

    m_plugin->sendKeyCodesToTarget(
        libvirtTarget,
        {m_plugin->keyToKeyCode(Qt::Key_Shift), m_plugin->keyToKeyCode(Qt::Key_End)}
    );
    nanosleep(&ts, nullptr);

    m_plugin->sendKeyCodesToTarget(libvirtTarget, {m_plugin->keyToKeyCode(Qt::Key_Backspace)});
    nanosleep(&ts, nullptr);

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

    QList<QString> linuxBasedOperatingSystems = QList<QString>(
        {
           "ubuntu",
           "debian",
           "centos",
           "fedora",
           "redhat",
           "opensuse",
           "archlinux",
           "alpinelinux",
           "popos"
       }
    );

    int guestInfoParamCount = 0;
    virTypedParameterPtr guestInfoParams = nullptr;

    if (virDomainGetGuestInfo(m_domain, VIR_DOMAIN_GUEST_INFO_OS, &guestInfoParams, &guestInfoParamCount, 0) >= 0) {
        virTypedParameterPtr osInfoParam = virTypedParamsGet(guestInfoParams, guestInfoParamCount, "os.id");

        if (osInfoParam != nullptr) {
            QString osName = QString(osInfoParam->field);

            virTypedParamsFree(guestInfoParams, guestInfoParamCount);

            if (osName == "windows") {
                return OperatingSystem::Windows;
            } else if (osName == "macosx") {
                return OperatingSystem::MacOSX;
            } else if (linuxBasedOperatingSystems.contains(osName)) {
                return OperatingSystem::Linux;
            } else {
                return OperatingSystem::Unknown;
            }
        }
    }

    virTypedParamsFree(guestInfoParams, guestInfoParamCount);

    QString osInfoXml = QString(virDomainGetMetadata(
        m_domain,
        VIR_DOMAIN_METADATA_ELEMENT,
        "http://libosinfo.org/xmlns/libvirt/domain/1.0",
        0
    ));

    QRegExp osPattern = QRegExp("http://(ubuntu|debian|centos|fedora|redhat|opensuse|archlinux|alpinelinux|system76)");

    if (osInfoXml.contains("http://microsoft.com")) {
        return OperatingSystem::Windows;
    } else if (osInfoXml.contains("http://apple.com")) {
        return OperatingSystem::MacOSX;
    } else if (osInfoXml.contains(osPattern)) {
        return OperatingSystem::Linux;
    }

    return OperatingSystem::Unknown;
}
