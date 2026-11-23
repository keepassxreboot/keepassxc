/*
 *  Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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

#include "YubiKeyInterfaceTPM.h"

#include "config-keepassx.h"

#ifdef KPXC_FEATURE_TPM2
#include <QScopeGuard>

#include <cstring>

#include <tss2/tss2_esys.h>
#include <tss2/tss2_tctildr.h>
#endif

YubiKeyInterfaceTPM* YubiKeyInterfaceTPM::m_instance(nullptr);

YubiKeyInterfaceTPM* YubiKeyInterfaceTPM::instance()
{
    if (!m_instance) {
        m_instance = new YubiKeyInterfaceTPM();
    }
    return m_instance;
}

YubiKeyInterfaceTPM::YubiKeyInterfaceTPM()
    : YubiKeyInterface()
{
#ifdef KPXC_FEATURE_TPM2
    m_initialized = true;
#endif
}

/**
 * Build the byte string that is fed to HMAC-SHA1 for a challenge.
 * Always PKCS7-pads to 64 bytes (identical to the USB/PCSC drivers). In
 * variable-length mode the trailing run of bytes equal to the final byte is
 * removed, which is what a YubiKey does when HMAC_LT64 is set on the slot.
 */
QByteArray YubiKeyInterfaceTPM::buildMessage(const QByteArray& challenge, int mode)
{
    QByteArray msg = challenge.left(64);
    const int padLen = 64 - msg.size();
    if (padLen > 0) {
        msg.append(QByteArray(padLen, static_cast<char>(padLen)));
    }

    if (mode == YubiKey::TPM_MODE_VARIABLE) {
        const char last = msg.at(msg.size() - 1);
        int len = msg.size();
        while (len > 0 && msg.at(len - 1) == last) {
            --len;
        }
        msg.truncate(len);
    }
    return msg;
}

#ifdef KPXC_FEATURE_TPM2

namespace
{
    // Owns the ESYS context and, when one was requested through the environment, the TCTI
    struct EsysHandle
    {
        ESYS_CONTEXT* ctx = nullptr;
        TSS2_TCTI_CONTEXT* tcti = nullptr;

        EsysHandle() = default;
        EsysHandle(const EsysHandle&) = delete;
        EsysHandle& operator=(const EsysHandle&) = delete;
        ~EsysHandle()
        {
            if (ctx) {
                Esys_Finalize(&ctx);
            }
            if (tcti) {
                Tss2_TctiLdr_Finalize(&tcti);
            }
        }
        ESYS_CONTEXT* get() const
        {
            return ctx;
        }
        explicit operator bool() const
        {
            return ctx != nullptr;
        }
    };

    bool openContext(EsysHandle& h, QString* error)
    {
        // Same precedence as tpm2-tools: TPM2TOOLS_TCTI, then TSS2_TCTI, else tctildr's default search
        // (tabrmd, /dev/tpmrm0, /dev/tpm0)
        QByteArray conf = qgetenv("TPM2TOOLS_TCTI");
        if (conf.isEmpty()) {
            conf = qgetenv("TSS2_TCTI");
        }
        if (!conf.isEmpty()) {
            auto rc = Tss2_TctiLdr_Initialize(conf.constData(), &h.tcti);
            if (rc != TSS2_RC_SUCCESS) {
                if (error) {
                    *error = YubiKeyInterface::tr("Invalid TPM2 TCTI configuration (error 0x%1).").arg(rc, 0, 16);
                }
                return false;
            }
        }

        auto rc = Esys_Initialize(&h.ctx, h.tcti, nullptr);
        if (rc != TSS2_RC_SUCCESS) {
            h.ctx = nullptr;
            if (error) {
                *error = YubiKeyInterface::tr("Could not connect to the TPM (error 0x%1).").arg(rc, 0, 16);
            }
            return false;
        }
        return true;
    }

    // Why a persistent object cannot be used as a challenge-response key
    enum class KeyIssue
    {
        None,
        NotFound, // nothing (readable) at that handle
        NotHmacKey, // some other kind of object
        PolicyBound, // HMAC key with an authorization policy (e.g. PCR values): not supported
        NoPasswordAuth // HMAC key without the userWithAuth attribute: cannot be authorized with a password
    };

    QString describeIssue(KeyIssue issue, uint32_t handle)
    {
        const auto hex = QStringLiteral("0x%1").arg(handle, 8, 16, QLatin1Char('0'));
        switch (issue) {
        case KeyIssue::PolicyBound:
            return YubiKeyInterface::tr("TPM2 key %1 is bound to an authorization policy (for example PCR values), "
                                        "which is not supported yet. Use a key protected by a PIN/password instead.")
                .arg(hex);
        case KeyIssue::NoPasswordAuth:
            return YubiKeyInterface::tr("TPM2 key %1 cannot be used: it was created without the userWithAuth "
                                        "attribute, so it cannot be authorized with a PIN/password.")
                .arg(hex);
        case KeyIssue::NotHmacKey:
            return YubiKeyInterface::tr("TPM2 object %1 is not an HMAC-SHA1 signing key.").arg(hex);
        default:
            return YubiKeyInterface::tr("No TPM2 key found at handle %1.").arg(hex);
        }
    }

    /**
     * Load the persistent object and verify it is a usable HMAC-SHA1 key.
     * @param issue receives the reason if the key cannot be used
     * @return ESYS_TR_NONE on failure
     */
    ESYS_TR loadHmacKey(ESYS_CONTEXT* ctx, uint32_t handle, bool* noDa = nullptr, KeyIssue* issue = nullptr)
    {
        auto fail = [&](KeyIssue reason) {
            if (issue) {
                *issue = reason;
            }
            return ESYS_TR_NONE;
        };

        ESYS_TR obj = ESYS_TR_NONE;
        if (Esys_TR_FromTPMPublic(ctx, handle, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &obj) != TSS2_RC_SUCCESS) {
            return fail(KeyIssue::NotFound);
        }

        TPM2B_PUBLIC* pub = nullptr;
        TPM2B_NAME* name = nullptr;
        TPM2B_NAME* qname = nullptr;
        if (Esys_ReadPublic(ctx, obj, ESYS_TR_NONE, ESYS_TR_NONE, ESYS_TR_NONE, &pub, &name, &qname)
            != TSS2_RC_SUCCESS) {
            Esys_TR_Close(ctx, &obj);
            return fail(KeyIssue::NotFound);
        }
        auto cleanup = qScopeGuard([&] {
            Esys_Free(pub);
            Esys_Free(name);
            Esys_Free(qname);
        });

        const auto& area = pub->publicArea;
        const auto& scheme = area.parameters.keyedHashDetail.scheme;
        const bool schemeOk = scheme.scheme == TPM2_ALG_NULL
                              || (scheme.scheme == TPM2_ALG_HMAC && scheme.details.hmac.hashAlg == TPM2_ALG_SHA1);
        const bool isHmacKey =
            area.type == TPM2_ALG_KEYEDHASH && schemeOk && (area.objectAttributes & TPMA_OBJECT_SIGN_ENCRYPT);
        // Keys with a policy would need a policy session (PCR values, ...), which is not implemented. They are
        // rejected before any authorization is sent, so they can never count as a failed attempt.
        KeyIssue problem = KeyIssue::None;
        if (!isHmacKey) {
            problem = KeyIssue::NotHmacKey;
        } else if (area.authPolicy.size != 0) {
            problem = KeyIssue::PolicyBound;
        } else if (!(area.objectAttributes & TPMA_OBJECT_USERWITHAUTH)) {
            problem = KeyIssue::NoPasswordAuth;
        }
        if (problem != KeyIssue::None) {
            Esys_TR_Close(ctx, &obj);
            return fail(problem);
        }
        if (noDa) {
            *noDa = (area.objectAttributes & TPMA_OBJECT_NODA) != 0;
        }
        return obj;
    }

    struct TpmKey
    {
        ESYS_CONTEXT* ctx;
        ESYS_TR obj;
        const Botan::secure_vector<char>* pin;
    };

    enum class HmacResult
    {
        Ok,
        AuthFailed, // wrong PIN, counted by the TPM's dictionary-attack protection
        AuthFailedNoDa, // wrong PIN, not counted (key has the NODA attribute)
        Lockout,
        Error
    };

    struct LockoutState
    {
        bool valid = false;
        uint32_t counter = 0; // failed tries currently recorded
        uint32_t maxTries = 0; // failures allowed before lockout
        uint32_t interval = 0; // seconds until one failed try is forgiven
    };

    LockoutState readLockoutState(ESYS_CONTEXT* ctx)
    {
        LockoutState st;
        TPMI_YES_NO more = TPM2_NO;
        TPMS_CAPABILITY_DATA* cap = nullptr;
        // LOCKOUT_COUNTER, MAX_AUTH_FAIL, LOCKOUT_INTERVAL and LOCKOUT_RECOVERY are consecutive properties
        auto rc = Esys_GetCapability(ctx,
                                     ESYS_TR_NONE,
                                     ESYS_TR_NONE,
                                     ESYS_TR_NONE,
                                     TPM2_CAP_TPM_PROPERTIES,
                                     TPM2_PT_LOCKOUT_COUNTER,
                                     4,
                                     &more,
                                     &cap);
        if (rc != TSS2_RC_SUCCESS || !cap) {
            return st;
        }
        bool haveCounter = false;
        bool haveMax = false;
        for (uint32_t i = 0; i < cap->data.tpmProperties.count; ++i) {
            const auto& prop = cap->data.tpmProperties.tpmProperty[i];
            switch (prop.property) {
            case TPM2_PT_LOCKOUT_COUNTER:
                st.counter = prop.value;
                haveCounter = true;
                break;
            case TPM2_PT_MAX_AUTH_FAIL:
                st.maxTries = prop.value;
                haveMax = true;
                break;
            case TPM2_PT_LOCKOUT_INTERVAL:
                st.interval = prop.value;
                break;
            default:
                break;
            }
        }
        Esys_Free(cap);
        // A limit of zero means lockout handling is configured unusually; do not make claims about it
        st.valid = haveCounter && haveMax && st.maxTries > 0;
        return st;
    }

    QString durationText(uint32_t seconds)
    {
        if (seconds >= 3600) {
            return YubiKeyInterface::tr("%n hour(s)", nullptr, static_cast<int>((seconds + 1800) / 3600));
        }
        if (seconds >= 60) {
            return YubiKeyInterface::tr("%n minute(s)", nullptr, static_cast<int>((seconds + 30) / 60));
        }
        return YubiKeyInterface::tr("%n second(s)", nullptr, static_cast<int>(seconds));
    }

    QString forgivenessText(const LockoutState& st)
    {
        return st.interval > 0
                   ? YubiKeyInterface::tr(" One failed attempt is forgiven every %1.").arg(durationText(st.interval))
                   : QString();
    }

    // Opening sentence of an authorization failure. Without a PIN the empty password was refused, which usually
    // means the user picked the plain entry for a key that is protected by a PIN/password.
    QString authFailureHeadline(bool hadPin)
    {
        return hadPin ? YubiKeyInterface::tr("Incorrect TPM PIN/password.")
                      : YubiKeyInterface::tr("This TPM2 key did not accept an empty password, so it is probably "
                                             "protected by a PIN/password. Choose the entry marked \"PIN/password\" "
                                             "for this key instead.");
    }

    QString countedFailureMessage(const LockoutState& st, bool hadPin)
    {
        auto msg = authFailureHeadline(hadPin);
        if (!st.valid) {
            return msg + YubiKeyInterface::tr(" The TPM counts failed attempts and may lock out.");
        }
        if (st.counter >= st.maxTries) {
            return msg + YubiKeyInterface::tr(" The TPM is now locked out (%1 of %2 failed attempts).")
                             .arg(st.counter)
                             .arg(st.maxTries)
                   + forgivenessText(st);
        }
        msg += YubiKeyInterface::tr(" The TPM counted this attempt: %1 of %2 failed attempts before lockout.")
                   .arg(st.counter)
                   .arg(st.maxTries);
        if (st.counter + 1 == st.maxTries) {
            msg += YubiKeyInterface::tr(" One more failure will lock the TPM.");
        }
        return msg + forgivenessText(st);
    }

    HmacResult classify(TSS2_RC rc)
    {
        // Strip the ESYS layer, parameter/session indicators and keep format + error code
        const uint32_t code = rc & 0xFFFF;
        const uint32_t fmt1 = code & (TPM2_RC_FMT1 | 0x3F);
        if ((code & TPM2_RC_FMT1) && fmt1 == TPM2_RC_AUTH_FAIL) {
            return HmacResult::AuthFailed;
        }
        if ((code & TPM2_RC_FMT1) && fmt1 == TPM2_RC_BAD_AUTH) {
            return HmacResult::AuthFailedNoDa;
        }
        if (code == TPM2_RC_LOCKOUT) { // a TPM "warning" class code (0x921), no parameter/session bits
            return HmacResult::Lockout;
        }
        return HmacResult::Error;
    }

    HmacResult tpmHmac(const TpmKey& key, const QByteArray& msg, Botan::secure_vector<char>& out, QString* err)
    {
        TPM2B_MAX_BUFFER buf{};
        if (msg.size() > static_cast<int>(sizeof(buf.buffer))) {
            return HmacResult::Error;
        }
        buf.size = msg.size();
        memcpy(buf.buffer, msg.constData(), msg.size());

        // Always set the authValue explicitly (empty when no PIN) so state of the object never leaks across calls
        TPM2B_AUTH auth{};
        const bool hadPin = key.pin && !key.pin->empty();
        if (hadPin) {
            if (key.pin->size() > sizeof(auth.buffer)) {
                if (err) {
                    *err = YubiKeyInterface::tr("The TPM PIN/password is too long.");
                }
                return HmacResult::Error;
            }
            auth.size = key.pin->size();
            memcpy(auth.buffer, key.pin->data(), key.pin->size());
        }
        auto rc = Esys_TR_SetAuth(key.ctx, key.obj, &auth);
        explicit_bzero(auth.buffer, sizeof(auth.buffer));
        if (rc != TSS2_RC_SUCCESS) {
            explicit_bzero(buf.buffer, sizeof(buf.buffer));
            return HmacResult::Error;
        }

        TPM2B_DIGEST* digest = nullptr;
        rc = Esys_HMAC(key.ctx, key.obj, ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE, &buf, TPM2_ALG_SHA1, &digest);
        explicit_bzero(buf.buffer, sizeof(buf.buffer));
        if (rc != TSS2_RC_SUCCESS || !digest) {
            const auto result = classify(rc);
            if (err) {
                switch (result) {
                case HmacResult::AuthFailed:
                    *err = countedFailureMessage(readLockoutState(key.ctx), hadPin);
                    break;
                case HmacResult::AuthFailedNoDa:
                    *err = authFailureHeadline(hadPin)
                           + YubiKeyInterface::tr(" (This key is exempt from TPM lockout.)");
                    break;
                case HmacResult::Lockout: {
                    const auto st = readLockoutState(key.ctx);
                    *err = YubiKeyInterface::tr("The TPM is locked out by dictionary attack protection. "
                                                "Wait and try again later.")
                           + (st.valid ? forgivenessText(st) : QString());
                    break;
                }
                default:
                    *err = YubiKeyInterface::tr("The TPM failed to compute the HMAC (error 0x%1).").arg(rc, 0, 16);
                }
            }
            return result;
        }
        out.assign(digest->buffer, digest->buffer + digest->size);
        explicit_bzero(digest->buffer, sizeof(digest->buffer));
        Esys_Free(digest);
        return out.size() == 20 ? HmacResult::Ok : HmacResult::Error;
    }
} // namespace

YubiKey::KeyMap YubiKeyInterfaceTPM::findValidKeys(int& connectedKeys)
{
    YubiKey::KeyMap keys;
    m_error.clear();
    m_unusableKeysHint.clear();

    QString err;
    EsysHandle ctx;
    if (!openContext(ctx, &err)) {
        qDebug("TPM2: %s", qPrintable(err));
        return keys;
    }

    QList<uint32_t> handles;
    TPMI_YES_NO more = TPM2_YES;
    uint32_t property = TPM2_PERSISTENT_FIRST;
    while (more == TPM2_YES) {
        TPMS_CAPABILITY_DATA* cap = nullptr;
        more = TPM2_NO;
        auto rc = Esys_GetCapability(ctx.get(),
                                     ESYS_TR_NONE,
                                     ESYS_TR_NONE,
                                     ESYS_TR_NONE,
                                     TPM2_CAP_HANDLES,
                                     property,
                                     TPM2_MAX_CAP_HANDLES,
                                     &more,
                                     &cap);
        if (rc != TSS2_RC_SUCCESS || !cap) {
            break;
        }
        for (uint32_t i = 0; i < cap->data.handles.count; ++i) {
            const auto h = cap->data.handles.handle[i];
            if ((h >> 24) == TPM2_HT_PERSISTENT) {
                handles << h;
            }
            property = h + 1;
        }
        Esys_Free(cap);
    }

    int policyBound = 0;
    int noPasswordAuth = 0;
    for (auto handle : handles) {
        bool noDa = false;
        auto issue = KeyIssue::None;
        ESYS_TR obj = loadHmacKey(ctx.get(), handle, &noDa, &issue);
        if (obj == ESYS_TR_NONE) {
            // Other kinds of objects are normal on a TPM; only HMAC keys that cannot be used are worth reporting
            policyBound += issue == KeyIssue::PolicyBound;
            noPasswordAuth += issue == KeyIssue::NoPasswordAuth;
            continue;
        }

        // The TPM does not reveal whether a key has an authValue. Probing with an empty auth is only safe for
        // NODA keys; for others a failed probe would count against dictionary-attack lockout, so offer both.
        bool offerPlain = true;
        bool offerPin = true;
        if (noDa) {
            Botan::secure_vector<char> probe;
            TpmKey tk{ctx.get(), obj, nullptr};
            const auto res = tpmHmac(tk, buildMessage(QByteArray(32, 0), YubiKey::TPM_MODE_FIXED64), probe, nullptr);
            offerPlain = res == HmacResult::Ok;
            offerPin = res == HmacResult::AuthFailed || res == HmacResult::AuthFailedNoDa;
        }
        Esys_TR_Close(ctx.get(), &obj);
        if (!offerPlain && !offerPin) {
            continue;
        }

        ++connectedKeys;
        const auto hex = QStringLiteral("0x%1").arg(handle, 8, 16, QLatin1Char('0'));
        for (int withPin = 0; withPin < 2; ++withPin) {
            if ((withPin && !offerPin) || (!withPin && !offerPlain)) {
                continue;
            }
            const int flags = YubiKey::TPM_SLOT_FLAG | (withPin ? YubiKey::TPM_PIN_FLAG : 0);
            const QString pinText = withPin ? tr(", PIN/password") : QString();
            keys.insert({handle, flags | YubiKey::TPM_MODE_FIXED64},
                        tr("TPM2 %1 - HMAC-SHA1 [Fixed 64-byte%2]").arg(hex, pinText));
            keys.insert({handle, flags | YubiKey::TPM_MODE_VARIABLE},
                        tr("TPM2 %1 - HMAC-SHA1 [Variable length (HMAC_LT64)%2]").arg(hex, pinText));
        }
    }

    QStringList reasons;
    if (policyBound > 0) {
        reasons << tr("keys bound to an authorization policy (for example PCR values) are not supported yet");
    }
    if (noPasswordAuth > 0) {
        reasons << tr("HMAC keys need the userWithAuth attribute");
    }
    if (!reasons.isEmpty()) {
        const auto reasonText = reasons.join(QStringLiteral("; "));
        m_unusableKeysHint = policyBound + noPasswordAuth == 1
                                 ? tr("A TPM2 key was found that cannot be used: %1.").arg(reasonText)
                                 : tr("TPM2 keys were found that cannot be used: %1.").arg(reasonText);
        qDebug("TPM2: %s", qPrintable(m_unusableKeysHint));
    }

    return keys;
}

QString YubiKeyInterfaceTPM::unusableKeysHint() const
{
    return m_unusableKeysHint;
}

YubiKey::ChallengeResult
YubiKeyInterfaceTPM::challenge(YubiKeySlot slot, const QByteArray& challenge, Botan::secure_vector<char>& response)
{
    return challengeWithPin(slot, challenge, response, nullptr);
}

YubiKey::ChallengeResult YubiKeyInterfaceTPM::challengeWithPin(YubiKeySlot slot,
                                                               const QByteArray& challenge,
                                                               Botan::secure_vector<char>& response,
                                                               const Botan::secure_vector<char>* pin)
{
    m_error.clear();

    // A PIN slot used without a PIN would send an empty password, which the TPM counts as a failed attempt
    // against its dictionary-attack protection. Refuse before the TPM is even contacted.
    if (YubiKey::tpmSlotNeedsPin(slot) && (!pin || pin->empty())) {
        m_error = tr("A PIN/password is required for this TPM2 key.");
        return YubiKey::ChallengeResult::YCR_ERROR;
    }

    EsysHandle ctx;
    if (!openContext(ctx, &m_error)) {
        return YubiKey::ChallengeResult::YCR_ERROR;
    }
    auto issue = KeyIssue::None;
    ESYS_TR obj = loadHmacKey(ctx.get(), slot.first, nullptr, &issue);
    if (obj == ESYS_TR_NONE) {
        m_error = describeIssue(issue, slot.first);
        return YubiKey::ChallengeResult::YCR_KEYNOTFOUND;
    }
    TpmKey tk{ctx.get(), obj, YubiKey::tpmSlotNeedsPin(slot) ? pin : nullptr};
    emit challengeStarted();
    auto ret = performChallenge(&tk, slot.second, true, challenge, response);
    emit challengeCompleted();
    Esys_TR_Close(ctx.get(), &obj);
    return ret;
}

YubiKey::ChallengeResult YubiKeyInterfaceTPM::performChallenge(
    void* key, int slot, bool, const QByteArray& challenge, Botan::secure_vector<char>& response)
{
    auto* tk = static_cast<TpmKey*>(key);
    const int mode = slot & YubiKey::TPM_MODE_MASK;
    if (mode != YubiKey::TPM_MODE_FIXED64 && mode != YubiKey::TPM_MODE_VARIABLE) {
        m_error = tr("Invalid TPM2 key mode.");
        return YubiKey::ChallengeResult::YCR_ERROR;
    }

    auto msg = buildMessage(challenge, mode);
    const auto res = tpmHmac(*tk, msg, response, &m_error);
    if (!msg.isEmpty()) {
        explicit_bzero(msg.data(), msg.size());
    }
    if (res != HmacResult::Ok) {
        response.clear();
        if (res == HmacResult::AuthFailed || res == HmacResult::AuthFailedNoDa) {
            return YubiKey::ChallengeResult::YCR_AUTHFAILED;
        }
        if (m_error.isEmpty()) {
            m_error = tr("The TPM returned an unexpected response.");
        }
        return YubiKey::ChallengeResult::YCR_ERROR;
    }
    return YubiKey::ChallengeResult::YCR_SUCCESS;
}

bool YubiKeyInterfaceTPM::performTestChallenge(void* key, int slot, bool* wouldBlock)
{
    if (wouldBlock) {
        *wouldBlock = false;
    }
    Botan::secure_vector<char> resp;
    return performChallenge(key, slot, false, QByteArray(32, 0), resp) == YubiKey::ChallengeResult::YCR_SUCCESS;
}

QString YubiKeyInterfaceTPM::pinNotice(YubiKeySlot slot)
{
    EsysHandle ctx;
    QString ignored;
    if (!openContext(ctx, &ignored)) {
        return {};
    }

    bool noDa = false;
    ESYS_TR obj = loadHmacKey(ctx.get(), slot.first, &noDa);
    if (obj == ESYS_TR_NONE) {
        return {};
    }
    Esys_TR_Close(ctx.get(), &obj);
    if (noDa) {
        return {}; // failures on this key are never counted
    }

    const auto st = readLockoutState(ctx.get());
    if (!st.valid || st.counter == 0) {
        return {};
    }
    if (st.counter >= st.maxTries) {
        return tr("The TPM is currently locked out by dictionary attack protection; entering the PIN will fail "
                  "until it recovers.")
               + forgivenessText(st);
    }
    if (st.counter + 1 == st.maxTries) {
        return tr("Warning: the TPM has recorded %1 of %2 failed attempts. One more failure will lock it.")
                   .arg(st.counter)
                   .arg(st.maxTries)
               + forgivenessText(st);
    }
    return tr("The TPM has already recorded %1 of %2 failed attempts before lockout.")
               .arg(st.counter)
               .arg(st.maxTries)
           + forgivenessText(st);
}

bool YubiKeyInterfaceTPM::testChallenge(YubiKeySlot slot, bool* wouldBlock)
{
    return testChallengeWithPin(slot, wouldBlock, nullptr);
}

bool YubiKeyInterfaceTPM::testChallengeWithPin(YubiKeySlot slot,
                                              bool* wouldBlock,
                                              const Botan::secure_vector<char>* pin,
                                              YubiKey::ChallengeResult* result)
{
    if (wouldBlock) {
        *wouldBlock = false;
    }
    Botan::secure_vector<char> resp;
    const auto ret = challengeWithPin(slot, QByteArray(32, 0), resp, pin);
    if (result) {
        *result = ret;
    }
    return ret == YubiKey::ChallengeResult::YCR_SUCCESS;
}

#else // !KPXC_FEATURE_TPM2

YubiKey::KeyMap YubiKeyInterfaceTPM::findValidKeys(int&)
{
    return {};
}

YubiKey::ChallengeResult YubiKeyInterfaceTPM::challenge(YubiKeySlot, const QByteArray&, Botan::secure_vector<char>&)
{
    return YubiKey::ChallengeResult::YCR_KEYNOTFOUND;
}

bool YubiKeyInterfaceTPM::testChallenge(YubiKeySlot, bool* wouldBlock)
{
    if (wouldBlock) {
        *wouldBlock = false;
    }
    return false;
}

QString YubiKeyInterfaceTPM::pinNotice(YubiKeySlot)
{
    return {};
}

QString YubiKeyInterfaceTPM::unusableKeysHint() const
{
    return {};
}

YubiKey::ChallengeResult YubiKeyInterfaceTPM::challengeWithPin(YubiKeySlot,
                                                               const QByteArray&,
                                                               Botan::secure_vector<char>&,
                                                               const Botan::secure_vector<char>*)
{
    return YubiKey::ChallengeResult::YCR_KEYNOTFOUND;
}

bool YubiKeyInterfaceTPM::testChallengeWithPin(YubiKeySlot,
                                              bool*,
                                              const Botan::secure_vector<char>*,
                                              YubiKey::ChallengeResult* result)
{
    if (result) {
        *result = YubiKey::ChallengeResult::YCR_KEYNOTFOUND;
    }
    return false;
}

YubiKey::ChallengeResult
YubiKeyInterfaceTPM::performChallenge(void*, int, bool, const QByteArray&, Botan::secure_vector<char>&)
{
    return YubiKey::ChallengeResult::YCR_ERROR;
}

bool YubiKeyInterfaceTPM::performTestChallenge(void*, int, bool*)
{
    return false;
}

#endif
