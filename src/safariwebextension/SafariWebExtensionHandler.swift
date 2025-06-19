import SafariServices
import os.log

// SEB_TODO: Convert this to Objective C++ (maybe)
class SafariWebExtensionHandler: NSObject, NSExtensionRequestHandling {
    let SocketFileName = "KeePassXC.BrowserServer"
    let applicationGroupIdentifier =
        Bundle.main.object(forInfoDictionaryKey: "APP_GROUP_IDENTIFIER") as! String
    static var socketFD: Int32 = -1
    static var socketConnected = false
    var maxMessageLength: Int32 = 1024 * 1024

    private var logger = Logger(
        subsystem: Bundle.main.bundleIdentifier!,
        category: String(describing: SafariWebExtensionHandler.self)
    )

    var socketPath: String? {
        FileManager.default.containerURL(
            forSecurityApplicationGroupIdentifier: applicationGroupIdentifier)?.appending(
                component: SocketFileName
            ).path(percentEncoded: false)
    }

    func closeSocket() {
        if Self.socketFD != -1 {
            logger.info("Closing socket")
            close(Self.socketFD)
            Self.socketFD = -1
        }
    }

    func connectSocket() -> Bool {
        // Reuse socket
        guard Self.socketFD == -1 else { return true }

        guard let socketPath else { return false }

        guard FileManager.default.fileExists(atPath: socketPath) else {
            logger.error("Socket file does not exist")
            return false
        }

        Self.socketFD = socket(AF_UNIX, SOCK_STREAM, 0)
        guard Self.socketFD > 0 else {
            logger.error("Failed to create socket")
            return false
        }

        var optval: Int = 1  // Use 1 to enable the option, 0 to disable
        guard
            setsockopt(
                Self.socketFD, SOL_SOCKET, SO_REUSEADDR, &optval,
                socklen_t(MemoryLayout<Int32>.size)) != -1
        else {
            logger.error("setsockopt error: \(errno)")
            return false
        }

        guard
            setsockopt(
                Self.socketFD, SOL_SOCKET, SO_SNDBUF, &maxMessageLength,
                socklen_t(MemoryLayout<Int32>.size(ofValue: maxMessageLength))) != -1
        else {
            logger.error("setsockopt error")
            return false
        }

        var address = sockaddr_un()
        address.sun_family = sa_family_t(AF_UNIX)

        withUnsafeMutableBytes(of: &address.sun_path) { ptr in
            socketPath.utf8CString.withUnsafeBytes { bytes in
                ptr.copyBytes(from: bytes)
            }
        }

        let result = withUnsafePointer(to: &address) {
            $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
                connect(Self.socketFD, $0, socklen_t(MemoryLayout<sockaddr_un>.size))
            }
        }

        if result != 0 {
            logger.error("Failed to connect to socket: \(errno)")
            close(Self.socketFD)
            Self.socketFD = -1
            return false
        }

        return true
    }

    func beginRequest(with context: NSExtensionContext) {
        let request = context.inputItems.first as? NSExtensionItem

        let message: Any?
        if #available(iOS 15.0, macOS 11.0, *) {
            message = request?.userInfo?[SFExtensionMessageKey]
        } else {
            message = request?.userInfo?["message"]
        }

        guard let message = message as? [String: Any],
            let data = try? JSONSerialization.data(withJSONObject: message as Any)
        else {
            context.cancelRequest(withError: SafariWebExtensionHandlerError.invalidRequest)
            return
        }

        if !Self.socketConnected {
            logger.info("Socket is not connected")
            if !connectSocket() {
                closeSocket()
                logger.error("Socket not connected")
                context.cancelRequest(
                    withError: SafariWebExtensionHandlerError.socketConnectionFailed)
                return
            }

            Self.socketConnected = true
        } else {
            logger.info("Socket is connected")
        }

        let bytesWritten = data.withUnsafeBytes { (bytes: UnsafeRawBufferPointer) -> Int in
            write(Self.socketFD, bytes.baseAddress, data.count)
        }

        if bytesWritten == -1 {
            logger.error("Cannot write to socket \(errno)")
            context.cancelRequest(withError: SafariWebExtensionHandlerError.socketWriteFailed)
            return
        }
        logger.debug("Sent message of \(data.count) bytes")

        var buffer = [UInt8](repeating: 0, count: Int(maxMessageLength))
        let bytesRead = read(Self.socketFD, &buffer, buffer.count)

        guard bytesRead > 0 else {
            logger.error("No bytes read")
            context.cancelRequest(withError: SafariWebExtensionHandlerError.socketReadFailed)
            return
        }

        let res = Data(buffer[0..<bytesRead])
        self.logger.debug("Received message: \(res)")

        guard let message = try? JSONSerialization.jsonObject(with: res) as? [String: Any] else {
            logger.error("Could not parse JSON from data")
            context.cancelRequest(withError: SafariWebExtensionHandlerError.invalidJSONResponse)
            return
        }

        let response = NSExtensionItem()
        response.userInfo = [SFExtensionMessageKey: message]

        context.completeRequest(returningItems: [response])
    }
}

enum SafariWebExtensionHandlerError: Error {
    case invalidRequest
    case socketConnectionFailed
    case socketWriteFailed
    case socketReadFailed
    case invalidJSONResponse
}
