/* HistoryDetailsViewController.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "HistoryDetailsViewController.h"
#import "PhoneMainView.h"
#import "FastAddressBook.h"
#import "Utils.h"

@implementation HistoryDetailsViewController

@synthesize callLogId;
@synthesize avatarImage;
@synthesize addressLabel;
@synthesize dateLabel;
@synthesize dateHeaderLabel;
@synthesize durationLabel;
@synthesize durationHeaderLabel;
@synthesize typeLabel;
@synthesize typeHeaderLabel;
@synthesize plainAddressLabel;
@synthesize plainAddressHeaderLabel;
@synthesize callButton;
@synthesize messageButton;
@synthesize addContactButton;

#pragma mark - LifeCycle Functions

- (id)init {
    self = [super initWithNibName:@"HistoryDetailsViewController" bundle:[NSBundle mainBundle]];
    if(self != nil) {
        dateFormatter = [[NSDateFormatter alloc] init];
        [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
        [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
        NSLocale *locale = [NSLocale currentLocale];
        [dateFormatter setLocale:locale];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [dateFormatter release];
    [callLogId release];
    
    [avatarImage release];
    [addressLabel release];
    [dateLabel release];
    [dateHeaderLabel release];
    [durationLabel release];
    [durationHeaderLabel release];
    [typeLabel release];
    [typeHeaderLabel release];
    [plainAddressLabel release];
    [plainAddressHeaderLabel release];
    [callButton release];
    [messageButton release];
    [addContactButton release];
    
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"HistoryDetails" 
                                                                content:@"HistoryDetailsViewController" 
                                                               stateBar:nil 
                                                        stateBarEnabled:false 
                                                                 tabBar:@"UIMainBar" 
                                                          tabBarEnabled:true 
                                                             fullscreen:false
                                                          landscapeMode:[LinphoneManager runningOnIpad]
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - Property Functions

- (void)setCallLogId:(NSString *)acallLogId {
    self->callLogId = [acallLogId copy];
    [self update];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    [HistoryDetailsViewController adaptSize:dateHeaderLabel field:dateLabel];
    [HistoryDetailsViewController adaptSize:durationHeaderLabel field:durationLabel];
    [HistoryDetailsViewController adaptSize:typeHeaderLabel field:typeLabel];
    [HistoryDetailsViewController adaptSize:plainAddressHeaderLabel field:plainAddressLabel];
    [callButton.titleLabel setAdjustsFontSizeToFitWidth:TRUE]; // Auto shrink: IB lack!
    [messageButton.titleLabel setAdjustsFontSizeToFitWidth:TRUE]; // Auto shrink: IB lack!
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(update) 
                                                 name:kLinphoneAddressBookUpdate
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(coreUpdateEvent:)
                                                 name:kLinphoneCoreUpdate
                                               object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:kLinphoneAddressBookUpdate
                                                  object:nil];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneCoreUpdate
                                                  object:nil];
}


#pragma mark - Event Functions

- (void)coreUpdateEvent:(NSNotification*)notif {
    [self update];
}


#pragma mark - 

+ (void)adaptSize:(UILabel*)label field:(UIView*)field {
    //
    // Adapt size
    //
    CGRect labelFrame = [label frame];
    CGRect fieldFrame = [field frame];
    
    fieldFrame.origin.x -= labelFrame.size.width;
    
    // Compute firstName size
    CGSize contraints;
    contraints.height = [label frame].size.height;
    contraints.width = ([field frame].size.width + [field frame].origin.x) - [label frame].origin.x;
    CGSize firstNameSize = [[label text] sizeWithFont:[label font] constrainedToSize: contraints];
    labelFrame.size.width = firstNameSize.width;
    
    // Compute lastName size & position
    fieldFrame.origin.x += labelFrame.size.width;
    fieldFrame.size.width = (contraints.width + [label frame].origin.x) - fieldFrame.origin.x;
    
    [label setFrame: labelFrame];
    [field setFrame: fieldFrame];
}

- (void)update {
    if(![LinphoneManager isLcReady]) {
        return;
    }
    
    // Look for the call log
    callLog = NULL;
    const MSList *list = linphone_core_get_call_logs([LinphoneManager getLc]);
    while(list != NULL) {
        LinphoneCallLog *log = (LinphoneCallLog *)list->data;
        const char *cid = linphone_call_log_get_call_id(log);
        if(cid != NULL && [callLogId isEqualToString:[NSString stringWithUTF8String:cid]]) {
            callLog = log;
            break;
        }
        list = list->next;
    }
    
    // Pop if callLog is null
    if(callLog == NULL) {
        [[PhoneMainView instance] popCurrentView];
        return;
    }
    
	LinphoneAddress* addr =linphone_call_log_get_remote_address(callLog);
    
    UIImage *image = nil;
    NSString* address  = nil;
    if(addr != NULL) {
        BOOL useLinphoneAddress = true;
        // contact name 
        char* lAddress = linphone_address_as_string_uri_only(addr);
        if(lAddress) {
            NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:lAddress]];
            contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
            if(contact) {
                image = [FastAddressBook getContactImage:contact thumbnail:true];
                address = [FastAddressBook getContactDisplayName:contact];
                useLinphoneAddress = false;
            }
            ms_free(lAddress);
        }
        if(useLinphoneAddress) {
            const char* lDisplayName = linphone_address_get_display_name(addr);
            const char* lUserName = linphone_address_get_username(addr);
            if (lDisplayName) 
                address = [NSString stringWithUTF8String:lDisplayName];
            else if(lUserName) 
                address = [NSString stringWithUTF8String:lUserName];
        }
    }
    
    // Set Image
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown.png"];
    }
    [avatarImage setImage:image];
    
    // Set Address
    if(address == nil) {
        address = NSLocalizedString(@"Unknown", nil);
    }
    [addressLabel setText:address];
    
    // Hide/Show add button
    if(contact) {
        [addContactButton setHidden:TRUE];
    } else {
        [addContactButton setHidden:FALSE];
    }
    
    // State
    NSMutableString *state = [NSMutableString string];
	if (linphone_call_log_get_dir(callLog) == LinphoneCallIncoming) {
		[state setString:NSLocalizedString(@"Incoming call", nil)];
	} else {
		[state setString:NSLocalizedString(@"Outgoing call", nil)];
	}
    switch (linphone_call_log_get_status(callLog)) {
        case LinphoneCallSuccess:
            break;
        case LinphoneCallAborted:
            [state appendString:NSLocalizedString(@" (Aborted)", nil)];
            break;
        case LinphoneCallMissed:
            [state appendString:NSLocalizedString(@" (Missed)", nil)];
            break;
        case LinphoneCallDeclined:
            [state appendString:NSLocalizedString(@" (Declined)", nil)];
            break;
    }
    [typeLabel setText:state];

    // Date
    NSDate *startData = [NSDate dateWithTimeIntervalSince1970:linphone_call_log_get_start_date(callLog)];
    [dateLabel setText:[dateFormatter stringFromDate:startData]];

    // Duration
    int duration = linphone_call_log_get_duration(callLog);
    [durationLabel setText:[NSString stringWithFormat:@"%02i:%02i", (duration/60), duration - 60 * (duration / 60), nil]];
    
    // contact name
    [plainAddressLabel setText:@""];
    if (addr != NULL) {
        if ([[LinphoneManager instance] lpConfigBoolForKey:@"contact_display_username_only"]) {
			[plainAddressLabel setText:[NSString stringWithUTF8String:linphone_address_get_username(addr)?linphone_address_get_username(addr):""]];
		} else {
			char* lAddress = linphone_address_as_string_uri_only(addr);
			if(lAddress != NULL) {
				[plainAddressLabel setText:[NSString stringWithUTF8String:lAddress]];
				ms_free(lAddress);
			}
		}
    }
    
    if (addr != NULL) {
        [callButton setHidden:FALSE];
        [messageButton setHidden:FALSE];
    } else {
        [callButton setHidden:TRUE];
        [messageButton setHidden:TRUE];
    }
    
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popCurrentView];
}

- (IBAction)onContactClick:(id)event {
    if(contact) {
        ContactDetailsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ContactDetailsViewController compositeViewDescription] push:TRUE], ContactDetailsViewController);
        if(controller != nil) {
            [ContactSelection setSelectionMode:ContactSelectionModeNone];
            [controller setContact:contact];
        }
    }
}

- (IBAction)onAddContactClick:(id)event {
    LinphoneAddress* addr;
	
	addr=linphone_call_log_get_remote_address(callLog);
    if (addr != NULL) {
        char* lAddress = linphone_address_as_string_uri_only(addr);
        if(lAddress != NULL) {
            [ContactSelection setAddAddress:[NSString stringWithUTF8String:lAddress]];
            [ContactSelection setSelectionMode:ContactSelectionModeEdit];
            
            [ContactSelection setSipFilter:nil];
            [ContactSelection setEmailFilter:FALSE];
            ContactsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ContactsViewController compositeViewDescription] push:TRUE], ContactsViewController);
            if(controller != nil) {
            }
            ms_free(lAddress);
        }
    }
}

- (IBAction)onCallClick:(id)event {
    LinphoneAddress* addr; 
	addr=linphone_call_log_get_remote_address(callLog);
    
    char* lAddress = linphone_address_as_string_uri_only(addr);
    if(lAddress == NULL) 
        return;
    
    NSString *displayName = nil;
    if(contact != nil) {
       displayName = [FastAddressBook getContactDisplayName:contact]; 
    } else {
        const char* lDisplayName = linphone_address_get_display_name(addr);
        const char* lUserName = linphone_address_get_username(addr);
        if (lDisplayName) 
            displayName = [NSString stringWithUTF8String:lDisplayName];
        else if(lUserName) 
            displayName = [NSString stringWithUTF8String:lUserName];
    }
    
    
    DialerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
    if(controller != nil) {
        if(displayName != nil) {
            [controller call:[NSString stringWithUTF8String:lAddress] displayName:displayName];
        } else {
            [controller call:[NSString stringWithUTF8String:lAddress]];
        }
    }
    ms_free(lAddress);
}

- (IBAction)onMessageClick:(id)event {
    LinphoneAddress* addr;
	addr=linphone_call_log_get_remote_address(callLog);
    
    char* lAddress = linphone_address_as_string_uri_only(addr);
    if(lAddress == NULL)
        return;
    
    NSString *displayName = nil;
    if(contact != nil) {
        displayName = [FastAddressBook getContactDisplayName:contact];
    } else {
        const char* lDisplayName = linphone_address_get_display_name(addr);
        const char* lUserName = linphone_address_get_username(addr);
        if (lDisplayName)
            displayName = [NSString stringWithUTF8String:lDisplayName];
        else if(lUserName)
            displayName = [NSString stringWithUTF8String:lUserName];
    }
    
    // Go to ChatRoom view
    [[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
    ChatRoomViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription] push:TRUE], ChatRoomViewController);
    if(controller != nil) {
        [controller setRemoteAddress:[NSString stringWithUTF8String:lAddress]];
    }
    ms_free(lAddress);
}

@end
