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

@implementation FirstLoginViewController

@synthesize loginButton;
@synthesize siteButton;
@synthesize usernameField;
@synthesize passwordField;
@synthesize waitView;

- (id)init {
    return [super initWithNibName:@"FirstLoginViewController" bundle:[NSBundle mainBundle]];
}

- (void)viewDidAppear:(BOOL)animated { 
    [super viewDidAppear:animated];
	[usernameField setText:[[LinphoneManager instance].settingsStore objectForKey:@"username_preference"]];
	[passwordField setText:[[LinphoneManager instance].settingsStore objectForKey:@"password_preference"]];
}

- (void)viewDidLoad {
	NSString* siteUrl = [[LinphoneManager instance].settingsStore objectForKey:@"first_login_view_url"];
	if (siteUrl==nil) {
		siteUrl=@"http://www.linphone.org";
	}
	[siteButton setTitle:siteUrl forState:UIControlStateNormal];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(registrationUpdate:) name:@"LinphoneRegistrationUpdate" object:nil];
}

- (void)registrationUpdate: (NSNotification*) notif {  
    LinphoneRegistrationState state = [[notif.userInfo objectForKey: @"state"] intValue];
    switch (state) {
        case LinphoneRegistrationOk: 
        {
            [[LinphoneManager instance].settingsStore setBool:false forKey:@"enable_first_login_view_preference"]; 
            [self.waitView setHidden:true];
            [[LinphoneManager instance] changeView:PhoneView_Dialer];
            break;
        }
        case LinphoneRegistrationNone: 
        case LinphoneRegistrationCleared:
        {
            [self.waitView setHidden:true];	
            break;
        }
        case LinphoneRegistrationFailed: 
        {
            [self.waitView setHidden:true];
            //default behavior if no registration delegates
            
            /*UIAlertView* error = [[UIAlertView alloc] initWithTitle:[NSString stringWithFormat:@"Registration failure for user %@", usernameField.text]
            												message:[notif.userInfo objectForKey: @"message"]
            											   delegate:nil 
            									  cancelButtonTitle:@"Continue" 
            									  otherButtonTitles:nil,nil];
            [error show];
            [error release];*/
            //erase uername passwd
			[[LinphoneManager instance].settingsStore setObject:Nil forKey:@"username_preference"];
			[[LinphoneManager instance].settingsStore setObject:Nil forKey:@"password_preference"];
            break;
        }
        case LinphoneRegistrationProgress: {
            [self.waitView setHidden:false];
            break;
        }
        default: break;
    }
}

- (void)dealloc {
    [super dealloc];
	[loginButton dealloc];
	[siteButton dealloc];
	[usernameField dealloc];
    [passwordField dealloc];
	[waitView dealloc];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

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

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.
    [theTextField resignFirstResponder];
    return YES;
}

@end
