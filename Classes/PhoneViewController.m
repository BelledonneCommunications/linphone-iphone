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
@synthesize  address ;
@synthesize  call;
@synthesize  hangup;
@synthesize status;

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
	[call setEnabled:true];
	[hangup setEnabled:false];
	
	[callDuration stop];
	
	[peerLabel setText:@""];
	
	[myTabBarController setSelectedIndex:DIALER_TAB_INDEX];
	
}
-(void) displayCallInProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[hangup setEnabled:true];
	if (displayName && [displayName length]>0) {
		[peerLabel setText:displayName];
	} else {
		[peerLabel setText:username?username:@""];
	}
	if (linphone_call_get_state(linphone_core_get_current_call([LinphoneManager getLc])) == LinphoneCallConnected) {
		[callDuration start];
		[callDuration setHidden:false];
	} else {
		[callDuration setText:@"Calling..."];
	}
	
	[address setHidden:true];
	[incallView setHidden:false];
	if (linphone_call_get_dir(linphone_core_get_current_call([LinphoneManager getLc])) == LinphoneCallOutgoing) {
		[call setEnabled:false];
	}
	
}

-(void) displayIncallFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[self displayCallInProgressFromUI:viewCtrl
							  forUser:username
					  withDisplayName:displayName];
}
//status reporting
-(void) displayStatus:(NSString*) message {
	[status setText:message];
	if (myIncallViewController != nil) {
		[myIncallViewController displayStatus:message];
	}
}


-(void) displayIncomingCallNotigicationFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground) {
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif)
		{
			notif.repeatInterval = 0;
			notif.alertBody =[NSString  stringWithFormat:@" %@ is calling you",username];
			notif.alertAction = @"Answer";
			notif.soundName = @"oldphone-mono-30s.caf";
			
			[[UIApplication sharedApplication]  presentLocalNotificationNow:notif];
		}
	} else 	{
		mIncomingCallActionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:@" %@ is calling you",username]
															   delegate:self 
													  cancelButtonTitle:@"Decline" 
												 destructiveButtonTitle:@"Answer" 
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
	[myIncallViewController release];
	[super dealloc];
}


@end
