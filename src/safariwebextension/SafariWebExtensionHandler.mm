#include "SafariWebExtensionHandler.h"

#include <Foundation/Foundation.h>
#include <SafariServices/SafariServices.h>

NSString * const SafariWebExtensionHandlerErrorDomain = @"org.keepassxc.keepassxc.SafariWebExtensionHandlerError";

NSString *applicationGroupIdentifier = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"APP_GROUP_IDENTIFIER"];;
NSURL *containerURL = [[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:applicationGroupIdentifier];
NSString *socketFileName = @"KeePassXC.BrowserServer";
NSURL *socketURL = [containerURL URLByAppendingPathComponent:socketFileName];
int32_t socketFD = -1;
int32_t maxMessageLength = 1024 * 1024;

@implementation SafariWebExtensionHandler

- (void)beginRequestWithExtensionContext:(NSExtensionContext *)context {
    NSExtensionItem *request = context.inputItems.firstObject;
    if (!request) {
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorInvalidRequest userInfo:@{NSLocalizedDescriptionKey: @"No input items."}]];
        return;
    }

    id message = nil;
    if (@available(iOS 15.0, macOS 11.0, *)) {
        message = request.userInfo[SFExtensionMessageKey];
    } else {
        message = request.userInfo[@"message"];
    }

    if (!message) {
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorInvalidRequest userInfo:@{NSLocalizedDescriptionKey: @"No message found."}]];
        return;
    }

    NSError *serializationError = nil;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:message options:0 error:&serializationError];
    if (!jsonData) {
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorInvalidRequest userInfo:@{NSLocalizedDescriptionKey: @"JSON serialization failed."}]];
        return;
    }

    if (![self connectSocket]) {
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorSocketConnectionFailed userInfo:@{NSLocalizedDescriptionKey: @"Could not connect to socket."}]];
        return;
    }

    ssize_t bytesWritten = write(socketFD, jsonData.bytes, jsonData.length);
    if (bytesWritten == -1) {
        [self closeSocket];
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorSocketWriteFailed userInfo:@{NSLocalizedDescriptionKey: @"Write to socket failed."}]];
        return;
    }

    NSMutableData *buffer = [NSMutableData dataWithLength:maxMessageLength];
    ssize_t bytesRead = read(socketFD, [buffer mutableBytes], maxMessageLength);
    if (bytesRead <= 0) {
        [self closeSocket];
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorSocketReadFailed userInfo:@{NSLocalizedDescriptionKey: @"Read from socket failed."}]];
        return;
    }

    NSData *responseData = [NSData dataWithBytesNoCopy:[buffer mutableBytes] length:bytesRead freeWhenDone:NO];
    NSDictionary *jsonResponse = [NSJSONSerialization JSONObjectWithData:responseData options:0 error:nil];
    if (!jsonResponse) {
        [context cancelRequestWithError:[NSError errorWithDomain:SafariWebExtensionHandlerErrorDomain code:SafariWebExtensionHandlerErrorInvalidJSONResponse userInfo:@{NSLocalizedDescriptionKey: @"Response JSON parsing failed."}]];
        return;
    }

    NSExtensionItem *responseItem = [[NSExtensionItem alloc] init];
    if (@available(iOS 15.0, macOS 11.0, *)) {
        responseItem.userInfo = @{ SFExtensionMessageKey: jsonResponse };
    } else {
        responseItem.userInfo = @{ @"message": jsonResponse };
    }

    [context completeRequestReturningItems:@[responseItem] completionHandler:nil];
}

- (bool)connectSocket {
    if (socketFD != -1) return YES;

    socketFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socketFD == -1) return NO;

    int optval = 1;
    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        [self closeSocket];
        return NO;
    }

    int max = 1024 * 1024;
    if (setsockopt(socketFD, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&max), sizeof(max)) == -1) {
        [self closeSocket];
        return NO;
    }

    struct sockaddr_un address = {
        .sun_family = AF_UNIX,
    };
    strncpy(address.sun_path, [socketURL fileSystemRepresentation], sizeof(address.sun_path) - 1);

    int result = connect(socketFD, reinterpret_cast<struct sockaddr *>(&address), sizeof(struct sockaddr_un));
    if (result != 0) {
        [self closeSocket];
        return NO;
    }

    return YES;
}

- (void)closeSocket {
    if (socketFD != -1) {
        close(socketFD);
        socketFD = -1;
    }
}

@end
