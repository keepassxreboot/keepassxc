#import <Foundation/Foundation.h>

extern NSString * const SafariWebExtensionHandlerErrorDomain;

typedef NS_ENUM(NSInteger, SafariWebExtensionHandlerError) {
    SafariWebExtensionHandlerErrorInvalidRequest = 1,
    SafariWebExtensionHandlerErrorSocketConnectionFailed,
    SafariWebExtensionHandlerErrorSocketWriteFailed,
    SafariWebExtensionHandlerErrorSocketReadFailed,
    SafariWebExtensionHandlerErrorInvalidJSONResponse,
};

@interface SafariWebExtensionHandler : NSObject <NSExtensionRequestHandling>

@end
