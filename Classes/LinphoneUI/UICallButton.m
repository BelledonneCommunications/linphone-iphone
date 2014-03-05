/* UICallButton.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */              

#import "UICallButton.h"
#import "LinphoneManager.h"

#import <CoreTelephony/CTCallCenter.h>

@implementation UICallButton

@synthesize addressField;


#pragma mark - Lifecycle Functions

- (void)initUICallButton {
    [self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
}

- (id)init {
    self = [super init];
    if (self) {
		[self initUICallButton];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
		[self initUICallButton];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUICallButton];
	}
    return self;
}	

- (void)dealloc {
	[addressField release];
    
    [super dealloc];
}


#pragma mark -

- (void)touchUp:(id) sender {
    NSString *address = [addressField text];
    NSString *displayName = nil;

    if( [address length] == 0){
        const MSList* logs = linphone_core_get_call_logs([LinphoneManager getLc]);
        while( logs ){
            LinphoneCallLog* log = logs->data;
            if( linphone_call_log_get_dir(log) == LinphoneCallOutgoing ){
                LinphoneProxyConfig* def_proxy = NULL;
                LinphoneAddress* to = linphone_call_log_get_to(log);
                const char*  domain = linphone_address_get_domain(to);
                char*   bis_address = NULL;

                linphone_core_get_default_proxy([LinphoneManager getLc], &def_proxy);

                // if the 'to' address is on the default proxy, only present the username
                if( def_proxy ){
                    const char* def_domain = linphone_proxy_config_get_domain(def_proxy);
                    if( def_domain && domain && !strcmp(domain, def_domain) ){
                        bis_address = ms_strdup(linphone_address_get_username(to));
                    }
                }

                if( bis_address == NULL ) {
                    bis_address = linphone_address_as_string_uri_only(to);
                }

                [addressField setText:[NSString stringWithUTF8String:bis_address]];
                ms_free(bis_address);
                // return after filling the address, let the user confirm the call by pressing again
                return;
            }
            logs = ms_list_next(logs);
        }
    }

    if( [address length] > 0){
        ABRecordRef contact = [[[LinphoneManager instance] fastAddressBook] getContact:address];
        if(contact) {
            displayName = [FastAddressBook getContactDisplayName:contact];
        }
        [[LinphoneManager instance] call:address displayName:displayName transfer:FALSE];
    }
}

@end
