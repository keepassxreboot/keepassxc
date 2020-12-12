#ifndef KEEPASSX_LEDGER_HARDWARE_H
#define KEEPASSX_LEDGER_HARDWARE_H

#include <QFuture>
#include <QMutex>
#include <QObject>
#include <QVector>

#include "config-keepassx.h"

#ifdef WITH_XC_LEDGER
namespace kpl
{
    class KPL;
    class LedgerDevice;
}
#endif

/**
 * Singleton class to manage the interface to hardware Ledger key(s)
 */
class LedgerHardwareKey : public QObject
{
    Q_OBJECT

public:
    enum DetectResult
    {
        Found = 0,
        NotFound = -1,
        ProtocolMismatch = -2
    };

    ~LedgerHardwareKey() override;
    static LedgerHardwareKey& instance();
    QFuture<bool> findFirstDevice();

    bool isInitialized();

    bool fromDeviceSlot(unsigned Slot, uint8_t* Key, const size_t KeyLen);
    bool fromDeviceDeriveName(QString const& Name, uint8_t* Key, const size_t KeyLen);

    QVector<uint8_t> getValidKeySlots();

    static QString protocolErrorMsg(uint8_t appProto, uint8_t libProto);
    static size_t maxNameSize();

signals:
    // Res is of type DetectResult
    void detectComplete(int Res, int AppProtocol, int LibProtocol);
    void userInteractionRequest();
    void userInteractionDone(bool Accepted);

private:
    explicit LedgerHardwareKey();
    void fillValidKeySlots();

#ifdef WITH_XC_LEDGER
    QVector<uint8_t> Slots_;
    QMutex Mutex_;
    std::unique_ptr<kpl::KPL> KPL_;
    std::unique_ptr<kpl::LedgerDevice> Dev_;
#endif
};

#endif // KEEPASSX_YUBIKEY_H
