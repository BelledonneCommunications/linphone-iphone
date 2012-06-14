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
#import "IncallViewController.h"
#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>
#import "LinphoneManager.h"
#include "FirstLoginViewController.h"
#include "MainScreenWithVideoPreview.h"
#include "linphonecore.h"
#include "private.h"
#import "PhoneMainView.h"

@implementation PhoneViewController
@synthesize dialerView;
@synthesize mDisplayName;
@synthesize address;
@synthesize callShort;
@synthesize callLarge;
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

@synthesize statusViewHolder;

@synthesize myTabBarController;
@synthesize mMainScreenWithVideoPreview;
@synthesize backToCallView;

@synthesize switchCamera;

- (void)updateCallAndBackButtons {
    @try {
        bool zeroCall = (linphone_core_get_calls_nb([LinphoneManager getLc]) == 0);
        
        [LinphoneManager set:callLarge hidden:!zeroCall withName:"CALL_LARGE button" andReason:__FUNCTION__];
        [LinphoneManager set:switchCamera hidden:!zeroCall withName:"SWITCH_CAM button" andReason:__FUNCTION__];
        [LinphoneManager set:callShort hidden:zeroCall withName:"CALL_SHORT button" andReason:__FUNCTION__];
        [LinphoneManager set:backToCallView hidden:zeroCall withName:"BACK button" andReason:__FUNCTION__];
        
        [callShort setTitle:[UICallButton transforModeEnabled] ? @"transfer":@"call" forState:UIControlStateNormal];
        
        if (!callShort.hidden)
            [callShort setEnabled:!linphone_core_sound_resources_locked([LinphoneManager getLc])];
    } @catch (NSException* exc) {
        // R.A.S: linphone core si simply not ready...
        ms_warning("Catched exception %s: %s", 
                   [exc.name cStringUsingEncoding:[NSString defaultCStringEncoding]], 
                   [exc.reason cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    }
}


- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"enable_first_login_view_preference"] == true) {
		myFirstLoginViewController = [[FirstLoginViewController alloc]  initWithNibName:@"FirstLoginViewController" 
																				 bundle:[NSBundle mainBundle]];
		[self presentModalViewController:myFirstLoginViewController animated:true];
	}
   // [[LinphoneManager instance] setRegistrationDelegate:self];
    
    [mMainScreenWithVideoPreview showPreview:YES];
    [self updateCallAndBackButtons];
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
}


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib : may be called twice
- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(callUpdate:) name:@"LinphoneCallUpdate" object:nil];
    
    [mDisplayName release];
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
    
    /*if (statusSubViewController == nil) {
        statusSubViewController = [[StatusSubViewController alloc]  initWithNibName:@"StatusSubViewController" 
                                                                         bundle:[NSBundle mainBundle]];
        [statusViewHolder addSubview:statusSubViewController.view];
    }*/
    
    [self updateCallAndBackButtons];
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL)textFieldShouldReturn:(UITextField *)theTextField {
    if (theTextField == address) {
        [address resignFirstResponder];
		[mDisplayName setText:@""]; //display name only relefvant 
		
    } 
    return YES;
}

- (void)viewWillAppear:(BOOL)animated {
    [self updateCallAndBackButtons];
    [super viewWillAppear:animated];
}

- (void)callUpdate: (NSNotification*) notif {  
    LinphoneCallWrapper *callWrapper = [notif.userInfo objectForKey: @"call"];
    LinphoneCall *call = callWrapper->call;
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    
    const char* lUserNameChars=linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    const char* lDisplayNameChars =  linphone_address_get_display_name(linphone_call_get_remote_address(call));        
	NSString* lDisplayName = [lDisplayNameChars?[[NSString alloc] initWithUTF8String:lDisplayNameChars]:@"" autorelease];
    
    bool canHideInCallView = (linphone_core_get_calls([LinphoneManager getLc]) == NULL);
    
	switch (state) {					
		case LinphoneCallIncomingReceived: 
			[self	displayIncomingCall:call 
                           NotificationFromUI:nil
                                      forUser:lUserName 
                              withDisplayName:lDisplayName];
			break;
			
		case LinphoneCallOutgoingInit: 
			[self		displayCall:call 
                      InProgressFromUI:nil
                               forUser:lUserName 
                       withDisplayName:lDisplayName];
			break;
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
			[self	displayInCall: call 
                                 FromUI:nil
                                forUser:lUserName 
                        withDisplayName:lDisplayName];
			break;
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);
            
            /* remote wants to add video */
            if (!linphone_call_params_video_enabled(current) && 
                linphone_call_params_video_enabled(remote) && 
                !linphone_core_get_video_policy([LinphoneManager getLc])->automatically_accept) {
                linphone_core_defer_call_update([LinphoneManager getLc], call);
                [self displayAskToEnableVideoCall:call forUser:lUserName withDisplayName:lDisplayName];
            } else if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [self displayInCall:call FromUI:nil forUser:lUserName withDisplayName:lDisplayName];
            }
            break;
        }
        case LinphoneCallUpdated:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            if (linphone_call_params_video_enabled(current)) {
                [self displayVideoCall:call FromUI:nil forUser:lUserName withDisplayName:lDisplayName];
            } else {
                [self displayInCall:call FromUI:nil forUser:lUserName withDisplayName:lDisplayName];
            }
            break;
            
        }
		case LinphoneCallError: { 
            if (canHideInCallView) {
                [self	displayDialerFromUI:nil
                                          forUser:@"" 
                                  withDisplayName:@""];
            } else {
				[self	displayInCall:call 
									 FromUI:nil
									forUser:lUserName 
							withDisplayName:lDisplayName];	
			}
			break;
		}
		case LinphoneCallEnd:
            if (canHideInCallView) {
                [self	displayDialerFromUI:nil
                                          forUser:@"" 
                                  withDisplayName:@""];
            } else {
				[self	displayInCall:call 
									 FromUI:nil
									forUser:lUserName 
							withDisplayName:lDisplayName];	
			}
			break;
		case LinphoneCallStreamsRunning:
			//check video
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				[self	displayVideoCall:call FromUI:nil
                                       forUser:lUserName 
                               withDisplayName:lDisplayName];
			} else {
                [self displayInCall:call FromUI:nil forUser:lUserName withDisplayName:lDisplayName];
            }
			break;
        default:
            break;
	}

}

- (void)displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	
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

    [self updateCallAndBackButtons];

	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"firstlogindone_preference" ] == true) {
		//first login case, dismmis first login view																		 
		[self dismissModalViewControllerAnimated:true];
	}; 
	
    [[LinphoneManager instance] changeView:PhoneView_Dialer];
    
    [mMainScreenWithVideoPreview showPreview:YES];
}

- (void)displayIncomingCall:(LinphoneCall*) call NotificationFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[mMainScreenWithVideoPreview showPreview:NO]; 
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
        cd.eventType = CD_NEW_CALL;
        cd.delegate = self;
        cd.call = call;
        
		mIncomingCallActionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),[displayName length]>0?displayName:username]
															   delegate:cd 
													  cancelButtonTitle:nil 
												 destructiveButtonTitle:NSLocalizedString(@"Answer",nil) 
													  otherButtonTitles:NSLocalizedString(@"Decline",nil),nil];
        
		mIncomingCallActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
        if ([LinphoneManager runningOnIpad]) {
            if (self.modalViewController != nil)
                [mIncomingCallActionSheet showInView:[self.modalViewController view]];
            else
                [mIncomingCallActionSheet showInView:self.parentViewController.view];
        } else {
            [mIncomingCallActionSheet showInView:self.parentViewController.view];
        }
		[mIncomingCallActionSheet release];
	}
	
    [mMainScreenWithVideoPreview showPreview:NO];
}

- (void)backToCallViewPressed {
    [UICallButton enableTransforMode:NO];
    [[LinphoneManager instance] changeView:PhoneView_InCall];

    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    
    if (!call || !linphone_call_params_video_enabled(linphone_call_get_current_params(call)) || linphone_call_get_state(call) != LinphoneCallStreamsRunning) {
        [self	displayInCall: call
				 FromUI:nil
				forUser:nil 
		withDisplayName:nil];
    } else {
        [self displayVideoCall:call FromUI:nil forUser:nil withDisplayName:nil];
    }
}

- (void)displayCall: (LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [mMainScreenWithVideoPreview showPreview:NO]; 
	if ([[LinphoneManager instance]currentView] != PhoneView_InCall) {
		[[LinphoneManager instance] changeView:PhoneView_InCall];
	}
    
    [mMainScreenWithVideoPreview showPreview:NO];
}

- (void)displayInCall: (LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    [mMainScreenWithVideoPreview showPreview:NO]; 
    if ([[LinphoneManager instance]currentView] != PhoneView_InCall && (call == 0x0 ||
																  linphone_call_get_dir(call)==LinphoneCallIncoming)){
		[[LinphoneManager instance] changeView:PhoneView_InCall];
	}
    
    [LinphoneManager set:callLarge hidden:YES withName:"CALL_LARGE button" andReason:__FUNCTION__];
    [LinphoneManager set:switchCamera hidden:YES withName:"SWITCH_CAMERA button" andReason:__FUNCTION__];
    [LinphoneManager set:callShort hidden:NO withName:"CALL_SHORT button" andReason:__FUNCTION__];
    [LinphoneManager set:backToCallView hidden:NO withName:"CALL_BACK button" andReason:__FUNCTION__];
    
    [self updateCallAndBackButtons];
} 


- (void)displayVideoCall:(LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName { 
    [mMainScreenWithVideoPreview showPreview:NO]; 
    
    [mMainScreenWithVideoPreview showPreview:NO];
    [self updateCallAndBackButtons];
}

- (void)displayAskToEnableVideoCall:(LinphoneCall*) call forUser:(NSString*) username withDisplayName:(NSString*) displayName {
}

- (void)actionSheet:(UIActionSheet *)actionSheet ofType:(enum CallDelegateType)type clickedButtonAtIndex:(NSInteger)buttonIndex withUserDatas:(void *)datas {
    if (type != CD_NEW_CALL)
        return;
    
    LinphoneCall* call = (LinphoneCall*)datas;
	if (buttonIndex == actionSheet.destructiveButtonIndex ) {
		linphone_core_accept_call([LinphoneManager getLc],call);	
	} else {
		linphone_core_terminate_call ([LinphoneManager getLc], call);
	}
	mIncomingCallActionSheet = nil;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
	[address dealloc];
	[mDisplayName dealloc];
	[dialerView dealloc];
	[callShort dealloc];
	[callLarge dealloc];
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
	[myTabBarController release];
	[super dealloc];
}

- (void)displayRegisteredFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain {    
    if (myFirstLoginViewController != nil && self.modalViewController == myFirstLoginViewController) {
        //[myFirstLoginViewController displayRegisteredFromUI:viewCtrl forUser:username withDisplayName:displayName onDomain:domain];
    }
}

- (void)displayRegisteringFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName onDomain:(NSString*)domain {
    if (myFirstLoginViewController != nil && self.modalViewController == myFirstLoginViewController) {
        //[myFirstLoginViewController displayRegisteringFromUI:viewCtrl forUser:username withDisplayName:displayName onDomain:domain];
    }
}

- (void)displayRegistrationFailedFromUI:(UIViewController*) viewCtrl forUser:(NSString*) user withDisplayName:(NSString*) displayName onDomain:(NSString*)domain forReason:(NSString*) reason {
    if (myFirstLoginViewController != nil && self.modalViewController == myFirstLoginViewController) {
        //[myFirstLoginViewController displayRegistrationFailedFromUI:viewCtrl forUser:user withDisplayName:displayName onDomain:domain forReason:reason];
    }
}

- (void)displayNotRegisteredFromUI:(UIViewController*) viewCtrl { 
    if (myFirstLoginViewController != nil && self.modalViewController == myFirstLoginViewController) {
        //[myFirstLoginViewController displayNotRegisteredFromUI:viewCtrl];
    }
}

- (void)firstVideoFrameDecoded: (LinphoneCall*) call {
  //  [mIncallViewController firstVideoFrameDecoded:call];
}

- (IBAction)onAddContact: (id) event {
    
}


@end
