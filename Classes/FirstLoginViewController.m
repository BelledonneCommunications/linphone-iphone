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
                                                 name:@"LinphoneRegistrationUpdate" 
                                               object:nil];
    
	[usernameField setText:[[LinphoneManager instance].settingsStore objectForKey:@"username_preference"]];
	[passwordField setText:[[LinphoneManager instance].settingsStore objectForKey:@"password_preference"]];
    
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
                                                    name:@"LinphoneRegistrationUpdate" 
                                                  object:nil];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
	NSString* siteUrl = [[LinphoneManager instance].settingsStore objectForKey:@"first_login_view_url"];
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
            [[LinphoneManager instance].settingsStore setBool:false forKey:@"enable_first_login_view_preference"]; 
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
			[[LinphoneManager instance].settingsStore setObject:Nil forKey:@"username_preference"];
			[[LinphoneManager instance].settingsStore setObject:Nil forKey:@"password_preference"];
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
		[[LinphoneManager instance].settingsStore setObject:usernameField.text forKey:@"username_preference"];
		[[LinphoneManager instance].settingsStore setObject:passwordField.text forKey:@"password_preference"];
		[self.waitView setHidden:false];
        [[LinphoneManager instance].settingsStore synchronize];
	};
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.
    [theTextField resignFirstResponder];
    return YES;
}

@end
