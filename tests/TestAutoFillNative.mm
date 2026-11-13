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
#include "TestAutoFillNative.h"
#include <QTest>
#include <QUuid>
#include <cerrno>
#include <unistd.h>

#import "autofill/AutoFillProtocol.h"
#import "autofill/CredentialProviderViewController.h"
#import <AuthenticationServices/AuthenticationServices.h>
#import <Cocoa/Cocoa.h>
#import <dispatch/dispatch.h>

static NSDictionary<NSString*, id>* payload(NSString* identifier)
{
    return @{
        AutoFillCredentialRecordIdentifierKey : identifier,
        AutoFillCredentialUsernameKey : @"alice",
        AutoFillCredentialPasswordKey : @"test-only-secret",
        AutoFillCredentialTitleKey : @"Example account",
        AutoFillCredentialDomainKey : @"accounts.example.com"
    };
}

@interface NativeTestClient : AutoFillServiceClient
@property(nonatomic, strong) NSMutableArray<KPAFCredentialsCompletion>* listReplies;
@property(nonatomic, strong) NSMutableArray<KPAFCredentialCompletion>* credentialReplies;
@end
@implementation NativeTestClient
- (instancetype)init
{
    if ((self = [super init])) {
        _listReplies = [NSMutableArray array];
        _credentialReplies = [NSMutableArray array];
    }
    return self;
}
- (void)fetchCredentialsForDomain:(NSString*)domain completion:(KPAFCredentialsCompletion)completion
{
    [self.listReplies addObject:[completion copy]];
}
- (void)fetchCredentialWithRecordIdentifier:(NSString*)identifier completion:(KPAFCredentialCompletion)completion
{
    [self.credentialReplies addObject:[completion copy]];
}
@end

@interface NativeTestController : CredentialProviderViewController
@property(nonatomic, strong) NativeTestClient* client;
@property(nonatomic, strong) KPAFCredential* completedCredential;
@property(nonatomic) NSUInteger completionCount;
@end
@implementation NativeTestController
- (AutoFillServiceClient*)serviceClient
{
    return self.client;
}
- (void)completeWithCredential:(KPAFCredential*)credential
{
    self.completedCredential = credential;
    ++self.completionCount;
}
@end

#define DRAIN_MAIN_QUEUE()                                                                                             \
    do {                                                                                                               \
        __block bool drained = false;                                                                                  \
        dispatch_async(dispatch_get_main_queue(), ^{                                                                   \
            drained = true;                                                                                            \
        });                                                                                                            \
        QTRY_VERIFY(drained);                                                                                          \
    } while (false)

struct MessagePortFixture
{
    NSString* portName;
    NSString* recordId = @"port-record-1";
    CFMessagePortRef localPort = nullptr;
    CFRunLoopSourceRef runLoopSource = nullptr;

    static CFDataRef callback(CFMessagePortRef, SInt32 msgid, CFDataRef data, void* info)
    {
        auto* self = static_cast<MessagePortFixture*>(info);
        if (!self || !data) {
            return nullptr;
        }
        id plist = [NSPropertyListSerialization propertyListWithData:(__bridge NSData*)data
                                                             options:NSPropertyListImmutable
                                                              format:nullptr
                                                               error:nullptr];
        if (![plist isKindOfClass:NSDictionary.class]) {
            return nullptr;
        }
        NSDictionary* request = plist;
        NSNumber* pidNumber = request[AutoFillRequestPidKey];
        NSString* query = request[AutoFillRequestQueryKey];
        if (![pidNumber isKindOfClass:NSNumber.class] || pidNumber.intValue != getpid()
            || ![query isKindOfClass:NSString.class]) {
            return nullptr;
        }
        id responseObj = nil;
        if (msgid == AutoFillMessageFetchCredentialsForDomain) {
            responseObj = @[ payload(self->recordId) ];
        } else if (msgid == AutoFillMessageFetchCredentialByRecordId) {
            responseObj = [query isEqualToString:self->recordId] ? payload(self->recordId) : @{};
        }
        if (!responseObj) {
            return nullptr;
        }
        NSData* bytes = [NSPropertyListSerialization dataWithPropertyList:responseObj
                                                                   format:NSPropertyListBinaryFormat_v1_0
                                                                  options:0
                                                                    error:nil];
        return bytes ? static_cast<CFDataRef>(CFBridgingRetain(bytes)) : nullptr;
    }

    MessagePortFixture()
    {
        portName = QStringLiteral("org.keepassxc.autofill.test.%1")
                       .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
                       .toNSString();
        CFMessagePortContext context = {0, this, nullptr, nullptr, nullptr};
        localPort =
            CFMessagePortCreateLocal(kCFAllocatorDefault, (__bridge CFStringRef)portName, callback, &context, nullptr);
        if (localPort) {
            runLoopSource = CFMessagePortCreateRunLoopSource(kCFAllocatorDefault, localPort, 0);
            CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
        }
    }

    void close()
    {
        if (runLoopSource) {
            CFRunLoopRemoveSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
            CFRelease(runLoopSource);
            runLoopSource = nullptr;
        }
        if (localPort) {
            CFMessagePortInvalidate(localPort);
            CFRelease(localPort);
            localPort = nullptr;
        }
    }

    ~MessagePortFixture()
    {
        close();
    }
};

QTEST_MAIN(TestAutoFillNative)

void TestAutoFillNative::testStaleListReply_data()
{
    QTest::addColumn<bool>("staleError");
    QTest::newRow("stale-success") << false;
    QTest::newRow("stale-error") << true;
}

void TestAutoFillNative::testStaleListReply()
{
    QFETCH(bool, staleError);
    @autoreleasepool {
        NSTableView* tableView = [NSTableView new];
        NativeTestController* controller = [NativeTestController new];
        controller.client = [NativeTestClient new];
        [controller fetchCredentialsForDomain:@"old.example.com"];
        [controller fetchCredentialsForDomain:@"accounts.example.com"];
        QCOMPARE(controller.client.listReplies.count, NSUInteger(2));
        QCOMPARE(controller.state, KPAFCredentialListStateLoading);
        KPAFCredential* newer = [[KPAFCredential alloc] initWithDictionary:payload(@"newer-record")];
        KPAFCredential* second = [[KPAFCredential alloc] initWithDictionary:payload(@"second-record")];
        controller.client.listReplies[1](@[ newer, second ], nil);
        DRAIN_MAIN_QUEUE();
        QCOMPARE(controller.state, KPAFCredentialListStatePopulated);
        QCOMPARE([controller numberOfRowsInTableView:tableView], NSInteger(2));
        NSError* error = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOTCONN userInfo:nil];
        KPAFCredential* older = [[KPAFCredential alloc] initWithDictionary:payload(@"older-record")];
        controller.client.listReplies[0](staleError ? nil : @[ older ], staleError ? error : nil);
        DRAIN_MAIN_QUEUE();
        QCOMPARE(controller.state, KPAFCredentialListStatePopulated);
        QCOMPARE([controller numberOfRowsInTableView:tableView], NSInteger(2));
        QCOMPARE(QString::fromNSString(controller.credentials[0].recordIdentifier), QStringLiteral("newer-record"));
        QCOMPARE(QString::fromNSString(controller.credentials[1].recordIdentifier), QStringLiteral("second-record"));
        [controller fetchCredentialsForDomain:@"unavailable.example.com"];
        controller.client.listReplies[2](nil, error);
        DRAIN_MAIN_QUEUE();
        QCOMPARE(controller.state, KPAFCredentialListStateError);
        QCOMPARE([controller numberOfRowsInTableView:tableView], NSInteger(2));
    }
}

void TestAutoFillNative::testStaleCredentialAfterListRequest()
{
    @autoreleasepool {
        NSTableView* tableView = [NSTableView new];
        NativeTestController* controller = [NativeTestController new];
        controller.client = [NativeTestClient new];
        KPAFCredential* credential = [[KPAFCredential alloc] initWithDictionary:payload(@"selected-record")];
        [controller fetchCredentialWithIdentifier:@"selected-record" silently:YES];
        controller.client.credentialReplies[0](credential, nil);
        DRAIN_MAIN_QUEUE();
        QCOMPARE(controller.completionCount, NSUInteger(1));
        QCOMPARE(QString::fromNSString(controller.completedCredential.recordIdentifier),
                 QStringLiteral("selected-record"));
        controller.completedCredential = nil;
        [controller fetchCredentialWithIdentifier:@"selected-record" silently:NO];
        [controller fetchCredentialsForDomain:@"accounts.example.com"];
        controller.client.credentialReplies[1](credential, nil);
        DRAIN_MAIN_QUEUE();
        QCOMPARE(controller.completionCount, NSUInteger(1));
        QVERIFY(controller.completedCredential == nil);
        QCOMPARE(controller.state, KPAFCredentialListStateLoading);
        KPAFCredential* listed = [[KPAFCredential alloc] initWithDictionary:payload(@"listed-record")];
        controller.client.listReplies[0](@[ listed ], nil);
        DRAIN_MAIN_QUEUE();
        QCOMPARE(controller.state, KPAFCredentialListStatePopulated);
        QCOMPARE([controller numberOfRowsInTableView:tableView], NSInteger(1));
        QCOMPARE(QString::fromNSString(controller.credentials[0].recordIdentifier), QStringLiteral("listed-record"));
    }
}

void TestAutoFillNative::testMessagePortRoundTrip()
{
    @autoreleasepool {
        MessagePortFixture fixture;
        QVERIFY(fixture.localPort != nullptr);

        AutoFillServiceClient* client = [AutoFillServiceClient new];
        client.portName = fixture.portName;

        __block NSArray<KPAFCredential*>* listResult = nil;
        __block NSError* listError = nil;
        __block bool listDone = false;
        [client fetchCredentialsForDomain:@"accounts.example.com"
                               completion:^(NSArray<KPAFCredential*>* credentials, NSError* error) {
                                   listResult = credentials;
                                   listError = error;
                                   listDone = true;
                               }];
        QTRY_VERIFY(listDone);
        QVERIFY(listError == nil);
        QCOMPARE(listResult.count, NSUInteger(1));
        QCOMPARE(QString::fromNSString(listResult[0].recordIdentifier), QStringLiteral("port-record-1"));

        __block KPAFCredential* credResult = nil;
        __block NSError* credError = nil;
        __block bool credDone = false;
        [client fetchCredentialWithRecordIdentifier:@"port-record-1"
                                         completion:^(KPAFCredential* credential, NSError* error) {
                                             credResult = credential;
                                             credError = error;
                                             credDone = true;
                                         }];
        QTRY_VERIFY(credDone);
        QVERIFY(credError == nil);
        QVERIFY(credResult != nil);
        QCOMPARE(QString::fromNSString(credResult.password), QStringLiteral("test-only-secret"));
    }
}

void TestAutoFillNative::testMessagePortInvalidation()
{
    @autoreleasepool {
        MessagePortFixture fixture;
        QVERIFY(fixture.localPort != nullptr);

        AutoFillServiceClient* client = [AutoFillServiceClient new];
        client.portName = fixture.portName;
        fixture.close();

        __block NSArray<KPAFCredential*>* listResult = nil;
        __block NSError* listError = nil;
        __block bool listDone = false;
        [client fetchCredentialsForDomain:@"accounts.example.com"
                               completion:^(NSArray<KPAFCredential*>* credentials, NSError* error) {
                                   listResult = credentials;
                                   listError = error;
                                   listDone = true;
                               }];
        QTRY_VERIFY(listDone);
        QVERIFY(listResult == nil);
        QVERIFY(listError != nil);
    }
}
