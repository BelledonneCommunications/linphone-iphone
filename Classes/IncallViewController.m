/* IncallViewController.h
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
#import "IncallViewController.h"
#import <AudioToolbox/AudioToolbox.h>
#import "linphonecore.h"



@implementation IncallViewController
@synthesize phoneviewDelegate;

@synthesize controlSubView;
@synthesize padSubView;

@synthesize peerName;
@synthesize peerNumber;
@synthesize callDuration;
@synthesize status;
@synthesize end;
@synthesize close;
@synthesize mute;
@synthesize dialer;
@synthesize speaker;
@synthesize contacts;

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


/*
// The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization

    }
    return self;
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	isMuted = false;
	isSpeaker = false;
	
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
	if (durationRefreasher != nil) {
		[ durationRefreasher invalidate];
	}
	
}

-(void) setLinphoneCore:(LinphoneCore*) lc {
	myLinphoneCore = lc;
}

-(void)displayStatus:(NSString*) message {
	[status setText:message];
}

-(void) startCall {
	const LinphoneAddress* address = linphone_core_get_remote_uri(myLinphoneCore);
	const char* displayName =  linphone_address_get_display_name(address)?linphone_address_get_display_name(address):"";
	[peerName setText:[NSString stringWithCString:displayName length:strlen(displayName)]];
	
	const char* username = linphone_address_get_username(address)!=0?linphone_address_get_username(address):"";
	[peerNumber setText:[NSString stringWithCString:username length:strlen(username)]];
	// start scheduler
	durationRefreasher = [NSTimer scheduledTimerWithTimeInterval:1 
									 target:self 
								   selector:@selector(updateCallDuration) 
								   userInfo:nil 
									repeats:YES];
	
}

-(void)updateCallDuration {
	int lDuration = linphone_core_get_current_call_duration(myLinphoneCore); 
	if (lDuration < 60) {
		[callDuration setText:[NSString stringWithFormat: @"%i s", lDuration]];
	} else {
		[callDuration setText:[NSString stringWithFormat: @"%i:%i", lDuration/60,lDuration - 60 *(lDuration/60)]];
	}
}

- (IBAction)doAction:(id)sender {
	
	if (linphone_core_in_call(myLinphoneCore)) {
		//incall behavior
		if (sender == one) {
			linphone_core_send_dtmf(myLinphoneCore,'1');	
		} else if (sender == two) {
			linphone_core_send_dtmf(myLinphoneCore,'2');	
		} else if (sender == three) {
			linphone_core_send_dtmf(myLinphoneCore,'3');	
		} else if (sender == four) {
			linphone_core_send_dtmf(myLinphoneCore,'4');	
		} else if (sender == five) {
			linphone_core_send_dtmf(myLinphoneCore,'5');	
		} else if (sender == six) {
			linphone_core_send_dtmf(myLinphoneCore,'6');	
		} else if (sender == seven) {
			linphone_core_send_dtmf(myLinphoneCore,'7');	
		} else if (sender == eight) {
			linphone_core_send_dtmf(myLinphoneCore,'8');	
		} else if (sender == nine) {
			linphone_core_send_dtmf(myLinphoneCore,'9');	
		} else if (sender == star) {
			linphone_core_send_dtmf(myLinphoneCore,'*');	
		} else if (sender == zero) {
			linphone_core_send_dtmf(myLinphoneCore,'0');	
		} else if (sender == hash) {
			linphone_core_send_dtmf(myLinphoneCore,'#');	
		}
	}
	
	
	if (sender == end) {
		linphone_core_terminate_call(myLinphoneCore,NULL);
	} else if (sender == dialer) {
		[controlSubView setHidden:true];
		[padSubView setHidden:false];
		
	} else if (sender == contacts) {
		// start people picker
		myPeoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
		[myPeoplePickerController setPeoplePickerDelegate:self];

		[self presentModalViewController: myPeoplePickerController animated:true]; 
	} else if (sender == close) {
		[controlSubView setHidden:false];
		[padSubView setHidden:true];
	} else if (sender == mute) {
		isMuted = isMuted?false:true;
		linphone_core_mute_mic(myLinphoneCore,isMuted);
		// swithc buttun state
		UIImage * tmpImage = [mute backgroundImageForState: UIControlStateNormal];
		[mute setBackgroundImage:[mute backgroundImageForState: UIControlStateHighlighted] forState:UIControlStateNormal];
		[mute setBackgroundImage:tmpImage forState:UIControlStateHighlighted];
		
	} else if (sender == speaker) {
		isSpeaker = isSpeaker?false:true;
		if (isSpeaker) {
			//redirect audio to speaker
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
									 , sizeof (audioRouteOverride)
									 , &audioRouteOverride);
		} else {
			//Cancel audio route redirection
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
									 , sizeof (audioRouteOverride)
									 , &audioRouteOverride);
		}
		// switch button state
		UIImage * tmpImage = [speaker backgroundImageForState: UIControlStateNormal];
		[speaker setBackgroundImage:[speaker backgroundImageForState: UIControlStateHighlighted] forState:UIControlStateNormal];
		[speaker setBackgroundImage:tmpImage forState:UIControlStateHighlighted];
		
	}else  {
		NSLog(@"unknown event from incall view");	
	}
	
}

// handle people picker behavior

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker 
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person {
	return true;
	
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker 
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person 
								property:(ABPropertyID)property 
							  identifier:(ABMultiValueIdentifier)identifier {
	
	return false;
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[self dismissModalViewControllerAnimated:true];
}




- (void)dealloc {
    [super dealloc];
}


@end
