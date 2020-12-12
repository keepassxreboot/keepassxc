#ifndef KEEPASSX_LEDGERKEY_H
#define KEEPASSX_LEDGERKEY_H

#include <QSharedPointer>
#include <QString>

#include "keys/Key.h"

namespace kpl
{
    class KPL;
} // namespace kpl

class LedgerKey : public Key
{
public:
    static QUuid UUID;

    LedgerKey();
    explicit LedgerKey(const QString& password);
    ~LedgerKey() override;
    QByteArray rawKey() const override;

    static QSharedPointer<LedgerKey> fromDeviceSlot(unsigned Slot);
    static QSharedPointer<LedgerKey> fromDeviceDeriveName(QString const& Name);

private:
    uint8_t RawKey_[32];
    bool Valid_ = false;
};

#endif // KEEPASSX_LEDGERKEY_H
