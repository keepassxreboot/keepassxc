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

#import "CredentialProviderViewController.h"
#import <os/log.h>
#include <unistd.h>

#import "AutoFillProtocol.h"

static NSString* const KPAFEmptyMessage = @"Unlock KeePassXC to expose matching entries.";
static NSString* const KPAFErrorMessage =
    @"Unable to contact KeePassXC. Make sure the app is running and the database is unlocked.";

@implementation KPAFCredential

static NSString* KPAFStringValue(NSDictionary<NSString*, id>* dictionary, NSString* key)
{
    id value = dictionary[key];
    return [value isKindOfClass:NSString.class] ? value : nil;
}

- (instancetype)initWithDictionary:(NSDictionary<NSString*, id>*)dictionary
{
    self = [super init];
    if (!self) {
        return nil;
    }

    NSString* recordIdentifier = KPAFStringValue(dictionary, AutoFillCredentialRecordIdentifierKey);
    NSString* username = KPAFStringValue(dictionary, AutoFillCredentialUsernameKey);
    NSString* password = KPAFStringValue(dictionary, AutoFillCredentialPasswordKey);
    NSString* title = KPAFStringValue(dictionary, AutoFillCredentialTitleKey);
    NSString* domain = KPAFStringValue(dictionary, AutoFillCredentialDomainKey);
    if (!recordIdentifier || !username || !title || !domain) {
        return nil;
    }

    _recordIdentifier = [recordIdentifier copy];
    _username = [username copy];
    _password = [password copy] ?: @"";
    _title = [title copy];
    _domain = [domain copy];
    return self;
}

@end

@implementation AutoFillServiceClient
{
    os_log_t m_log;
    dispatch_queue_t m_queue;
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
        _portName = AutoFillMachPortName;
        m_log = os_log_create("org.keepassxc.keepassxc", "AutoFillClient");
        m_queue = dispatch_queue_create("org.keepassxc.keepassxc.autofill.client", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

- (void)sendRequest:(AutoFillMessageId)messageId
              query:(NSString*)query
         completion:(void (^)(id _Nullable plist, NSError* _Nullable error))completion
{
    NSString* portName = [self.portName copy] ?: AutoFillMachPortName;
    NSDictionary<NSString*, id>* payload = @{
        AutoFillRequestQueryKey : query ?: @"",
        AutoFillRequestPidKey : @(getpid()),
    };
    NSData* requestData = [NSPropertyListSerialization dataWithPropertyList:payload
                                                                     format:NSPropertyListBinaryFormat_v1_0
                                                                    options:0
                                                                      error:nil];
    dispatch_async(m_queue, ^{
        CFMessagePortRef remotePort = CFMessagePortCreateRemote(kCFAllocatorDefault, (__bridge CFStringRef)portName);
        if (!remotePort) {
            os_log_error(self->m_log, "Unable to connect to KeePassXC AutoFill Mach port.");
            if (completion) {
                NSError* err = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOTCONN userInfo:nil];
                completion(nil, err);
            }
            return;
        }

        CFDataRef responseDataRef = nullptr;
        SInt32 status = CFMessagePortSendRequest(
            remotePort, messageId, (__bridge CFDataRef)requestData, 5.0, 5.0, kCFRunLoopDefaultMode, &responseDataRef);
        CFMessagePortInvalidate(remotePort);
        CFRelease(remotePort);

        if (status != kCFMessagePortSuccess || !responseDataRef) {
            os_log_error(self->m_log, "AutoFill Mach request failed with status %d", static_cast<int>(status));
            if (responseDataRef) {
                CFRelease(responseDataRef);
            }
            if (completion) {
                NSError* err = [NSError errorWithDomain:NSPOSIXErrorDomain code:EIO userInfo:nil];
                completion(nil, err);
            }
            return;
        }

        NSData* responseData = CFBridgingRelease(responseDataRef);
        NSError* plistError = nil;
        id plist = [NSPropertyListSerialization propertyListWithData:responseData
                                                             options:NSPropertyListImmutable
                                                              format:nullptr
                                                               error:&plistError];
        if (completion) {
            completion(plist, plistError);
        }
    });
}

- (void)fetchCredentialsForDomain:(NSString*)domain completion:(KPAFCredentialsCompletion)completion
{
    [self sendRequest:AutoFillMessageFetchCredentialsForDomain
                query:domain ?: @""
           completion:^(id plist, NSError* error) {
               if (error || ![plist isKindOfClass:NSArray.class]) {
                   if (completion) {
                       completion(nil, error ?: [NSError errorWithDomain:NSPOSIXErrorDomain code:EINVAL userInfo:nil]);
                   }
                   return;
               }
               NSMutableArray<KPAFCredential*>* credentials = [NSMutableArray array];
               NSArray* entries = plist;
               for (NSDictionary<NSString*, id>* entry in entries) {
                   if ([entry isKindOfClass:NSDictionary.class]) {
                       KPAFCredential* credential = [[KPAFCredential alloc] initWithDictionary:entry];
                       if (credential) {
                           [credentials addObject:credential];
                       }
                   }
               }
               if (completion) {
                   completion(credentials, nil);
               }
           }];
}

- (void)fetchCredentialWithRecordIdentifier:(NSString*)recordIdentifier completion:(KPAFCredentialCompletion)completion
{
    [self sendRequest:AutoFillMessageFetchCredentialByRecordId
                query:recordIdentifier
           completion:^(id plist, NSError* error) {
               if (error || ![plist isKindOfClass:NSDictionary.class]) {
                   if (completion) {
                       completion(nil, error ?: [NSError errorWithDomain:NSPOSIXErrorDomain code:EINVAL userInfo:nil]);
                   }
                   return;
               }
               NSDictionary<NSString*, id>* dict = plist;
               KPAFCredential* credential = [[KPAFCredential alloc] initWithDictionary:dict];
               if (completion) {
                   if (credential) {
                       completion(credential, nil);
                   } else {
                       NSError* err = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOENT userInfo:nil];
                       completion(nil, err);
                   }
               }
           }];
}

@end

@interface CredentialProviderViewController ()

@property(nonatomic) KPAFCredentialListState state;
@property(nonatomic, strong) NSMutableArray<KPAFCredential*>* credentials;
@property(nonatomic) NSUInteger requestGeneration;
@property(nonatomic, strong) NSScrollView* scrollView;
@property(nonatomic, strong) NSTableView* tableView;
@property(nonatomic, strong) NSTextField* statusLabel;
@property(nonatomic, strong) NSProgressIndicator* activityIndicator;
@property(nonatomic, strong) NSButton* cancelButton;
@property(nonatomic, strong) NSButton* fillButton;
@property(nonatomic) os_log_t log;

@end

@implementation CredentialProviderViewController

- (instancetype)init
{
    self = [super initWithNibName:nil bundle:nil];
    if (self) {
        _credentials = [NSMutableArray array];
        _state = KPAFCredentialListStateIdle;
        _log = os_log_create("org.keepassxc.keepassxc", "AutoFillUI");
    }
    return self;
}

- (void)loadView
{
    self.view = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, 480.0, 260.0)];
    self.preferredContentSize = NSMakeSize(480.0, 260.0);
    [self setupActionButtons];
    [self setupTableView];
    [self setupStatusLabel];
    [self setupActivityIndicator];
    [self transitionToState:self.state message:nil];
}

- (void)prepareCredentialListForServiceIdentifiers:(NSArray<ASCredentialServiceIdentifier*>*)serviceIdentifiers
{
    [self fetchCredentialsForDomain:serviceIdentifiers.firstObject.identifier];
}

- (void)provideCredentialWithoutUserInteractionForIdentity:(ASPasswordCredentialIdentity*)credentialIdentity
{
    [self fetchCredentialWithIdentifier:credentialIdentity.recordIdentifier silently:YES];
}

- (void)prepareInterfaceToProvideCredentialForIdentity:(ASPasswordCredentialIdentity*)credentialIdentity
{
    [self fetchCredentialWithIdentifier:credentialIdentity.recordIdentifier silently:NO];
}

- (void)setupActionButtons
{
    self.cancelButton = [NSButton buttonWithTitle:@"Cancel" target:self action:@selector(didClickCancel:)];
    self.cancelButton.translatesAutoresizingMaskIntoConstraints = NO;
    self.cancelButton.keyEquivalent = @"\e";
    [self.view addSubview:self.cancelButton];

    self.fillButton = [NSButton buttonWithTitle:@"AutoFill" target:self action:@selector(didSelectRow:)];
    self.fillButton.translatesAutoresizingMaskIntoConstraints = NO;
    self.fillButton.keyEquivalent = @"\r";
    self.fillButton.enabled = NO;
    [self.view addSubview:self.fillButton];

    [NSLayoutConstraint activateConstraints:@[
        [self.cancelButton.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor constant:16.0],
        [self.cancelButton.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-12.0],
        [self.fillButton.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor constant:-16.0],
        [self.fillButton.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor constant:-12.0],
    ]];
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
    self.tableView.doubleAction = @selector(didSelectRow:);
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
        [self.scrollView.bottomAnchor constraintEqualToAnchor:self.fillButton.topAnchor constant:-12.0],
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

- (void)transitionToState:(KPAFCredentialListState)state message:(NSString*)message
{
    self.state = state;
    if (state == KPAFCredentialListStateLoading) {
        [self.activityIndicator startAnimation:nil];
    } else {
        [self.activityIndicator stopAnimation:nil];
    }
    BOOL isPopulated = (state == KPAFCredentialListStatePopulated);
    BOOL hasStatus = (state == KPAFCredentialListStateEmpty || state == KPAFCredentialListStateError);
    self.scrollView.hidden = !isPopulated;
    self.fillButton.hidden = !isPopulated;
    self.cancelButton.hidden = (state == KPAFCredentialListStateIdle);
    self.statusLabel.stringValue = message ?: @"";
    self.statusLabel.hidden = !hasStatus;
}

- (AutoFillServiceClient*)serviceClient
{
    return [AutoFillServiceClient sharedClient];
}

- (void)fetchCredentialsForDomain:(NSString*)domain
{
    NSUInteger generation = ++self.requestGeneration;
    [self transitionToState:KPAFCredentialListStateLoading message:nil];
    __weak typeof(self) weakSelf = self;
    [[self serviceClient] fetchCredentialsForDomain:domain
                                         completion:^(NSArray<KPAFCredential*>* _Nullable credentials,
                                                      NSError* _Nullable error) {
                                             dispatch_async(dispatch_get_main_queue(), ^{
                                                 if (!weakSelf || weakSelf.requestGeneration != generation) {
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
                                                     [weakSelf.tableView selectRowIndexes:[NSIndexSet indexSetWithIndex:0]
                                                                     byExtendingSelection:NO];
                                                     weakSelf.fillButton.enabled = YES;
                                                     [weakSelf transitionToState:KPAFCredentialListStatePopulated
                                                                         message:nil];
                                                 }
                                             });
                                         }];
}

- (void)fetchCredentialWithIdentifier:(NSString*)identifier silently:(BOOL)silently
{
    if (!identifier.length) {
        NSError* error = [NSError errorWithDomain:ASExtensionErrorDomain code:ASExtensionErrorCodeFailed userInfo:nil];
        [self.extensionContext cancelRequestWithError:error];
        return;
    }

    NSUInteger generation = ++self.requestGeneration;
    if (!silently) {
        [self transitionToState:KPAFCredentialListStateLoading message:nil];
    }
    __weak typeof(self) weakSelf = self;
    [[self serviceClient] fetchCredentialWithRecordIdentifier:identifier
                                                   completion:^(KPAFCredential* _Nullable credential,
                                                                NSError* _Nullable error) {
                                                       dispatch_async(dispatch_get_main_queue(), ^{
                                                           if (!weakSelf || weakSelf.requestGeneration != generation) {
                                                               return;
                                                           }
                                                           if (credential) {
                                                               [weakSelf completeWithCredential:credential];
                                                           } else {
                                                               os_log_error(weakSelf.log,
                                                                            "Failed to fetch credential: %{public}@",
                                                                            error.localizedDescription);
                                                               ASExtensionErrorCode errorCode =
                                                                   silently
                                                                       ? ASExtensionErrorCodeUserInteractionRequired
                                                                       : ASExtensionErrorCodeCredentialIdentityNotFound;
                                                               NSError* extensionError =
                                                                   [NSError errorWithDomain:ASExtensionErrorDomain
                                                                                       code:errorCode
                                                                                   userInfo:nil];
                                                               [weakSelf.extensionContext
                                                                   cancelRequestWithError:extensionError];
                                                           }
                                                       });
                                                   }];
}

- (void)completeWithCredential:(KPAFCredential*)credential
{
    if (!credential.password.length) {
        NSError* error = [NSError errorWithDomain:ASExtensionErrorDomain
                                             code:ASExtensionErrorCodeCredentialIdentityNotFound
                                         userInfo:nil];
        [self.extensionContext cancelRequestWithError:error];
        return;
    }

    ASPasswordCredential* passwordCredential =
        [[ASPasswordCredential alloc] initWithUser:credential.username password:credential.password];
    [self.extensionContext completeRequestWithSelectedCredential:passwordCredential completionHandler:nil];
}

- (void)didSelectRow:(id)sender
{
    NSInteger row = (sender == self.tableView && self.tableView.clickedRow >= 0) ? self.tableView.clickedRow
                                                                                 : self.tableView.selectedRow;
    if (row >= 0 && row < static_cast<NSInteger>(self.credentials.count)) {
        [self fetchCredentialWithIdentifier:self.credentials[row].recordIdentifier silently:NO];
    }
}

- (void)didClickCancel:(id)sender
{
    NSError* error = [NSError errorWithDomain:ASExtensionErrorDomain
                                         code:ASExtensionErrorCodeUserCanceled
                                     userInfo:nil];
    [self.extensionContext cancelRequestWithError:error];
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

    NSTableCellView* cellView = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
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

- (void)tableViewSelectionDidChange:(NSNotification*)notification
{
    NSInteger selectedRow = self.tableView.selectedRow;
    self.fillButton.enabled = (selectedRow >= 0 && selectedRow < static_cast<NSInteger>(self.credentials.count));
}

@end
