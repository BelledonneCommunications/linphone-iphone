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

@synthesize callLog;
@synthesize avatarImage;
@synthesize addressLabel;
@synthesize dateLabel;
@synthesize dateHeaderLabel;
@synthesize durationLabel;
@synthesize durationHeaderLabel;
@synthesize typeLabel;
@synthesize typeHeaderLabel;
@synthesize addressButton;
@synthesize addContactButton;

#pragma mark - LifeCycle Functions

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [avatarImage release];
    [addressLabel release];
    [dateLabel release];
    [dateHeaderLabel release];
    [durationLabel release];
    [durationHeaderLabel release];
    [typeLabel release];
    [typeHeaderLabel release];
    [addressButton release];
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
                                                             fullscreen:false];
    }
    return compositeDescription;
}


#pragma mark - Property Functions

- (void)setCallLog:(LinphoneCallLog *)acallLog {
    self->callLog = acallLog;
    [self update];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    [HistoryDetailsViewController adaptSize:dateHeaderLabel field:dateLabel];
    [HistoryDetailsViewController adaptSize:durationHeaderLabel field:durationLabel];
    [HistoryDetailsViewController adaptSize:typeHeaderLabel field:typeLabel];
    [addressButton.titleLabel setAdjustsFontSizeToFitWidth:TRUE]; // Auto shrink: IB lack!
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(update:) 
                                                 name:@"LinphoneAddressBookUpdate" 
                                               object:nil];
    [self update];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneAddressBookUpdate" 
                                                  object:nil];
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
    // Don't update if callLog is null
    if(callLog==NULL) {
        return;
    }
    
	LinphoneAddress* addr; 
	if (callLog->dir == LinphoneCallIncoming) {
		addr = callLog->from;
	} else {
		addr = callLog->to;
	}
    
    UIImage *image = nil;
    NSString* address  = nil;
    if(addr != NULL) {
        BOOL useLinphoneAddress = true;
        // contact name 
        const char* lAddress = linphone_address_as_string_uri_only(addr);
        if(lAddress) {
            NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:lAddress]];
            contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
            if(contact) {
                image = [FastAddressBook getContactImage:contact thumbnail:false];
                address = [FastAddressBook getContactDisplayName:contact];
                useLinphoneAddress = false;
            }
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
        address = @"Unknown";
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
	if (callLog->dir == LinphoneCallIncoming) {
		[state setString:@"Incoming call"];
	} else {
		[state setString:@"Outgoing call"];
	}
    switch (callLog->status) {
        case LinphoneCallSuccess:
            break;
        case LinphoneCallAborted:
            [state appendString:@" (Aborted)"];
            break;
        case LinphoneCallMissed:
            [state appendString:@" (Missed)"];
            break;
        case LinphoneCallDeclined :
            [state appendString:@" (Declined)"];
            break;
    }
    [typeLabel setText:state];

    // Date
    NSDate *startData = [NSDate dateWithTimeIntervalSince1970:callLog->start_date_time];
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    NSLocale *locale = [NSLocale currentLocale];
    [dateFormatter setLocale:locale];
    [dateLabel setText:[dateFormatter stringFromDate:startData]];
    [dateFormatter release];

    // Duration
    int duration = callLog->duration;
    [durationLabel setText:[NSString stringWithFormat:@"%02i:%02i", (duration/60), duration - 60 * (duration / 60), nil]];
    
    if (addr != NULL) {
        // contact name 
        const char* lAddress = linphone_address_as_string_uri_only(addr);
        if(lAddress != NULL) {
            [addressButton setTitle:[NSString stringWithUTF8String:lAddress] forState:UIControlStateNormal];
            [addressButton setHidden:FALSE];
        } else {
           [addressButton setHidden:TRUE]; 
        }
    } else {
        [addressButton setHidden:TRUE];
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
            [controller setContact:contact];
        }
    }
}

- (IBAction)onAddContactClick:(id)event {
    [ContactSelection setSelectionMode:ContactSelectionModeEdit];
    [ContactSelection setAddAddress:[[addressButton titleLabel] text]];
    [ContactSelection setSipFilter:FALSE];
    ContactsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ContactsViewController compositeViewDescription] push:TRUE], ContactsViewController);
    if(controller != nil) {
    }
}

- (IBAction)onAddressClick:(id)event {
    LinphoneAddress* addr; 
	if (callLog->dir == LinphoneCallIncoming) {
		addr = callLog->from;
	} else {
		addr = callLog->to;
	}
    
    const char* lAddress = linphone_address_as_string_uri_only(addr);
    
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
}

@end
