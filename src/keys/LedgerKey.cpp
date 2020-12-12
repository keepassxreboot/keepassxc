#include <QtConcurrent>

#include "core/AsyncTask.h"
#include "core/Tools.h"

#include <algorithm>
#include <cstring>

#include <sodium.h>

#include "LedgerKey.h"
#include "drivers/LedgerHardwareKey.h"

QUuid LedgerKey::UUID("4f998a1e-10a3-476c-81f1-77d1faafc59c");

LedgerKey::LedgerKey()
    : Key(UUID)
{
}

LedgerKey::~LedgerKey()
{
    sodium_memzero(RawKey_, sizeof(RawKey_));
}

QByteArray LedgerKey::rawKey() const
{
    return QByteArray::fromRawData(reinterpret_cast<const char*>(RawKey_), sizeof(RawKey_));
}

QSharedPointer<LedgerKey> LedgerKey::fromDeviceSlot(unsigned Slot)
{
    auto Ret = QSharedPointer<LedgerKey>::create();
    if (!Ret) {
        return Ret;
    }
    bool Res = AsyncTask::runAndWaitForFuture(
        [&] { return LedgerHardwareKey::instance().fromDeviceSlot(Slot, Ret->RawKey_, sizeof(Ret->RawKey_)); });
    if (!Res) {
        return {};
    }
    return Ret;
}

QSharedPointer<LedgerKey> LedgerKey::fromDeviceDeriveName(QString const& Name)
{
    auto Key = QSharedPointer<LedgerKey>::create();
    if (!Key) {
        return Key;
    }
    bool Res = AsyncTask::runAndWaitForFuture(
        [&] { return LedgerHardwareKey::instance().fromDeviceDeriveName(Name, Key->RawKey_, sizeof(Key->RawKey_)); });
    if (!Res) {
        return {};
    }
    return Key;
}
