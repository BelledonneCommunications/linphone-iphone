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
#import "osip2/osip.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>




@implementation PhoneViewController
@synthesize  address ;
@synthesize  call;
@synthesize status;

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

-(void)setPhoneNumber:(NSString*)number {
	[address setText:number];
}

-(void)dismissIncallView {
	[self dismissModalViewControllerAnimated:true];
}

//implements call/cancel button behavior 
-(IBAction) doAction:(id)sender {
	
	if (sender == call) {
		if (!linphone_core_in_call(mCore)) {
			const char* lCallee = [[address text]  cStringUsingEncoding:[NSString defaultCStringEncoding]];
			linphone_core_invite(mCore,lCallee) ;		
		} 
		if (linphone_core_inc_invite_pending(mCore)) {
			linphone_core_accept_call(mCore,NULL);	
		}
		//Cancel audio route redirection
		UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
		
		AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
					, sizeof (audioRouteOverride)
					, &audioRouteOverride);
		
	
	} 

}

//implements keypad behavior 
-(IBAction) doKeyPad:(id)sender {
	if (!linphone_core_in_call(mCore)) {
		//outcall behavior	
		//remove sip: if first digits
		if ([address.text isEqualToString:@"sip:"]) {
			[address setText:@""];
		}
		NSString* newAddress = nil;
		if (sender == one) {
			newAddress = [address.text stringByAppendingString:@"1"];
		} else if (sender == two) {
			newAddress = [address.text stringByAppendingString:@"2"];
		} else if (sender == three) {
			newAddress = [address.text stringByAppendingString:@"3"];
		} else if (sender == four) {
			newAddress = [address.text stringByAppendingString:@"4"];
		} else if (sender == five) {
			newAddress = [address.text stringByAppendingString:@"5"];
		} else if (sender == six) {
			newAddress = [address.text stringByAppendingString:@"6"];
		} else if (sender == seven) {
			newAddress = [address.text stringByAppendingString:@"7"];
		} else if (sender == eight) {
			newAddress = [address.text stringByAppendingString:@"8"];
		} else if (sender == nine) {
			newAddress = [address.text stringByAppendingString:@"9"];
		} else if (sender == star) {
			newAddress = [address.text stringByAppendingString:@"*"];
		} else if (sender == zero) {
			newAddress = [address.text stringByAppendingString:@"0"];
			//start timer for +
			[self performSelector:@selector(doKeyZeroLongPress) withObject:nil afterDelay:0.5];
		} else if (sender == hash) {
			newAddress = [address.text stringByAppendingString:@"#"];
		} else if (sender == back) {
			if ([address.text length] >0) {
				newAddress = [address.text substringToIndex: [address.text length]-1];
			} 
		} else  {
			NSLog(@"unknown event from diad pad");
			return;
		}
		if (newAddress != nil) {
			[address setText:newAddress];	
		}
	}
}

//implements keypad up  
-(IBAction) doKeyPadUp:(id)sender {
	if (sender == zero) {
		//cancel timer for +
		[NSObject cancelPreviousPerformRequestsWithTarget:self 
												 selector:@selector(doKeyZeroLongPress)
												   object:nil];
	} else  {
		NSLog(@"unknown up event from dial pad");	
	}
}

-(void)doKeyZeroLongPress {
		NSString* newAddress = [[address.text substringToIndex: [address.text length]-1]  stringByAppendingString:@"+"];
		[address setText:newAddress];

}

-(void) setLinphoneCore:(LinphoneCore*) lc {
	mCore = lc;
	[myIncallViewController setLinphoneCore:mCore];
}
-(void)displayStatus:(NSString*) message {
	[status setText:message];
	if (myIncallViewController != nil) {
		[myIncallViewController displayStatus:message];
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


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	if (myIncallViewController == nil) {
		myIncallViewController = [[IncallViewController alloc] initWithNibName:@"IncallViewController" bundle:[NSBundle mainBundle]];
		[myIncallViewController setPhoneviewDelegate:self];
		
	}
	
	
	
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
    }
    return YES;
}



-(void) dismissAlertDialog:(UIAlertView*) alertView{
	[alertView dismissWithClickedButtonIndex:0 animated:TRUE];
}

- (void)dealloc {
    [address dealloc];
	[call dealloc];
	[status dealloc];
	[super dealloc];
}

-(void) callStateChange:(LinphoneGeneralState*) state {
	//	/* states for GSTATE_GROUP_POWER */
	//	GSTATE_POWER_OFF = 0,        /* initial state */
	//	GSTATE_POWER_STARTUP,
	//	GSTATE_POWER_ON,
	//	GSTATE_POWER_SHUTDOWN,
	//	/* states for GSTATE_GROUP_REG */
	//	GSTATE_REG_NONE = 10,       /* initial state */
	//	GSTATE_REG_OK,
	//	GSTATE_REG_FAILED,
	//	/* states for GSTATE_GROUP_CALL */
	//	GSTATE_CALL_IDLE = 20,      /* initial state */
	//	GSTATE_CALL_OUT_INVITE,
	//	GSTATE_CALL_OUT_CONNECTED,
	//	GSTATE_CALL_IN_INVITE,
	//	GSTATE_CALL_IN_CONNECTED,
	//	GSTATE_CALL_END,
	//	GSTATE_CALL_ERROR,
	//	GSTATE_INVALID
	switch (state->new_state) {
		case GSTATE_CALL_IN_INVITE:
		case GSTATE_CALL_OUT_INVITE: {
			//[myIncallViewController startCall];
			[self presentModalViewController: myIncallViewController animated:true];
			break;
		}
			
		case GSTATE_CALL_ERROR: {
			NSString* lTitle= state->message!=nil?[NSString stringWithCString:state->message length:strlen(state->message)]: @"Error";
			NSString* lMessage=lTitle;
			
			
			UIAlertView* error = [[UIAlertView alloc] initWithTitle:lTitle
															message:lMessage 
														   delegate:nil 
												  cancelButtonTitle:nil 
												  otherButtonTitles:nil];
			[error show];
			[self performSelector:@selector(dismissAlertDialog:) withObject:error afterDelay:1];
			[self performSelector:@selector(dismissIncallView) withObject:nil afterDelay:1];
			
		}
			break;
		case GSTATE_CALL_IN_CONNECTED:
		case GSTATE_CALL_OUT_CONNECTED: {
			[myIncallViewController startCall];
			break;
		}
			
		case GSTATE_CALL_END: {
			//end off call, just dismiss Incall view
			[self dismissIncallView];
			break;
		}
		default:
			break;
	}
	
}


@end
