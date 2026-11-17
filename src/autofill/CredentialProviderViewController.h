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

#import <AuthenticationServices/AuthenticationServices.h>
#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, KPAFCredentialListState) {
    KPAFCredentialListStateIdle = 0,
    KPAFCredentialListStateLoading,
    KPAFCredentialListStateEmpty,
    KPAFCredentialListStateError,
    KPAFCredentialListStatePopulated,
};

@interface KPAFCredential : NSObject

@property(nonatomic, copy, readonly) NSString* recordIdentifier;
@property(nonatomic, copy, readonly) NSString* username;
@property(nonatomic, copy, readonly) NSString* password;
@property(nonatomic, copy, readonly) NSString* title;
@property(nonatomic, copy, readonly) NSString* domain;

- (nullable instancetype)initWithDictionary:(NSDictionary<NSString*, id>*)dictionary;

@end

typedef void (^KPAFCredentialsCompletion)(NSArray<KPAFCredential*>* _Nullable credentials, NSError* _Nullable error);
typedef void (^KPAFCredentialCompletion)(KPAFCredential* _Nullable credential, NSError* _Nullable error);

@interface AutoFillServiceClient : NSObject

@property(nonatomic, copy) NSString* portName;

+ (instancetype)sharedClient;
- (void)fetchCredentialsForDomain:(NSString* _Nullable)domain
                       completion:(KPAFCredentialsCompletion _Nullable)completion;
- (void)fetchCredentialWithRecordIdentifier:(NSString*)recordIdentifier
                                 completion:(KPAFCredentialCompletion _Nullable)completion;

@end

@interface CredentialProviderViewController
    : ASCredentialProviderViewController <NSTableViewDataSource, NSTableViewDelegate>

@property(nonatomic, readonly) KPAFCredentialListState state;
@property(nonatomic, strong, readonly) NSMutableArray<KPAFCredential*>* credentials;

- (AutoFillServiceClient*)serviceClient;
- (void)fetchCredentialsForDomain:(NSString* _Nullable)domain;
- (void)fetchCredentialWithIdentifier:(NSString*)identifier silently:(BOOL)silently;
- (void)completeWithCredential:(KPAFCredential*)credential;

@end

NS_ASSUME_NONNULL_END
