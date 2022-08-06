#ifndef KEEPASSX_TOUCHID_H
#define KEEPASSX_TOUCHID_H

#include <QHash>

class TouchID
{
public:
    static TouchID& getInstance();

private:
    TouchID()
    {
       // Nothing to do here
    }

public:
    TouchID(TouchID const&) = delete;
    void operator=(TouchID const&) = delete;

    bool storeKey(const QString& databasePath, const QByteArray& passwordKey);
    bool getKey(const QString& databasePath, QByteArray& passwordKey) const;
    bool containsKey(const QString& databasePath) const;
    void reset(const QString& databasePath = "");

    bool isAvailable();

private:
    static bool isWatchAvailable();
    static bool isTouchIdAvailable();

    static void deleteKeyEntry(const QString& accountName);
    static QString databaseKeyName(const QString& databasePath);

private:
    QHash<QString, QByteArray> m_encryptedMasterKeys;
};

#endif // KEEPASSX_TOUCHID_H
