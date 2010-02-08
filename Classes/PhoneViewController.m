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

-(void)setPhoneNumber:(NSString*)number {
	[address setText:number];
	if (displayName) {
		[displayName release];
		displayName=nil;
	}
}
-(void)setPhoneNumber:(NSString*)number withDisplayName:(NSString*) name {
	[self setPhoneNumber:number];
	displayName = name;
}

-(void)dismissIncallView {
	[self dismissModalViewControllerAnimated:true];
}

//implements call/cancel button behavior 
-(IBAction) doAction:(id)sender {
	
	if (sender == call) {
		if (!linphone_core_in_call(mCore)) {
			if ([address.text length] == 0) return; //just return
			if ([address.text hasPrefix:@"sip:"]) {
				linphone_core_invite(mCore, [address.text cStringUsingEncoding:[NSString defaultCStringEncoding]]);
			} else {
				char normalizedUserName[256];
				LinphoneProxyConfig* proxyCfg;	
				//get default proxy
				linphone_core_get_default_proxy(mCore,&proxyCfg);
				NSString* toUserName = [NSString stringWithString:[address text]];
				linphone_proxy_config_normalize_number(proxyCfg,[toUserName cStringUsingEncoding:[NSString defaultCStringEncoding]],normalizedUserName,sizeof(normalizedUserName));
				LinphoneAddress* tmpAddress = linphone_address_new(linphone_core_get_identity(mCore));
				linphone_address_set_username(tmpAddress,normalizedUserName);
				linphone_address_set_display_name(tmpAddress,displayName?[displayName cStringUsingEncoding:[NSString defaultCStringEncoding]]:nil);
				linphone_core_invite(mCore,linphone_address_as_string(tmpAddress)) ;
				linphone_address_destroy(tmpAddress);
			}
		} else if (linphone_core_inc_invite_pending(mCore)) {
			linphone_core_accept_call(mCore,NULL);
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
									 , sizeof (audioRouteOverride)
									 , &audioRouteOverride);
			
		}
		//Cancel audio route redirection
		
	} else if (sender == hangup) {
		linphone_core_terminate_call(mCore,NULL);
	} else if (sender == mute) {
		[self muteAction:!isMuted]; 
		
	} else if (sender == speaker) {
		[self speakerAction:!isSpeaker];		
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
	} else {
			//incall behavior
			if (sender == one) {
				linphone_core_send_dtmf(mCore,'1');	
			} else if (sender == two) {
				linphone_core_send_dtmf(mCore,'2');	
			} else if (sender == three) {
				linphone_core_send_dtmf(mCore,'3');	
			} else if (sender == four) {
				linphone_core_send_dtmf(mCore,'4');	
			} else if (sender == five) {
				linphone_core_send_dtmf(mCore,'5');	
			} else if (sender == six) {
				linphone_core_send_dtmf(mCore,'6');	
			} else if (sender == seven) {
				linphone_core_send_dtmf(mCore,'7');	
			} else if (sender == eight) {
				linphone_core_send_dtmf(mCore,'8');	
			} else if (sender == nine) {
				linphone_core_send_dtmf(mCore,'9');	
			} else if (sender == star) {
				linphone_core_send_dtmf(mCore,'*');	
			} else if (sender == zero) {
				linphone_core_send_dtmf(mCore,'0');	
			} else if (sender == hash) {
				linphone_core_send_dtmf(mCore,'#');	
			} else if (sender == hangup) {
				linphone_core_terminate_call(mCore,NULL);
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
- (void)viewDidAppear:(BOOL)animated {
	[[UIApplication sharedApplication] setIdleTimerDisabled:true];
}
- (void)viewDidDisappear:(BOOL)animated {
	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
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


-(void)updateCallDuration {
	int lDuration = linphone_core_get_current_call_duration(mCore); 
	if (lDuration < 60) {
		[callDuration setText:[NSString stringWithFormat: @"%i s", lDuration]];
	} else {
		[callDuration setText:[NSString stringWithFormat: @"%i:%i", lDuration/60,lDuration - 60 *(lDuration/60)]];
	}
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
			[hangup setEnabled:true];
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
			[self muteAction:false];
			// test if speaker must be unactivated after ring tone
			if (!isSpeaker) [self speakerAction:false];
			
			const LinphoneAddress* callAddress = linphone_core_get_remote_uri(mCore);
			const char* callDisplayName =  linphone_address_get_display_name(callAddress)?linphone_address_get_display_name(callAddress):"";
			if (callDisplayName && callDisplayName[0] != '\000') {
				
			[peerLabel setText:[NSString stringWithCString:callDisplayName length:strlen(callDisplayName)]];
			} else {
				const char* username = linphone_address_get_username(callAddress)!=0?linphone_address_get_username(callAddress):"";
				[peerLabel setText:[NSString stringWithCString:username length:strlen(username)]];
			}
			// start scheduler
			durationRefreasher = [NSTimer scheduledTimerWithTimeInterval:1 
																	target:self 
																	selector:@selector(updateCallDuration) 
																	userInfo:nil 
																	repeats:YES];
			[address setHidden:true];
			[incallView setHidden:false];
			[call setEnabled:false];
			
			break;
		}
			
		case GSTATE_CALL_END: {
			[address setHidden:false];
			[incallView setHidden:true];
			[call setEnabled:true];
			[hangup setEnabled:false];
			
			if (durationRefreasher != nil) {
				[ durationRefreasher invalidate];
				durationRefreasher=nil;
			}
			[peerLabel setText:@""];
			[callDuration setText:@""];

			break;
		}
		default:
			break;
	}
	
}

-(void) muteAction:(bool) value {
	linphone_core_mute_mic(mCore,value);
	if (value) {
		[mute setImage:[UIImage imageNamed:@"mic_muted.png"] forState:UIControlStateNormal];
	} else {
		[mute setImage:[UIImage imageNamed:@"mic_active.png"] forState:UIControlStateNormal];
	}
	isMuted=value;
	// swithc buttun state
};

-(void) speakerAction:(bool) value {
	if (value) {
		//redirect audio to speaker
		UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;  
		AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
								 , sizeof (audioRouteOverride)
								 , &audioRouteOverride);
		[speaker setImage:[UIImage imageNamed:@"Speaker-32-on.png"] forState:UIControlStateNormal];
	} else {
		//Cancel audio route redirection
		UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
		AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
								 , sizeof (audioRouteOverride)
								 , &audioRouteOverride);
		[speaker setImage:[UIImage imageNamed:@"Speaker-32-off.png"] forState:UIControlStateNormal];
	}
	isSpeaker=value;
	
};

@end
