class QJsonObject;
class QLocalSocket;
class QString;

class SafariWebExtensionHelper {

public:
    static SafariWebExtensionHelper* instance();

    bool isSafariWebExtension(QLocalSocket* socket);
    void broadcastClientMessage(const QString& reply);

private:
    SafariWebExtensionHelper() = default;
    SafariWebExtensionHelper(const SafariWebExtensionHelper&) = delete;
    SafariWebExtensionHelper& operator=(const SafariWebExtensionHelper&) = delete;
};

inline SafariWebExtensionHelper* safariWebExtensionHelper()
{
    return SafariWebExtensionHelper::instance();
}
