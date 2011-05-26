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
 *  GNU Library General Public License for more details.                
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



@implementation PhoneViewController
@synthesize  dialerView ;
@synthesize  address ;
@synthesize  call;
@synthesize  hangup;
@synthesize status;
@synthesize erase;


@synthesize incallView;
@synthesize callDuration;
@synthesize mute;
@synthesize speaker;	
@synthesize peerLabel;

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





/*
 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
 - (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
 if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
 }
 
 return self;
 }
 */


- (void)viewDidAppear:(BOOL)animated {
	[[UIApplication sharedApplication] setIdleTimerDisabled:true];
	[mute reset];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"enable_first_login_view_preference"] == true) {
		myFirstLoginViewController = [[FirstLoginViewController alloc]  initWithNibName:@"FirstLoginViewController" 
																				 bundle:[NSBundle mainBundle]];
		[[LinphoneManager instance] setRegistrationDelegate:myFirstLoginViewController];
		[self presentModalViewController:myFirstLoginViewController animated:true];
	}; 
}

- (void)viewDidDisappear:(BOOL)animated {
	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	
	mDisplayName = [UILabel alloc];
	[zero initWithNumber:'0'  addressField:address ];
	[one initWithNumber:'1'  addressField:address ];
	[two initWithNumber:'2'  addressField:address ];
	[three initWithNumber:'3'  addressField:address ];
	[four initWithNumber:'4'  addressField:address ];
	[five initWithNumber:'5'  addressField:address ];
	[six initWithNumber:'6'  addressField:address ];
	[seven initWithNumber:'7'  addressField:address ];
	[eight initWithNumber:'8'  addressField:address ];
	[nine initWithNumber:'9'  addressField:address ];
	[star initWithNumber:'*'  addressField:address ];
	[hash initWithNumber:'#'  addressField:address ];
	[call initWithAddress:address withDisplayName:mDisplayName];
	[mute initWithOnImage:[UIImage imageNamed:@"mic_muted.png"]  offImage:[UIImage imageNamed:@"mic_active.png"] ];
	[speaker initWithOnImage:[UIImage imageNamed:@"Speaker-32-on.png"]  offImage:[UIImage imageNamed:@"Speaker-32-off.png"] ];
	[erase initWithAddressField:address];
	
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
	
	[address setHidden:false];
	if (username) {
		[address setText:username];
	} //else keep previous
	
	[mDisplayName setText:displayName];
	[incallView setHidden:true];
	[dialerView setHidden:false];
	
	[call setEnabled:true];
	[hangup setEnabled:false];

	[callDuration stop];
    UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = NO;

	
	[peerLabel setText:@""];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"firstlogindone_preference" ] == true) {
		//first login case, dismmis first login view																		 
		[self dismissModalViewControllerAnimated:true];
	}; 
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	
}
-(void) displayIncalViewforUser:(NSString*) username withDisplayName:(NSString*) displayName {
    UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
    if (device.proximityMonitoringEnabled == YES) {
        ms_message("Ok this device support proximity, and I just enabled it");
    }

    [hangup setEnabled:true];
	if (displayName && [displayName length]>0) {
		[peerLabel setText:displayName];
	} else {
		[peerLabel setText:username?username:@""];
	}
	[address setHidden:true];
	[incallView setHidden:false];
	[dialerView setHidden:true];
}
-(void) displayCallInProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[self displayIncalViewforUser:username
				  withDisplayName:displayName];
	[call setEnabled:false];
	[callDuration setText:NSLocalizedString(@"Calling...",nil)];
	[speaker reset];
}

-(void) displayIncallFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[callDuration start];
	[callDuration setHidden:false];

	if (linphone_call_get_dir(linphone_core_get_current_call([LinphoneManager getLc])) == LinphoneCallIncoming) {
		[self displayIncalViewforUser:username
					  withDisplayName:displayName];
		[speaker reset];
	} 
}
//status reporting
-(void) displayStatus:(NSString*) message {
	[status setText:message];
}


-(void) displayIncomingCallNotigicationFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground) {
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif)
		{
			notif.repeatInterval = 0;
			notif.alertBody =[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),displayName?displayName:username];
			notif.alertAction = @"Answer";
			notif.soundName = @"oldphone-mono-30s.caf";
			
			[[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
		}
	} else 	{
		mIncomingCallActionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),displayName?displayName:username]
															   delegate:self 
													  cancelButtonTitle:NSLocalizedString(@"Decline",nil) 
												 destructiveButtonTitle:NSLocalizedString(@"Answer",nil) 
													  otherButtonTitles:nil];
		mIncomingCallActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
		[mIncomingCallActionSheet showInView:self.view];
		[mIncomingCallActionSheet release];
	}
	
}
- (void)actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 0 ) {
		linphone_core_accept_call([LinphoneManager getLc],linphone_core_get_current_call([LinphoneManager getLc]));	
	} else {
		linphone_core_terminate_call ([LinphoneManager getLc],linphone_core_get_current_call([LinphoneManager getLc]));
	}
	mIncomingCallActionSheet = nil;
}

- (void)dealloc {
	[address dealloc];
	[ mDisplayName dealloc];
	[incallView dealloc];
	[dialerView dealloc];
	[callDuration dealloc];
	[mute dealloc];
	[speaker dealloc];	
	[peerLabel dealloc];
	[call dealloc];
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
	[super dealloc];
}


@end
