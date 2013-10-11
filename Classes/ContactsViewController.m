/* ContactsViewController.m
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

#import "ContactsViewController.h"
#import "PhoneMainView.h"
#import "Utils.h"

#import <AddressBook/ABPerson.h>

@implementation ContactSelection

static ContactSelectionMode sSelectionMode = ContactSelectionModeNone;
static NSString* sAddAddress = nil;
static NSString* sSipFilter = nil;
static BOOL sEmailFilter = FALSE;

+ (void)setSelectionMode:(ContactSelectionMode)selectionMode {
    sSelectionMode = selectionMode;
}

+ (ContactSelectionMode)getSelectionMode {
    return sSelectionMode;
}

+ (void)setAddAddress:(NSString*)address {
    if(sAddAddress != nil) {
        [sAddAddress release];
        sAddAddress= nil;
    }
    if(address != nil) {
        sAddAddress = [address retain];
    }
}

+ (NSString*)getAddAddress {
    return sAddAddress;
}

+ (void)setSipFilter:(NSString*)domain {
    [sSipFilter release];
	sSipFilter = [domain retain];
}

+ (NSString*)getSipFilter {
    return sSipFilter;
}

+ (void)setEmailFilter:(BOOL)enable {
    sEmailFilter = enable;
}

+ (BOOL)getEmailFilter {
    return sEmailFilter;
}

@end

@implementation ContactsViewController

@synthesize tableController;
@synthesize tableView;

@synthesize allButton;
@synthesize linphoneButton;
@synthesize backButton;
@synthesize addButton;

typedef enum _HistoryView {
    History_All,
    History_Linphone,
    History_MAX
} HistoryView;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ContactsViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [tableController release];
    [tableView release];
    
    [allButton release];
    [linphoneButton release];
    [backButton release];
    [addButton release];
    
    [super dealloc];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"Contacts" 
                                                                content:@"ContactsViewController" 
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


#pragma mark - ViewController Functions

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewWillDisappear:animated];
    }
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewWillAppear:animated];
    }   
    
    [self update];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewDidAppear:animated];
    }
    if(![FastAddressBook isAuthorized]) {
        UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Address book",nil)
														message:NSLocalizedString(@"You must authorize the application to have access to address book.\n"
                                                                                  "Toggle the application in Settings > Privacy > Contacts",nil)
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
											  otherButtonTitles:nil];
		[error show];
		[error release];
        [[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
    }
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewDidDisappear:animated];
    }  
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self changeView:History_All];
    
    // Set selected+over background: IB lack !
    [linphoneButton setBackgroundImage:[UIImage imageNamed:@"contacts_linphone_selected.png"]
                 forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    [linphoneButton setTitle:[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleDisplayName"]
					forState:UIControlStateNormal];
	
	[LinphoneUtils buttonFixStates:linphoneButton];
    
    // Set selected+over background: IB lack !
    [allButton setBackgroundImage:[UIImage imageNamed:@"contacts_all_selected.png"] 
                    forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    [LinphoneUtils buttonFixStates:allButton];
    
    [tableController.tableView setBackgroundColor:[UIColor clearColor]]; // Can't do it in Xib: issue with ios4
    [tableController.tableView setBackgroundView:nil]; // Can't do it in Xib: issue with ios4
}


#pragma mark -

- (void)changeView:(HistoryView)view {
    if(view == History_All) {
        [ContactSelection setSipFilter:nil];
        [ContactSelection setEmailFilter:FALSE];
        [tableController loadData];
        allButton.selected = TRUE;
    } else {
        allButton.selected = FALSE;
    }
    
    if(view == History_Linphone) {
        [ContactSelection setSipFilter:[LinphoneManager instance].contactFilter];
	[ContactSelection setEmailFilter:FALSE];
        [tableController loadData];
        linphoneButton.selected = TRUE;
    } else {
        linphoneButton.selected = FALSE;
    }
}

- (void)update {
    switch ([ContactSelection getSelectionMode]) {
        case ContactSelectionModePhone:
        case ContactSelectionModeMessage:
            [addButton setHidden:TRUE];
            [backButton setHidden:FALSE];
            break;
        default:
            [addButton setHidden:FALSE];
            [backButton setHidden:TRUE];
            break;
    }
    if([ContactSelection getSipFilter]) {
        allButton.selected = FALSE;
        linphoneButton.selected = TRUE;
    } else {
        allButton.selected = TRUE;
        linphoneButton.selected = FALSE;   
    }
    [tableController loadData];
}


#pragma mark - Action Functions

- (IBAction)onAllClick:(id)event {
    [self changeView: History_All];
}

- (IBAction)onLinphoneClick:(id)event {
    [self changeView: History_Linphone];
}

- (IBAction)onAddContactClick:(id)event {
    // Go to Contact details view
    ContactDetailsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ContactDetailsViewController compositeViewDescription] push:TRUE], ContactDetailsViewController);
    if(controller != nil) {
        if([ContactSelection getAddAddress] == nil) {
            [controller newContact];
        } else {
            [controller newContact:[ContactSelection getAddAddress]];
        }
    }
}

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popCurrentView];
}

@end
