#ifndef KEEPASSX_TOUCHID_H
#define KEEPASSX_TOUCHID_H

#define TOUCHID_UNDEFINED -1
#define TOUCHID_AVAILABLE 1
#define TOUCHID_NOT_AVAILABLE 0

#include <QHash>

class TouchID
{
public:
    static TouchID& getInstance();

private:
    TouchID()
    {
    }

    // TouchID(TouchID const&); // Don't Implement
    // void operator=(TouchID const&); // Don't implement

    QHash<QString, QByteArray> m_encryptedMasterKeys;
    int m_available = TOUCHID_UNDEFINED;

public:
    TouchID(TouchID const&) = delete;

    void operator=(TouchID const&) = delete;

    bool storeKey(const QString& databasePath, const QByteArray& passwordKey);

    bool getKey(const QString& databasePath, QByteArray& passwordKey) const;

    bool isAvailable();

    bool authenticate(const QString& message = "") const;

    void reset(const QString& databasePath = "");
};

#endif // KEEPASSX_TOUCHID_H
