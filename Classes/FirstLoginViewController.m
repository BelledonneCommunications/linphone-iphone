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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */          


#import "FirstLoginViewController.h"
#import "LinphoneManager.h"

@implementation FirstLoginViewController

@synthesize ok;
@synthesize username;
@synthesize passwd;
@synthesize activityIndicator;
@synthesize site;



- (void)viewDidAppear:(BOOL)animated { 
    [super viewDidAppear:animated];
	//[username setText:[[NSUserDefaults standardUserDefaults] stringForKey:@"username_preference"]];
	//[passwd setText:[[NSUserDefaults standardUserDefaults] stringForKey:@"password_preference"]];
}



- (void)dealloc {
    [super dealloc];
	[ok dealloc];
	[site dealloc];
	[username dealloc];
	[activityIndicator dealloc];
}



-(void) doOk:(id)sender {
	if (sender == site) {
		NSURL *url = [NSURL URLWithString:@"http://www.linphone.org"];
		[[UIApplication sharedApplication] openURL:url];
		return;
	}
	NSString* errorMessage=nil;
	if ([username.text length]==0 ) {
		errorMessage=NSLocalizedString(@"Enter your username",nil);
	}  else if ([passwd.text length]==0 ) {
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
	} else {
		[[NSUserDefaults standardUserDefaults] setObject:username.text forKey:@"username_preference"];
		[[NSUserDefaults standardUserDefaults] setObject:passwd.text forKey:@"password_preference"];
		[self.activityIndicator setHidden:false];
	};
	
	
}
-(void) displayRegisteredFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain {
	[[NSUserDefaults standardUserDefaults] setBool:false forKey:@"enable_first_login_view_preference"]; 
	[self.parentViewController dismissModalViewControllerAnimated:true];
	[self.activityIndicator setHidden:true];
	[[LinphoneManager instance] setRegistrationDelegate:nil]; 
}
-(void) displayRegisteringFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain {
	[self.activityIndicator setHidden:false];
}
-(void) displayRegistrationFailedFromUI:(UIViewController*) viewCtrl forUser:(NSString*) user withDisplayName:(NSString*) displayName onDomain:(NSString*)domain forReason:(NSString*) reason {
	[self.activityIndicator setHidden:true];
	//default behavior if no registration delegates
	
	UIAlertView* error = [[UIAlertView alloc]	initWithTitle:[NSString stringWithFormat:@"Registration failure for user %@",user]
													message:reason
												   delegate:nil 
										  cancelButtonTitle:@"Continue" 
										  otherButtonTitles:nil ,nil];
	[error show];
	
}
-(void) displayNotRegisteredFromUI:(UIViewController*) viewCtrl { 
	[self.activityIndicator setHidden:true];	
}


- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    // When the user presses return, take focus away from the text field so that the keyboard is dismissed.
    [theTextField resignFirstResponder];
    return YES;
}


@end
