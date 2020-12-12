#include "LedgerHardwareKey.h"
#include "core/AsyncTask.h"

#include <memory>

#include <kpl/kpl.h>
#include <kpl/ledger_device.h>

static std::unique_ptr<LedgerHardwareKey> Instance_;

LedgerHardwareKey::LedgerHardwareKey()
    : Mutex_(QMutex::Recursive)
{
}

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
    return QtConcurrent::run([&] {
        if (!Mutex_.tryLock(1000)) {
            emit detectComplete(NotFound, 0, 0);
            return false;
        }
        Dev_ = kpl::LedgerDevice::getFirstDevice();
        int Res = NotFound;
        int App = 0;
        int Lib = 0;
        if (Dev_) {
          kpl::Version AppVer;
          auto EKPL = kpl::KPL::fromDevice(*Dev_, AppVer);
          if (!EKPL) {
              Res = (EKPL.errorValue() == kpl::Result::BAD_PROTOCOL_VERSION) ? ProtocolMismatch : NotFound;
              App = AppVer.protocol();
              Lib = kpl::Version::lib().protocol();
          }
          else {
            KPL_.reset(new kpl::KPL{std::move(EKPL.get())});
            fillValidKeySlots();
            Res = Found;
          }
        }
        emit detectComplete(Res, App, Lib);
        Mutex_.unlock();
        return Res == Found;
    });
}

bool LedgerHardwareKey::isInitialized()
{
    return KPL_.get() != nullptr;
}

bool LedgerHardwareKey::fromDeviceSlot(unsigned Slot, uint8_t* Key, const size_t KeyLen)
{
    assert(isInitialized());
    if (!Mutex_.tryLock(1000)) {
        return false;
    }
    emit userInteractionRequest();
    auto Res = KPL_->getKey(Slot, Key, KeyLen);
    const bool Ret = (Res == kpl::Result::SUCCESS);
    emit userInteractionDone(Ret);
    Mutex_.unlock();
    return Ret;
}

bool LedgerHardwareKey::fromDeviceDeriveName(QString const& Name, uint8_t* Key, const size_t KeyLen)
{
    assert(isInitialized());
    if (!Mutex_.tryLock(1000)) {
        return false;
    }
    emit userInteractionRequest();
    QByteArray NameUtf8 = Name.toUtf8();
    auto Res = KPL_->getKeyFromName(NameUtf8.constData(), Key, KeyLen);
    const bool Ret = (Res == kpl::Result::SUCCESS);
    emit userInteractionDone(Ret);
    Mutex_.unlock();
    return Ret;
}

QVector<uint8_t> LedgerHardwareKey::getValidKeySlots()
{
    QVector<uint8_t> Ret;
    if (!Mutex_.tryLock(1000)) {
        return Ret;
    }
    Ret = Slots_;
    Mutex_.unlock();
    return Ret;
}

void LedgerHardwareKey::fillValidKeySlots()
{
    assert(isInitialized());
    std::vector<uint8_t> Slots;
    const auto Res = KPL_->getValidKeySlots(Slots);
    if (Res == kpl::Result::SUCCESS) {
        Slots_ = QVector<uint8_t>{Slots.begin(), Slots.end()};
    } else {
        Slots_.clear();
    }
}

QString LedgerHardwareKey::protocolErrorMsg(uint8_t appProto, uint8_t libProto)
{
    QString Msg = QString(tr("Ledger hardware key found, but protocol version mismatch: "
                             "device application supports version %1, current software supports version %2. "))
                      .arg(appProto)
                      .arg(libProto);
    if (appProto < libProto) {
        Msg += QString(tr("You should update the application running on your device with Ledger Live."));
    } else {
        Msg += QString(tr("You should update KeePassXC."));
    }
    return Msg;
}

size_t LedgerHardwareKey::maxNameSize()
{
    return kpl::KPL::maxNameSize();
}
