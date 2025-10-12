#include "SafariWebExtensionHelper.h"

#include <QLocalSocket>
#include <QJsonObject>
#include <QString>
#include <QDebug>

#include <Foundation/Foundation.h>
#include <SafariServices/SafariServices.h>
#include <Security/SecBase.h>

SafariWebExtensionHelper* SafariWebExtensionHelper::instance()
{
    static SafariWebExtensionHelper instance;
    return &instance;
}

bool SafariWebExtensionHelper::isSafariWebExtension(QLocalSocket* socket)
{
    int sockfd = socket->socketDescriptor();
    pid_t pid = -1;
    socklen_t len = sizeof(pid);
    if (getsockopt(sockfd, SOL_LOCAL, LOCAL_PEERPID, &pid, &len) == -1) {
        qDebug() << "Failed to get peer PID, error:" << strerror(errno);
        return false;
    }

    CFNumberRef pidNumber = CFNumberCreate(NULL, kCFNumberIntType, &pid);

    const void *keys[] = { kSecGuestAttributePid };
    const void *values[] = { pidNumber };

    CFDictionaryRef attributes = CFDictionaryCreate(NULL, keys, values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (attributes == NULL) {
        if (pidNumber != NULL) CFRelease(pidNumber);
        return false;
    }

    SecCodeRef parentCode = NULL;
    OSStatus status = SecCodeCopyGuestWithAttributes(NULL, attributes, 0, &parentCode);

    if (attributes != NULL) CFRelease(attributes);
    if (pidNumber != NULL) CFRelease(pidNumber);

    if (status != errSecSuccess || parentCode == NULL) {
        qDebug() << "Failed to get SecCode for parent process (PID" << pid << "):" << static_cast<int>(status);
        return false;
    }

    CFStringRef appIdentifierCF = CFStringCreateWithCString(NULL, APPLE_APP_IDENTIFIER, kCFStringEncodingUTF8);
    CFStringRef requirementStringCF = CFStringCreateWithFormat(NULL, NULL, CFSTR("anchor apple generic and identifier \"%@.SafariWebExtension\""), appIdentifierCF);

    if (appIdentifierCF != NULL) CFRelease(appIdentifierCF);

    SecRequirementRef requirement = NULL;
    status = SecRequirementCreateWithString(requirementStringCF, SecCSFlags(), &requirement);

    if (requirementStringCF != NULL) CFRelease(requirementStringCF);

    if (status != errSecSuccess || requirement == NULL) {
        qDebug() << "Failed to create requirement:" << static_cast<int>(status);
        if (parentCode != NULL) {
            CFRelease(parentCode);
        }
        return false;
    }

    status = SecCodeCheckValidity(parentCode, SecCSFlags(), requirement);

    if (parentCode != NULL) {
        CFRelease(parentCode);
    }
    if (requirement != NULL) {
        CFRelease(requirement);
    }

    return (status == errSecSuccess);
}

void SafariWebExtensionHelper::broadcastClientMessage(const QString& reply)
{
    NSString* jsonString = reply.toNSString();
    NSData *data = [jsonString dataUsingEncoding:NSUTF8StringEncoding];

    NSError *error = nil;
    NSDictionary *message = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];

    if (error) {
        QString errorDescription = QString::fromNSString(error.localizedDescription);
        qDebug() << "Error converting NSString to NSDictionary:" << errorDescription;
        return;
    }

    NSString *appIdentifier = QString::fromUtf8(APPLE_APP_IDENTIFIER).toNSString();
    NSString *extensionIdentifier = [appIdentifier stringByAppendingString:@".SafariWebExtension"];

    [SFSafariApplication dispatchMessageWithName:@"proxy_message"
                    toExtensionWithIdentifier:extensionIdentifier
                                        userInfo:message
                              completionHandler:nil];
}
