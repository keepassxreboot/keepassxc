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
#import <os/log.h>

#import "AutoFillProviderProtocol.h"
#import "AutoFillXPCProtocol.h"

typedef NS_ENUM(NSInteger, KPAFCredentialListState)
{
    KPAFCredentialListStateIdle = 0,
    KPAFCredentialListStateLoading,
    KPAFCredentialListStateEmpty,
    KPAFCredentialListStateError,
    KPAFCredentialListStatePopulated,
};

static NSString* const KPAFEmptyMessage = @"Unlock KeePassXC to expose matching entries.";
static NSString* const KPAFErrorMessage =
    @"Unable to contact KeePassXC. Make sure the app is running and the database is unlocked.";

@interface KPAFCredential : NSObject

@property(nonatomic, copy, readonly) NSString* recordIdentifier;
@property(nonatomic, copy, readonly) NSString* username;
@property(nonatomic, copy, readonly) NSString* password;
@property(nonatomic, copy, readonly) NSString* title;
@property(nonatomic, copy, readonly) NSString* domain;
@property(nonatomic, copy, readonly) NSString* url;

- (nullable instancetype)initWithDictionary:(NSDictionary<NSString*, id>*)dictionary;

@end

@implementation KPAFCredential

- (instancetype)initWithDictionary:(NSDictionary<NSString*, id>*)dictionary
{
    self = [super init];
    if (!self) {
        return nil;
    }

    NSString* recordIdentifier = dictionary[AutoFillCredentialRecordIdentifierKey];
    NSString* username = dictionary[AutoFillCredentialUsernameKey];
    NSString* password = dictionary[AutoFillCredentialPasswordKey];
    NSString* title = dictionary[AutoFillCredentialTitleKey];
    NSString* domain = dictionary[AutoFillCredentialDomainKey];
    NSString* url = dictionary[AutoFillCredentialUrlKey];
    if (!recordIdentifier || !username || !password || !title || !domain || !url) {
        return nil;
    }

    _recordIdentifier = [recordIdentifier copy];
    _username = [username copy];
    _password = [password copy];
    _title = [title copy];
    _domain = [domain copy];
    _url = [url copy];
    return self;
}

@end

typedef void (^KPAFCredentialsCompletion)(NSArray<KPAFCredential*>* _Nullable credentials, NSError* _Nullable error);
typedef void (^KPAFCredentialCompletion)(KPAFCredential* _Nullable credential, NSError* _Nullable error);

@interface AutoFillServiceClient : NSObject

+ (instancetype)sharedClient;
- (void)fetchCredentialsForDomain:(NSString* _Nullable)domain completion:(KPAFCredentialsCompletion)completion;
- (void)fetchCredentialWithRecordIdentifier:(NSString*)recordIdentifier completion:(KPAFCredentialCompletion)completion;

@end

@implementation AutoFillServiceClient
{
    NSXPCConnection* m_connection;
    os_log_t m_log;
}

+ (instancetype)sharedClient
{
    static AutoFillServiceClient* sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        m_log = os_log_create("org.keepassxc.keepassxc", "AutoFillClient");
    }
    return self;
}

- (NSXPCConnection*)connection
{
    if (m_connection) {
        return m_connection;
    }

    NSXPCConnection* connection = [[NSXPCConnection alloc] initWithServiceName:@"org.keepassxc.KeePassXC.AutoFill-XPC-Service"];
    connection.remoteObjectInterface = [NSXPCInterface interfaceWithProtocol:@protocol(AutoFillXPCProtocol)];
    __weak AutoFillServiceClient* weakSelf = self;
    __weak NSXPCConnection* weakConnection = connection;
    connection.invalidationHandler = ^{
        AutoFillServiceClient* strongSelf = weakSelf;
        if (strongSelf) {
            os_log_error(strongSelf->m_log, "Lost AutoFill XPC connection.");
            if (weakConnection == strongSelf->m_connection) {
                strongSelf->m_connection = nil;
            }
        }
    };
    [connection resume];
    m_connection = connection;
    return m_connection;
}

- (id<AutoFillXPCProtocol>)proxyWithError:(void (^)(NSError*))errorHandler
{
    NSXPCConnection* connection = [self connection];
    id remoteObject = [connection remoteObjectProxyWithErrorHandler:^(NSError* error) {
        os_log_error(m_log, "AutoFill XPC proxy failed: %{public}@", error.localizedDescription);
        if (errorHandler) {
            errorHandler(error);
        }
    }];
    return [remoteObject conformsToProtocol:@protocol(AutoFillXPCProtocol)] ? remoteObject : nil;
}

- (void)fetchCredentialsForDomain:(NSString*)domain completion:(KPAFCredentialsCompletion)completion
{
    id<AutoFillXPCProtocol> proxy = [self proxyWithError:^(NSError* error) {
        if (completion) {
            completion(nil, error);
        }
    }];
    if (!proxy) {
        if (completion) {
            NSError* err = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOTCONN userInfo:nil];
            completion(nil, err);
        }
        return;
    }

    [proxy getLoginsForURL:domain ?: @""
                 withReply:^(NSArray<NSDictionary<NSString*, id>*>* payload) {
                     NSMutableArray<KPAFCredential*>* credentials = [NSMutableArray array];
                     for (NSDictionary<NSString*, id>* entry in payload) {
                         KPAFCredential* credential = [[KPAFCredential alloc] initWithDictionary:entry];
                         if (credential) {
                             [credentials addObject:credential];
                         }
                     }
                     if (completion) {
                         completion(credentials, nil);
                     }
                 }];
}

- (void)fetchCredentialWithRecordIdentifier:(NSString*)recordIdentifier completion:(KPAFCredentialCompletion)completion
{
    id<AutoFillXPCProtocol> proxy = [self proxyWithError:^(NSError* error) {
        if (completion) {
            completion(nil, error);
        }
    }];
    if (!proxy) {
        if (completion) {
            NSError* err = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOTCONN userInfo:nil];
            completion(nil, err);
        }
        return;
    }

    [proxy getCredentialWithRecordIdentifier:recordIdentifier
                                   withReply:^(NSDictionary<NSString*, id>* payload) {
                                       KPAFCredential* credential = [[KPAFCredential alloc] initWithDictionary:payload];
                                       if (completion) {
                                           if (credential) {
                                               completion(credential, nil);
                                           } else {
                                               NSError* err =
                                                   [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOENT userInfo:nil];
                                               completion(nil, err);
                                           }
                                       }
                                   }];
}

@end

@interface CredentialProviderViewController :
    ASCredentialProviderViewController <NSTableViewDataSource, NSTableViewDelegate>

@property(nonatomic) KPAFCredentialListState state;
@property(nonatomic, copy) NSString* statusMessage;
@property(nonatomic, strong) NSMutableArray<KPAFCredential*>* credentials;
@property(nonatomic, copy) NSString* currentDomain;
@property(nonatomic, strong) NSScrollView* scrollView;
@property(nonatomic, strong) NSTableView* tableView;
@property(nonatomic, strong) NSTextField* statusLabel;
@property(nonatomic, strong) NSProgressIndicator* activityIndicator;
@property(nonatomic) os_log_t log;

@end

@implementation CredentialProviderViewController

- (instancetype)init
{
    self = [super initWithNibName:nil bundle:nil];
    if (self) {
        _credentials = [NSMutableArray array];
        _state = KPAFCredentialListStateIdle;
        _statusMessage = @"";
        _log = os_log_create("org.keepassxc.keepassxc", "AutoFillUI");
    }
    return self;
}

- (void)loadView
{
    self.view = [[NSView alloc] initWithFrame:NSZeroRect];
    self.view.translatesAutoresizingMaskIntoConstraints = NO;
    [self setupTableView];
    [self setupStatusLabel];
    [self setupActivityIndicator];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self updateState];
}

- (void)prepareCredentialListForServiceIdentifiers:(NSArray<ASCredentialServiceIdentifier*>*)serviceIdentifiers
{
    self.currentDomain = serviceIdentifiers.firstObject.identifier;
    [self fetchCredentialsForDomain:self.currentDomain];
}

- (void)provideCredentialWithoutUserInteractionForCredentialIdentity:(ASPasswordCredentialIdentity*)credentialIdentity
                          NS_SWIFT_NAME(provideCredentialWithoutUserInteraction(for:))
{
    NSString* identifier = credentialIdentity.recordIdentifier;
    if (!identifier.length) {
        NSError* error = [NSError errorWithDomain:ASExtensionErrorDomain
                                             code:ASExtensionErrorCodeFailed
                                         userInfo:nil];
        [self.extensionContext cancelRequestWithError:error];
        return;
    }
    [self fetchCredentialWithIdentifier:identifier silently:YES];
}

- (void)prepareInterfaceToProvideCredentialForCredentialIdentity:(ASPasswordCredentialIdentity*)credentialIdentity
{
    NSString* identifier = credentialIdentity.recordIdentifier;
    if (!identifier.length) {
        NSError* error = [NSError errorWithDomain:ASExtensionErrorDomain
                                             code:ASExtensionErrorCodeFailed
                                         userInfo:nil];
        [self.extensionContext cancelRequestWithError:error];
        return;
    }
    [self fetchCredentialWithIdentifier:identifier silently:NO];
}

- (void)setupTableView
{
    NSTableColumn* titleColumn = [[NSTableColumn alloc] initWithIdentifier:@"title"];
    titleColumn.title = @"Item";
    titleColumn.width = 240.0;

    NSTableColumn* userColumn = [[NSTableColumn alloc] initWithIdentifier:@"username"];
    userColumn.title = @"Username";
    userColumn.width = 220.0;

    self.tableView = [[NSTableView alloc] initWithFrame:NSZeroRect];
    [self.tableView addTableColumn:titleColumn];
    [self.tableView addTableColumn:userColumn];
    self.tableView.delegate = self;
    self.tableView.dataSource = self;
    self.tableView.usesAlternatingRowBackgroundColors = YES;
    self.tableView.selectionHighlightStyle = NSTableViewSelectionHighlightStyleRegular;
    self.tableView.doubleAction = @selector(didDoubleClickRow:);
    self.tableView.target = self;

    self.scrollView = [[NSScrollView alloc] initWithFrame:NSZeroRect];
    self.scrollView.translatesAutoresizingMaskIntoConstraints = NO;
    self.scrollView.hasVerticalScroller = YES;
    self.scrollView.documentView = self.tableView;
    [self.view addSubview:self.scrollView];

    [NSLayoutConstraint activateConstraints:@[
        [self.scrollView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [self.scrollView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [self.scrollView.topAnchor constraintEqualToAnchor:self.view.topAnchor],
        [self.scrollView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    ]];
}

- (void)setupStatusLabel
{
    self.statusLabel = [NSTextField labelWithString:@""];
    self.statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.statusLabel.alignment = NSTextAlignmentCenter;
    self.statusLabel.lineBreakMode = NSLineBreakByWordWrapping;
    self.statusLabel.hidden = YES;
    [self.view addSubview:self.statusLabel];

    [NSLayoutConstraint activateConstraints:@[
        [self.statusLabel.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [self.statusLabel.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
        [self.statusLabel.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.leadingAnchor constant:16.0],
        [self.statusLabel.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.trailingAnchor constant:-16.0],
    ]];
}

- (void)setupActivityIndicator
{
    self.activityIndicator = [[NSProgressIndicator alloc] initWithFrame:NSZeroRect];
    self.activityIndicator.translatesAutoresizingMaskIntoConstraints = NO;
    self.activityIndicator.style = NSProgressIndicatorStyleSpinning;
    self.activityIndicator.controlSize = NSControlSizeRegular;
    self.activityIndicator.displayedWhenStopped = NO;
    [self.view addSubview:self.activityIndicator];

    [NSLayoutConstraint activateConstraints:@[
        [self.activityIndicator.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [self.activityIndicator.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    ]];
}

- (void)updateState
{
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!weakSelf) {
            return;
        }
        switch (weakSelf.state) {
        case KPAFCredentialListStateIdle:
            [weakSelf.activityIndicator stopAnimation:nil];
            weakSelf.statusLabel.hidden = YES;
            weakSelf.scrollView.hidden = YES;
            break;
        case KPAFCredentialListStateLoading:
            weakSelf.statusLabel.hidden = YES;
            weakSelf.scrollView.hidden = YES;
            [weakSelf.activityIndicator startAnimation:nil];
            break;
        case KPAFCredentialListStatePopulated:
            [weakSelf.activityIndicator stopAnimation:nil];
            weakSelf.statusLabel.hidden = YES;
            weakSelf.scrollView.hidden = NO;
            break;
        case KPAFCredentialListStateEmpty:
        case KPAFCredentialListStateError:
            [weakSelf.activityIndicator stopAnimation:nil];
            weakSelf.scrollView.hidden = YES;
            weakSelf.statusLabel.stringValue = weakSelf.statusMessage ?: @"";
            weakSelf.statusLabel.hidden = NO;
            break;
        }
    });
}

- (void)transitionToState:(KPAFCredentialListState)state message:(NSString*)message
{
    self.state = state;
    self.statusMessage = message ?: @"";
    [self updateState];
}

- (void)fetchCredentialsForDomain:(NSString*)domain
{
    [self transitionToState:KPAFCredentialListStateLoading message:nil];
    __weak typeof(self) weakSelf = self;
    [[AutoFillServiceClient sharedClient] fetchCredentialsForDomain:domain
                                                         completion:^(NSArray<KPAFCredential*>* _Nullable credentials,
                                                                     NSError* _Nullable error) {
                                                             dispatch_async(dispatch_get_main_queue(), ^{
                                                                 if (!weakSelf) {
                                                                     return;
                                                                 }
                                                                 if (error) {
                                                                     os_log_error(weakSelf.log,
                                                                                  "Failed to fetch credentials: %{public}@",
                                                                                  error.localizedDescription);
                                                                     [weakSelf transitionToState:KPAFCredentialListStateError
                                                                                         message:KPAFErrorMessage];
                                                                     return;
                                                                 }
                                                                 [weakSelf.credentials removeAllObjects];
                                                                 [weakSelf.credentials addObjectsFromArray:credentials];
                                                                 [weakSelf.tableView reloadData];
                                                                 if (weakSelf.credentials.count == 0) {
                                                                     [weakSelf transitionToState:KPAFCredentialListStateEmpty
                                                                                         message:KPAFEmptyMessage];
                                                                 } else {
                                                                     [weakSelf transitionToState:KPAFCredentialListStatePopulated
                                                                                         message:nil];
                                                                 }
                                                             });
                                                         }];
}

- (void)fetchCredentialWithIdentifier:(NSString*)identifier silently:(BOOL)silently
{
    if (!silently) {
        [self transitionToState:KPAFCredentialListStateLoading message:nil];
    }
    __weak typeof(self) weakSelf = self;
    [[AutoFillServiceClient sharedClient] fetchCredentialWithRecordIdentifier:identifier
                                                                   completion:^(KPAFCredential* _Nullable credential,
                                                                                NSError* _Nullable error) {
                                                                       dispatch_async(dispatch_get_main_queue(), ^{
                                                                           if (!weakSelf) {
                                                                               return;
                                                                           }
                                                                           if (credential) {
                                                                               [weakSelf completeWithCredential:credential];
                                                                           } else {
                                                                               os_log_error(weakSelf.log,
                                                                                            "Failed to fetch credential: %{public}@",
                                                                                            error.localizedDescription);
                                                                               NSError* extensionError =
                                                                                   [NSError errorWithDomain:ASExtensionErrorDomain
                                                                                                       code:ASExtensionErrorCodeCredentialIdentityNotFound
                                                                                                   userInfo:nil];
                                                                               [weakSelf.extensionContext
                                                                                   cancelRequestWithError:extensionError];
                                                                           }
                                                                       });
                                                                   }];
}

- (void)completeWithCredential:(KPAFCredential*)credential
{
    ASPasswordCredential* passwordCredential =
        [[ASPasswordCredential alloc] initWithUser:credential.username password:credential.password];
    [self.extensionContext completeRequestWithSelectedCredential:passwordCredential completionHandler:nil];
}

- (void)didDoubleClickRow:(id)sender
{
    NSInteger row = self.tableView.clickedRow;
    if (row < 0 || row >= static_cast<NSInteger>(self.credentials.count)) {
        return;
    }
    [self completeWithCredential:self.credentials[row]];
}

#pragma mark - NSTableViewDataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView*)tableView
{
    return static_cast<NSInteger>(self.credentials.count);
}

#pragma mark - NSTableViewDelegate

- (NSView*)tableView:(NSTableView*)tableView viewForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row
{
    if (row < 0 || row >= static_cast<NSInteger>(self.credentials.count)) {
        return nil;
    }

    NSString* identifier = tableColumn.identifier;
    KPAFCredential* credential = self.credentials[row];
    NSString* text = nil;
    if ([identifier isEqualToString:@"username"]) {
        text = credential.username.length ? credential.username : @"<no username>";
    } else {
        text = credential.title.length ? credential.title : credential.domain;
    }

    NSTableCellView* cellView =
        [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
    if (!cellView) {
        cellView = [[NSTableCellView alloc] initWithFrame:NSZeroRect];
        cellView.identifier = tableColumn.identifier;
        NSTextField* textField = [NSTextField labelWithString:text ?: @""];
        textField.translatesAutoresizingMaskIntoConstraints = NO;
        [cellView addSubview:textField];
        cellView.textField = textField;
        [NSLayoutConstraint activateConstraints:@[
            [textField.leadingAnchor constraintEqualToAnchor:cellView.leadingAnchor constant:4.0],
            [textField.trailingAnchor constraintEqualToAnchor:cellView.trailingAnchor constant:-4.0],
            [textField.topAnchor constraintEqualToAnchor:cellView.topAnchor constant:2.0],
            [textField.bottomAnchor constraintEqualToAnchor:cellView.bottomAnchor constant:-2.0],
        ]];
    }

    cellView.textField.stringValue = text ?: @"";
    return cellView;
}

- (BOOL)tableView:(NSTableView*)tableView shouldSelectRow:(NSInteger)row
{
    return row >= 0 && row < static_cast<NSInteger>(self.credentials.count);
}

@end
