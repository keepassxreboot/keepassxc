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

#ifndef KEEPASSX_AUTOFILL_PROTOCOL_H
#define KEEPASSX_AUTOFILL_PROTOCOL_H

#import <Foundation/Foundation.h>

static NSString* const AutoFillMachPortName = @"org.keepassxc.keepassxc.autofill.ipc";

static NSString* const AutoFillRequestQueryKey = @"query";
static NSString* const AutoFillRequestPidKey = @"pid";

static NSString* const AutoFillCredentialRecordIdentifierKey = @"recordIdentifier";
static NSString* const AutoFillCredentialTitleKey = @"title";
static NSString* const AutoFillCredentialUsernameKey = @"username";
static NSString* const AutoFillCredentialPasswordKey = @"password";
static NSString* const AutoFillCredentialDomainKey = @"domain";

typedef NS_ENUM(SInt32, AutoFillMessageId) {
    AutoFillMessageFetchCredentialsForDomain = 1,
    AutoFillMessageFetchCredentialByRecordId = 2,
};

#endif // KEEPASSX_AUTOFILL_PROTOCOL_H
