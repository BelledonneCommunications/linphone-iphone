/* FirstLoginViewController.m
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

#import "LinphoneManager.h"
#import "FirstLoginViewController.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation FirstLoginViewController

@synthesize loginButton;
@synthesize siteButton;
@synthesize usernameField;
@synthesize passwordField;
@synthesize waitView;

#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"FirstLoginViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
	[loginButton release];
	[siteButton release];
	[usernameField release];
    [passwordField release];
	[waitView release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"FirstLogin" 
                                                                content:@"FirstLoginViewController" 
                                                               stateBar:nil 
                                                        stateBarEnabled:false 
                                                                 tabBar:nil 
                                                          tabBarEnabled:false 
                                                             fullscreen:false
                                                          landscapeMode:false
                                                           portraitMode:true];
    }
    return compositeDescription;
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated { 
    [super viewWillAppear:animated];
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(registrationUpdateEvent:) 
                                                 name:kLinphoneRegistrationUpdate
                                               object:nil];
    
	
	[usernameField setText:[[LinphoneManager instance] lpConfigStringForKey:@"wizard_username"]];
	[passwordField setText:[[LinphoneManager instance] lpConfigStringForKey:@"wizard_password"]];
    
    // Update on show
    const MSList* list = linphone_core_get_proxy_config_list([LinphoneManager getLc]);
    if(list != NULL) {
        LinphoneProxyConfig *config = (LinphoneProxyConfig*) list->data;
        if(config) {
            [self registrationUpdate:linphone_proxy_config_get_state(config)];
        }
    }
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:kLinphoneRegistrationUpdate
                                                  object:nil];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
	NSString* siteUrl = [[LinphoneManager instance] lpConfigStringForKey:@"first_login_view_url"];
	if (siteUrl==nil) {
		siteUrl=@"http://www.linphone.org";
	}
	[siteButton setTitle:siteUrl forState:UIControlStateNormal];
}

#pragma mark - Event Functions

- (void)registrationUpdateEvent:(NSNotification*)notif {  
    [self registrationUpdate:[[notif.userInfo objectForKey: @"state"] intValue]];
}


#pragma mark - 

- (void)registrationUpdate:(LinphoneRegistrationState)state {
    switch (state) {
        case LinphoneRegistrationOk: {
            [[LinphoneManager instance] lpConfigSetBool:FALSE forKey:@"enable_first_login_view_preference"]; 
            [waitView setHidden:true];
            [[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
            break;
        }
        case LinphoneRegistrationNone: 
        case LinphoneRegistrationCleared: {
            [waitView setHidden:true];	
            break;
        }
        case LinphoneRegistrationFailed: {
            [waitView setHidden:true];
            
            //erase uername passwd
			[[LinphoneManager instance] lpConfigSetString:nil forKey:@"wizard_username"];
			[[LinphoneManager instance] lpConfigSetString:nil forKey:@"wizard_password"];
            break;
        }
        case LinphoneRegistrationProgress: {
            [waitView setHidden:false];
            break;
        }
        default: break;
    }
}

#pragma mark - Action Functions

- (void)onSiteClick:(id)sender {
    NSURL *url = [NSURL URLWithString:siteButton.titleLabel.text];
    [[UIApplication sharedApplication] openURL:url];
    return;
}

- (void)onLoginClick:(id)sender {
	NSString* errorMessage=nil;
	if ([usernameField.text length]==0 ) {
		errorMessage=NSLocalizedString(@"Enter your username",nil);
	}  else if ([passwordField.text length]==0 ) {
		errorMessage=NSLocalizedString(@"Enter your password",nil);
	} 

	if (errorMessage != nil) {
		UIAlertView* error=nil;
		error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
										   message:errorMessage 
										  delegate:nil 
								 cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
								 otherButtonTitles:nil];
		[error show];
        [error release];
	} else {
		linphone_core_clear_all_auth_info([LinphoneManager getLc]);
		linphone_core_clear_proxy_config([LinphoneManager getLc]);
		LinphoneProxyConfig* proxyCfg = linphone_core_create_proxy_config([LinphoneManager getLc]);
		/*default domain is supposed to be preset from linphonerc*/
		NSString* identity = [NSString stringWithFormat:@"sip:%@@%s",usernameField.text, linphone_proxy_config_get_addr(proxyCfg)];
		linphone_proxy_config_set_identity(proxyCfg,[identity UTF8String]);
		LinphoneAuthInfo* auth_info =linphone_auth_info_new([usernameField.text UTF8String]
															,[usernameField.text UTF8String]
															,[passwordField.text UTF8String]
															,NULL
															,NULL);
		linphone_core_add_auth_info([LinphoneManager getLc], auth_info);
		linphone_core_add_proxy_config([LinphoneManager getLc], proxyCfg);
		linphone_core_set_default_proxy([LinphoneManager getLc], proxyCfg);
		[self.waitView setHidden:false];
	};
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.
    [theTextField resignFirstResponder];
    return YES;
}

@end
