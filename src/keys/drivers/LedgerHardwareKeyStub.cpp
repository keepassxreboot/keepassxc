#include "LedgerHardwareKey.h"
#include "core/AsyncTask.h"

#include <memory>

static std::unique_ptr<LedgerHardwareKey> Instance_;

LedgerHardwareKey::LedgerHardwareKey() = default;
LedgerHardwareKey::~LedgerHardwareKey() = default;

LedgerHardwareKey& LedgerHardwareKey::instance()
{
    if (!Instance_) {
        Instance_.reset(new LedgerHardwareKey{});
    }
    return *Instance_;
}

QFuture<bool> LedgerHardwareKey::findFirstDevice()
{
    return QtConcurrent::run([] { return false; });
}

bool LedgerHardwareKey::isInitialized()
{
    return false;
}

bool LedgerHardwareKey::fromDeviceSlot(unsigned, uint8_t*, const size_t)
{
    return false;
}

bool LedgerHardwareKey::fromDeviceDeriveName(QString const&, uint8_t*, const size_t)
{
    return false;
}

QVector<uint8_t> LedgerHardwareKey::getValidKeySlots()
{
    return {};
}

void LedgerHardwareKey::fillValidKeySlots()
{
}

QString LedgerHardwareKey::protocolErrorMsg(uint8_t, uint8_t)
{
    return {};
}

size_t LedgerHardwareKey::maxNameSize()
{
    return 0;
}
