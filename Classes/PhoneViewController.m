/* PhoneViewController.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
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

#import "PhoneViewController.h"
#import "linphoneAppDelegate.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import "LinphoneManager.h"
#include "FirstLoginViewController.h"


@implementation PhoneViewController
@synthesize  dialerView ;
@synthesize  address ;
@synthesize  callShort;
@synthesize  callLarge;
@synthesize  hangup;
@synthesize status;
@synthesize erase;

@synthesize one;
@synthesize two;
@synthesize three;
@synthesize four;
@synthesize five;
@synthesize six;
@synthesize seven;
@synthesize eight;
@synthesize nine;
@synthesize star;
@synthesize zero;
@synthesize hash;

@synthesize back;
@synthesize myTabBarController;
@synthesize backToCallView;


//implements keypad behavior 
-(IBAction) doKeyPad:(id)sender {
	if (sender == back) {
		if ([address.text length] >0) {
			NSString* newAddress; 
			newAddress = [address.text substringToIndex: [address.text length]-1];
			[address setText:newAddress];
		} 
	} 	
	
}

- (void)viewDidAppear:(BOOL)animated {
	[[UIApplication sharedApplication] setIdleTimerDisabled:true];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"enable_first_login_view_preference"] == true) {
		myFirstLoginViewController = [[FirstLoginViewController alloc]  initWithNibName:@"FirstLoginViewController" 
																				 bundle:[NSBundle mainBundle]];
		[[LinphoneManager instance] setRegistrationDelegate:myFirstLoginViewController];
		[self presentModalViewController:myFirstLoginViewController animated:true];
	}; 
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	
	mDisplayName = [UILabel alloc];
	[zero initWithNumber:'0'  addressField:address dtmf:false];
	[one initWithNumber:'1'  addressField:address dtmf:false];
	[two initWithNumber:'2'  addressField:address dtmf:false];
	[three initWithNumber:'3'  addressField:address dtmf:false];
	[four initWithNumber:'4'  addressField:address dtmf:false];
	[five initWithNumber:'5'  addressField:address dtmf:false];
	[six initWithNumber:'6'  addressField:address dtmf:false];
	[seven initWithNumber:'7'  addressField:address dtmf:false];
	[eight initWithNumber:'8'  addressField:address dtmf:false];
	[nine initWithNumber:'9'  addressField:address dtmf:false];
	[star initWithNumber:'*'  addressField:address dtmf:false];
	[hash initWithNumber:'#'  addressField:address dtmf:false];
	[callShort initWithAddress:address];
	[callLarge initWithAddress:address];
	[erase initWithAddressField:address];
    [backToCallView addTarget:self action:@selector(backToCallViewPressed) forControlEvents:UIControlEventTouchUpInside];
    mIncallViewController = [[IncallViewController alloc]  initWithNibName:@"IncallViewController" 
																	bundle:[NSBundle mainBundle]];

}




/*
 // Override to allow orientations other than the default portrait orientation.
 - (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
 // Return YES for supported orientations
 return (interfaceOrientation == UIInterfaceOrientationPortrait);
 }
 */

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    if (theTextField == address) {
        [address resignFirstResponder];
		[mDisplayName setText:@""]; //display name only relefvant 
		
    } 
    return YES;
}


-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	
	//cancel local notification, just in case
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]  
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground ) {
		// cancel local notif if needed
		[[UIApplication sharedApplication] cancelAllLocalNotifications];
	} else {
		if (mIncomingCallActionSheet) {
			[mIncomingCallActionSheet dismissWithClickedButtonIndex:1 animated:true];
			mIncomingCallActionSheet=nil;
		}
	}
	
	if (username) {
		[address setText:username];
	} //else keep previous
	
	[mDisplayName setText:displayName];
    // disable call button if != Paused
    if (linphone_core_get_calls_nb([LinphoneManager getLc]) == 0) {
        [callLarge setHidden:FALSE];
        [callShort setHidden:TRUE];
        [backToCallView setHidden:TRUE];
    } else {
        [callLarge setHidden:TRUE];
        [callShort setHidden:FALSE];
        [backToCallView setHidden:FALSE];        
    }

	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"firstlogindone_preference" ] == true) {
		//first login case, dismmis first login view																		 
		[self dismissModalViewControllerAnimated:true];
	}; 
	[mIncallViewController displayDialerFromUI:viewCtrl
									   forUser:username
							   withDisplayName:displayName];
	
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	
}

//status reporting
-(void) displayStatus:(NSString*) message {
	[status setText:message];
	[mIncallViewController displayStatus:message];
}


-(void) displayIncomingCall:(LinphoneCall*) call NotificationFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif)
		{
			notif.repeatInterval = 0;
			notif.alertBody =[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),[displayName length]>0?displayName:username];
			notif.alertAction = @"Answer";
			notif.soundName = @"oldphone-mono-30s.caf";
            NSData *callData = [NSData dataWithBytes:&call length:sizeof(call)];
			notif.userInfo = [NSDictionary dictionaryWithObject:callData forKey:@"call"];
			
			[[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
		}
	} else 	{
        CallDelegate* cd = [[CallDelegate alloc] init];
        cd.delegate = self;
        cd.call = call;
        
		mIncomingCallActionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),[displayName length]>0?displayName:username]
															   delegate:cd 
													  cancelButtonTitle:NSLocalizedString(@"Decline",nil) 
												 destructiveButtonTitle:NSLocalizedString(@"Answer",nil) 
													  otherButtonTitles:nil];
        
		mIncomingCallActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
		[mIncomingCallActionSheet showInView:self.parentViewController.view];
		[mIncomingCallActionSheet release];
	}
	
}

-(void) backToCallViewPressed {
    [self	displayInCall: nil
				 FromUI:nil
				forUser:nil 
		withDisplayName:nil];
}

-(void) displayCall: (LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	if (self.presentedViewController != (UIViewController*)mIncallViewController) {
		[self presentModalViewController:(UIViewController*)mIncallViewController animated:true];
	}
	[mIncallViewController displayCall:call InProgressFromUI:viewCtrl
							   forUser:username
					   withDisplayName:displayName];
	
}

-(void) displayInCall: (LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    if (self.presentedViewController != (UIViewController*)mIncallViewController && (call == 0x0 ||
																  linphone_call_get_dir(call)==LinphoneCallIncoming)){
		[self presentModalViewController:(UIViewController*)mIncallViewController animated:true];
		
	}
	

	
	[mIncallViewController displayInCall:call FromUI:viewCtrl
								 forUser:username
						 withDisplayName:displayName];
	[callLarge setHidden:TRUE];
	[callShort setHidden:FALSE];
	[backToCallView setHidden:FALSE];
	
} 


-(void) displayVideoCall:(LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName { 
	[mIncallViewController  displayVideoCall:call FromUI:viewCtrl 
									 forUser:username 
							 withDisplayName:displayName];
}



- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex withUserDatas:(void *)datas{
    LinphoneCall* call = (LinphoneCall*)datas;
	if (buttonIndex == 0 ) {
		linphone_core_accept_call([LinphoneManager getLc],call);	
	} else {
		linphone_core_terminate_call ([LinphoneManager getLc], call);
	}
	mIncomingCallActionSheet = nil;
}

- (void)dealloc {
	[address dealloc];
	[ mDisplayName dealloc];
	[dialerView dealloc];
	[callShort dealloc];
	[callLarge dealloc];
	[hangup dealloc];
	[status dealloc];
	[one dealloc];
	[two dealloc];
	[three dealloc];
	[four dealloc];
	[five dealloc];
	[six dealloc];
	[seven dealloc];
	[eight dealloc];
	[nine dealloc];
	[star dealloc];
	[zero dealloc];
	[hash dealloc];
	[back dealloc];
	[myTabBarController release];
	[mIncallViewController release];
	[super dealloc];
}


@end
