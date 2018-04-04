#ifndef KEEPASSX_TOUCHID_H
#define KEEPASSX_TOUCHID_H

#define TOUCHID_UNDEFINED -1
#define TOUCHID_AVAILABLE 1
#define TOUCHID_NOT_AVAILABLE 0

#include <QHash>
#include <QString>
#include <QByteArray>
#include <QSharedPointer>

class TouchID
{
    public:
        static TouchID& getInstance();

    private:
        TouchID() {} // Constructor? (the {} brackets) are needed here.

        // C++ 03
        // ========
        // Don't forget to declare these two. You want to make sure they
        // are unacceptable otherwise you may accidentally get copies of
        // your singleton appearing.

        // TouchID(TouchID const&); // Don't Implement
        // void operator=(TouchID const&); // Don't implement

        QHash<QString, QByteArray> m_encryptedMasterKeys;
        int m_available = TOUCHID_UNDEFINED;

    public:
        // C++ 11
        // =======
        // We can use the better technique of deleting the methods
        // we don't want.

        TouchID(TouchID const&) = delete;
        void operator=(TouchID const&) = delete;

        // Note: Scott Meyers mentions in his Effective Modern
        //       C++ book, that deleted functions should generally
        //       be public as it results in better error messages
        //       due to the compilers behavior to check accessibility
        //       before deleted status

        bool storeKey(const QString& databasePath, const QByteArray& passwordKey);
        QSharedPointer<QByteArray> getKey(const QString& databasePath) const;
        bool isAvailable();
        bool authenticate(const QString& message = "") const;
        void reset(const QString& databasePath = "");
};

#endif // KEEPASSX_TOUCHID_H
