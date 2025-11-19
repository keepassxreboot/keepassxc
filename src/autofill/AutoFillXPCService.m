/*
 *  Copyright (C) 2024 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#import "AutoFillXPCProtocol.h"
#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>
#import <os/log.h>

@interface AutoFillXPCService : NSObject <NSXPCListenerDelegate, AutoFillXPCProtocol> {
    NSXPCConnection* _providerConnection;
}

@property(nonatomic, strong) NSXPCListenerEndpoint* providerEndpoint;
@property(nonatomic, strong) dispatch_queue_t dispatchQueue;

- (NSXPCConnection*)connection;

@end

@implementation AutoFillXPCService

- (instancetype)init
{
    self = [super init];
    if (self) {
        _dispatchQueue = dispatch_queue_create("org.keepassxc.autofill.service.queue", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

- (BOOL)listener:(NSXPCListener *)listener shouldAcceptNewConnection:(NSXPCConnection *)newConnection {
    newConnection.exportedInterface = [NSXPCInterface interfaceWithProtocol:@protocol(AutoFillXPCProtocol)];
    newConnection.exportedObject = self;
    [newConnection resume];
    return YES;
}

- (void)registerProvider:(NSXPCListenerEndpoint*)endpoint withReply:(void (^)(NSError* error))reply
{
    void (^replyCopy)(NSError* error) = [reply copy];
    dispatch_async(self.dispatchQueue, ^{
        self.providerEndpoint = endpoint;
        if (replyCopy) {
            replyCopy(nil);
        }
    });
}

- (void)getLoginsForURL:(NSString*)url withReply:(void (^)(NSArray<NSDictionary<NSString*, id>*>* logins))reply
{
    void (^replyCopy)(NSArray<NSDictionary<NSString*, id>*>* logins) = [reply copy];
    dispatch_async(self.dispatchQueue, ^{
        NSXPCConnection* conn = self.connection;
        if (!conn) {
            os_log_error(OS_LOG_DEFAULT, "AutoFill XPC service: no provider connection available");
            if (replyCopy) {
                replyCopy(@[]);
            }
            return;
        }
        id<AutoFillProviderProtocol> provider = conn.remoteObjectProxy;
        [provider fetchCredentialsMatchingDomain:url
                                        withReply:^(NSArray<NSDictionary<NSString*, id>*>* credentials) {
                                            if (replyCopy) {
                                                replyCopy(credentials ?: @[]);
                                            }
                                        }];
    });
}

- (void)getCredentialWithRecordIdentifier:(NSString*)recordIdentifier
                                withReply:(void (^)(NSDictionary<NSString*, id>* credential))reply
{
    void (^replyCopy)(NSDictionary<NSString*, id>* credential) = [reply copy];
    dispatch_async(self.dispatchQueue, ^{
        NSXPCConnection* conn = self.connection;
        if (!conn) {
            os_log_error(OS_LOG_DEFAULT, "AutoFill XPC service: no provider connection available");
            if (replyCopy) {
                replyCopy(@{});
            }
            return;
        }
        id<AutoFillProviderProtocol> provider = conn.remoteObjectProxy;
        [provider fetchCredentialWithRecordIdentifier:recordIdentifier
                                             withReply:^(NSDictionary<NSString*, id>* credential) {
                                                 if (replyCopy) {
                                                     replyCopy(credential ?: @{});
                                                 }
                                             }];
    });
}

- (NSXPCConnection*)connection
{
    if (_providerConnection) {
        return _providerConnection;
    }

    if (!self.providerEndpoint) {
        os_log_error(OS_LOG_DEFAULT, "AutoFill service does not have provider endpoint");
        return nil;
    }

    _providerConnection = [[NSXPCConnection alloc] initWithListenerEndpoint:self.providerEndpoint];
    _providerConnection.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(AutoFillProviderProtocol)];

    __weak AutoFillXPCService* weakSelf = self;
    _providerConnection.interruptionHandler = ^{
        os_log_info(OS_LOG_DEFAULT, "AutoFill service provider connection interrupted");
    };
    _providerConnection.invalidationHandler = ^{
        os_log_info(OS_LOG_DEFAULT, "AutoFill service provider connection invalidated");
        AutoFillXPCService* strongSelf = weakSelf;
        if (strongSelf) {
            dispatch_async(strongSelf.dispatchQueue, ^{
                strongSelf->_providerConnection = nil;
            });
        }
    };

    [_providerConnection resume];

    return _providerConnection;
}

@end

int main(void) {
    @autoreleasepool {
        os_log(OS_LOG_DEFAULT, "KeePassXC AutoFill XPC Service starting.");
        NSXPCListener *listener = [NSXPCListener serviceListener];
        AutoFillXPCService *service = [[AutoFillXPCService alloc] init];
        listener.delegate = service;
        [listener resume];
        [[NSRunLoop currentRunLoop] run];
    }
    return 0;
}
