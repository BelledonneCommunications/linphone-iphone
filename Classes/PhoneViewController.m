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
@synthesize  gsmCall;

@synthesize pad;
@synthesize endPhoneNumEditing;
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
@synthesize tun;
@synthesize back;
@synthesize linphoneDelegate;

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

-(void) callLogUpdated:(LinphoneCallLog*) log {
	//nop
}

-(void)dismissIncallView {
	[self dismissModalViewControllerAnimated:true];
}

//implements call/cancel button behavior 
-(IBAction) doAction:(id)sender {
	//1 normalize phone number
	if ([address.text length] == 0) return; //just return
	
	if (sender == gsmCall || sender == call) {
		
		char normalizedUserName[256];
		LinphoneProxyConfig* proxyCfg;	
		//get default proxy
		linphone_core_get_default_proxy(mCore,&proxyCfg);
		NSString* toUserName = [NSString stringWithString:[address text]];
		linphone_proxy_config_normalize_number(proxyCfg,[toUserName cStringUsingEncoding:[NSString defaultCStringEncoding]],normalizedUserName,sizeof(normalizedUserName));
		
		if (sender == call) {
			// check if ready to place a call
			if (!linphone_proxy_config_is_registered(proxyCfg)) {
#ifdef LINPHONE_WIFI_ONLY	
				if (!linphone_proxy_config_register_enabled(proxyCfg)) {
					UIAlertView* error = [[UIAlertView alloc]	initWithTitle:NSLocalizedString(@"Connection interumpida",nil)
																	message:NSLocalizedString(@"Favor de conectarse a una red inalambrica",nil) 
																   delegate:nil 
														  cancelButtonTitle:NSLocalizedString(@"Ok",nil) 
														  otherButtonTitles:nil];
					[error show];
					
				} else {
					[self displayNetworkErrorAlert];
				}
#else
				[self displayNetworkErrorAlert];
#endif /*LINPHONE_WIFI_ONLY*/
				return;
			}
			
			if (!linphone_core_in_call(mCore)) {
				LinphoneAddress* tmpAddress = linphone_address_new(linphone_core_get_identity(mCore));
				linphone_address_set_username(tmpAddress,normalizedUserName);
				linphone_address_set_display_name(tmpAddress,displayName?[displayName cStringUsingEncoding:[NSString defaultCStringEncoding]]:nil);
				linphone_core_invite(mCore,linphone_address_as_string(tmpAddress)) ;
				linphone_address_destroy(tmpAddress);
			} 
			if (linphone_core_inc_invite_pending(mCore)) {
				linphone_core_accept_call(mCore,NULL);	
			}
			//Cancel audio route redirection
			UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_None;  
			
			AudioSessionSetProperty (kAudioSessionProperty_OverrideAudioRoute
									 , sizeof (audioRouteOverride)
									 , &audioRouteOverride);
		} else if (sender == gsmCall) {
			NSURL *url = [NSURL URLWithString:[NSString stringWithFormat:@"tel:%s", normalizedUserName]];
			[[UIApplication sharedApplication] openURL:url];
		} 
		
	} else if (sender == endPhoneNumEditing) {
		[address  resignFirstResponder];
	} else if (sender == tun) {
		[self setTunnelState: [linphoneDelegate toggleTunnel]];
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
				//start timer for back
				[self performSelector:@selector(doBackspaceLongPress) withObject:nil afterDelay:0.5];
				//erase displayname is case of number correction
				displayName=@"";
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
	} if (sender == back) {
		//cancel timer for back
		[NSObject cancelPreviousPerformRequestsWithTarget:self 
												 selector:@selector(doBackspaceLongPress)
												   object:nil];
		
	}else  {
		NSLog(@"unknown up event from dial pad");	
	}
}

-(void)doKeyZeroLongPress {
		NSString* newAddress = [[address.text substringToIndex: [address.text length]-1]  stringByAppendingString:@"+"];
		[address setText:newAddress];

}

-(void)doBackspaceLongPress {
	[address setText:@""];
}

-(void) setLinphoneCore:(LinphoneCore*) lc {
	mCore = lc;
	[myIncallViewController setLinphoneCore:mCore];
	
}
-(void)displayStatus:(NSString*) message {
}

-(void) enableCall:(bool) enable{
	if (enable) {
		[call setImage:[UIImage imageNamed:@"boton_AXTEL_2.png"] forState:UIControlStateNormal];
		[call setImage:[UIImage imageNamed:@"boton_AXTEL_1.png"] forState:UIControlStateHighlighted];
	} else {
		[call setImage:[UIImage imageNamed:@"boton_AXTEL_1.png"] forState:UIControlStateNormal];
		[call setImage:[UIImage imageNamed:@"boton_AXTEL_2.png"] forState:UIControlStateHighlighted];
	}
}
-(void) displayNetworkErrorAlert {
	
	UIAlertView* error = [[UIAlertView alloc]		initWithTitle:NSLocalizedString(@"Connection interumpida",nil)
													message:NSLocalizedString(@"Your phone is no longuer connected, check your connection settings",nil) 
													delegate:nil 
													cancelButtonTitle:NSLocalizedString(@"Ok",nil) 
													otherButtonTitles:nil];
	[error show];
	
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
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWasShown:)
												 name:UIKeyboardDidShowNotification object:nil];
	
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWillHide:)
												 name:UIKeyboardWillHideNotification object:nil];
	
	
	
}

-(void) keyboardWasShown:(NSNotification*)aNotification {
	[pad setHidden:true];
	[back setHidden:true];
}
-(void) keyboardWillHide:(NSNotification*)aNotification {
	[pad setHidden:false];
	[back setHidden:false];
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

/*
- (void)viewWillAppear:(BOOL)animated {
	[self setTunnelState:[linphoneDelegate isTunnel]];
}
*/
-(void) dismissAlertDialog:(UIAlertView*) alertView{
	[alertView dismissWithClickedButtonIndex:0 animated:TRUE];
}

- (void)dealloc {
    [address dealloc];
	[call dealloc];
	[super dealloc];
}

-(void)setTunnelState:(bool) state {
	if (state) {
		[tun setImage:[UIImage imageNamed:@"auroc-On.png"] forState:UIControlStateNormal];
	} else {
		[tun setImage:[UIImage imageNamed:@"auroc-Off.png"] forState:UIControlStateNormal];
	}
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
		case GSTATE_REG_FAILED: {
			[self enableCall:false];
			break;
		}
		case GSTATE_REG_OK: {
			LinphoneProxyConfig* proxyCfg;	
			//get default proxy
			
			if ((linphone_core_get_default_proxy(mCore,&proxyCfg)==0) && linphone_proxy_config_is_registered(proxyCfg)) {
				[self enableCall:true];
			} else {
				[self enableCall:false];
			}
			break;
		}
		case GSTATE_CALL_IN_INVITE:
		case GSTATE_CALL_OUT_INVITE: {
			[myIncallViewController resetView];
			[self presentModalViewController: myIncallViewController animated:true];
			[myIncallViewController displayStatus:NSLocalizedString(@"Llamando...",nil)];
			
			break;
		}
			
		case GSTATE_CALL_ERROR: {
			NSString* lTitle= @"Error";
			NSString* lMessage=state->message!=nil?[NSString stringWithCString:state->message length:strlen(state->message)]: @"";
			
			
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
			displayName=@"";
			break;
		}
		default:
			break;
	}
	
}


@end
