/* PhoneMainView.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import <QuartzCore/QuartzCore.h>

#import "PhoneMainView.h"
#import "DialerViewController.h"
#import "HistoryViewController.h"
#import "ContactsViewController.h"
#import "IncomingCallViewController.h"
#import "InCallViewController.h"
#import "SettingsViewController.h"
#import "ChatViewController.h"

#import "AbstractCall.h"

@implementation PhoneMainView

@synthesize mainViewController;
@synthesize modalViewController;

- (void)myInit {
    self->currentPhoneView = -1;
}

- (id)init {
    self = [super init];
    if (self) {
		[self myInit];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self myInit];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self myInit];
	}
    return self;
}	

- (void)changeView: (NSNotification*) notif {   
    NSNumber *viewId = [notif.userInfo objectForKey: @"view"];
    NSNumber *tabBar = [notif.userInfo objectForKey: @"tabBar"];
    NSNumber *fullscreen = [notif.userInfo objectForKey: @"fullscreen"];
    
    // Check view change
    if(viewId != nil) {
        PhoneView view = [viewId intValue];
        UICompositeViewDescription* description = [viewDescriptions objectForKey:[NSNumber numberWithInt: view]];
        if(description == nil)
            return;
        if(view != currentPhoneView) {
            [mainViewController changeView:description];
            currentPhoneView = view;
        } 
    }
    
    if(tabBar != nil) {
        [mainViewController setToolBarHidden:![tabBar boolValue]];
    }
    
    if(fullscreen != nil) {
        [mainViewController setFullScreen:[fullscreen boolValue]];
    }
    
    // Call abstractCall
    NSDictionary *dict = [notif.userInfo objectForKey: @"args"];
    if(dict != nil)
        [AbstractCall call:[mainViewController getCurrentViewController] dict:dict];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [[self view] addSubview: mainViewController.view];
    
    // Init view descriptions
    viewDescriptions = [[NSMutableDictionary alloc] init];
    
    //
    // Main View
    //
    UICompositeViewDescription *dialerDescription = [UICompositeViewDescription alloc];
    dialerDescription->content = @"DialerViewController";
    dialerDescription->tabBar = @"UIMainBar";
    dialerDescription->tabBarEnabled = true;
    dialerDescription->stateBar = @"UIStateBar";
    dialerDescription->stateBarEnabled = true;
    dialerDescription->fullscreen = false;
    [viewDescriptions setObject:dialerDescription forKey:[NSNumber numberWithInt: PhoneView_Dialer]];
    
    
    //
    // Contacts View
    //
    UICompositeViewDescription *contactsDescription = [UICompositeViewDescription alloc];
    contactsDescription->content = @"ContactsViewController";
    contactsDescription->tabBar = @"UIMainBar";
    contactsDescription->tabBarEnabled = true;
    contactsDescription->stateBar = nil;
    contactsDescription->stateBarEnabled = false;
    contactsDescription->fullscreen = false;
    [viewDescriptions setObject:contactsDescription forKey:[NSNumber numberWithInt: PhoneView_Contacts]];
    
    
    //
    // Call History View
    //
    UICompositeViewDescription *historyDescription = [UICompositeViewDescription alloc];
    historyDescription->content = @"HistoryViewController";
    historyDescription->tabBar = @"UIMainBar";
    historyDescription->tabBarEnabled = true;
    historyDescription->stateBar = nil;
    historyDescription->stateBarEnabled = false;
    historyDescription->fullscreen = false;
    [viewDescriptions setObject:historyDescription forKey:[NSNumber numberWithInt: PhoneView_History]];
    
    //
    // IncomingCall View
    //
    UICompositeViewDescription *incomingCallDescription = [UICompositeViewDescription alloc];
    incomingCallDescription->content = @"IncomingCallViewController";
    incomingCallDescription->tabBar = nil;
    incomingCallDescription->tabBarEnabled = false;
    incomingCallDescription->stateBar = @"UIStateBar";
    incomingCallDescription->stateBarEnabled = true;
    incomingCallDescription->fullscreen = false;
    [viewDescriptions setObject:incomingCallDescription forKey:[NSNumber numberWithInt: PhoneView_IncomingCall]];
    
    //
    // InCall View
    //
    UICompositeViewDescription *inCallDescription = [UICompositeViewDescription alloc];
    inCallDescription->content = @"InCallViewController";
    inCallDescription->tabBar = @"UICallBar";
    inCallDescription->tabBarEnabled = true;
    inCallDescription->stateBar = @"UIStateBar";
    inCallDescription->stateBarEnabled = true;
    inCallDescription->fullscreen = false;
    [viewDescriptions setObject:inCallDescription forKey:[NSNumber numberWithInt: PhoneView_InCall]];
    
    
    //
    // Settings View
    //
    UICompositeViewDescription *settingsDescription = [UICompositeViewDescription alloc];
    settingsDescription->content = @"SettingsViewController";
    settingsDescription->tabBar = @"UIMainBar";
    settingsDescription->tabBarEnabled = true;
    settingsDescription->stateBar = nil;
    settingsDescription->stateBarEnabled = false;
    settingsDescription->fullscreen = false;
    [viewDescriptions setObject:settingsDescription forKey:[NSNumber numberWithInt: PhoneView_Settings]];
    
    //
    // Chat View
    //
    UICompositeViewDescription *chatDescription = [UICompositeViewDescription alloc];
    chatDescription->content = @"ChatViewController";
    chatDescription->tabBar = @"UIMainBar";
    chatDescription->tabBarEnabled = true;
    chatDescription->stateBar = nil;
    chatDescription->stateBarEnabled = false;
    chatDescription->fullscreen = false;
    [viewDescriptions setObject:chatDescription forKey:[NSNumber numberWithInt: PhoneView_Chat]];
    
    // Set observers
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(changeView:) 
                                                 name:@"LinphoneMainViewChange" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(registrationUpdate:) 
                                                 name:@"LinphoneRegistrationUpdate" 
                                               object:nil];

    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(batteryLevelChanged:) 
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
}
     
- (void)viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)registrationUpdate: (NSNotification*) notif { 
    LinphoneRegistrationState state = [[notif.userInfo objectForKey: @"state"] intValue];
    LinphoneProxyConfig *cfg = [[notif.userInfo objectForKey: @"cfg"] pointerValue];
    // Show error
    if (state == LinphoneRegistrationFailed) {
		NSString* lErrorMessage=nil;
		if (linphone_proxy_config_get_error(cfg) == LinphoneReasonBadCredentials) {
			lErrorMessage = NSLocalizedString(@"Bad credentials, check your account settings",nil);
		} else if (linphone_proxy_config_get_error(cfg) == LinphoneReasonNoResponse) {
			lErrorMessage = NSLocalizedString(@"SIP server unreachable",nil);
		} 
		
		if (lErrorMessage != nil && linphone_proxy_config_get_error(cfg) != LinphoneReasonNoResponse) { 
            //do not report network connection issue on registration
			//default behavior if no registration delegates
			UIApplicationState s = [UIApplication sharedApplication].applicationState;
            
            // do not stack error message when going to backgroud
            if (s != UIApplicationStateBackground) {
                UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Registration failure",nil)
                                                                message:lErrorMessage
                                                               delegate:nil 
                                                      cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
                                                      otherButtonTitles:nil ,nil];
                [error show];
                [error release];
            }
		}
		
	}
}

- (void)callUpdate:(NSNotification*) notif {  
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    NSString *message = [notif.userInfo objectForKey: @"message"];
    
    bool canHideInCallView = (linphone_core_get_calls([LinphoneManager getLc]) == NULL);
    
	switch (state) {					
		case LinphoneCallIncomingReceived: 
        {
			[self displayIncomingCall:call];
			break;
        }
		case LinphoneCallOutgoingInit: 
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
        case LinphoneCallUpdated:
        {
            if ([[LinphoneManager instance] currentView] != PhoneView_InCall) {
                [[LinphoneManager instance] changeView:PhoneView_InCall];
            }
            break;
        }
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);
            
            if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                if ([[LinphoneManager instance] currentView] != PhoneView_InCall) {
                    [[LinphoneManager instance] changeView:PhoneView_InCall];
                }
            }
            break;
        }
		case LinphoneCallError:
        {
            [self displayCallError:call message: message];
        }
		case LinphoneCallEnd: 
        {
            [self dismissIncomingCall];
            if (canHideInCallView) {
                if ([[LinphoneManager instance] currentView] != PhoneView_Dialer) {
                    // Go to dialer view
                    NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                                           [[[NSArray alloc] initWithObjects: @"", nil] autorelease]
                                          , @"setAddress:",
                                          nil] autorelease];
                    [[LinphoneManager instance] changeView:PhoneView_Dialer dict:dict];
                }
            } else {
                if ([[LinphoneManager instance] currentView] != PhoneView_InCall) {
                    [[LinphoneManager instance] changeView:PhoneView_InCall];
                }
			}
			break;
        }
		case LinphoneCallStreamsRunning:
        {
            if ([[LinphoneManager instance] currentView] != PhoneView_InCall) {
                [[LinphoneManager instance] changeView:PhoneView_InCall];
            }
			break;
        }
        default:
            break;
	}
    
}

- (void)displayCallError:(LinphoneCall*) call message:(NSString*) message {
    const char* lUserNameChars=linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    NSString* lMessage;
    NSString* lTitle;
    
    //get default proxy
    LinphoneProxyConfig* proxyCfg;	
    linphone_core_get_default_proxy([LinphoneManager getLc],&proxyCfg);
    if (proxyCfg == nil) {
        lMessage = NSLocalizedString(@"Please make sure your device is connected to the internet and double check your SIP account configuration in the settings.", nil);
    } else {
        lMessage = [NSString stringWithFormat : NSLocalizedString(@"Cannot call %@", nil), lUserName];
    }
    
    if (linphone_call_get_reason(call) == LinphoneReasonNotFound) {
        lMessage = [NSString stringWithFormat : NSLocalizedString(@"'%@' not registered to Service", nil), lUserName];
    } else {
        if (message != nil) {
            lMessage = [NSString stringWithFormat : NSLocalizedString(@"%@\nReason was: %@", nil), lMessage, message];
        }
    }
    lTitle = NSLocalizedString(@"Call failed",nil);
    UIAlertView* error = [[UIAlertView alloc] initWithTitle:lTitle
                                                    message:lMessage 
                                                   delegate:nil 
                                          cancelButtonTitle:NSLocalizedString(@"Dismiss",nil) 
                                          otherButtonTitles:nil];
    [error show];
    [error release];
}

- (void)dismissIncomingCall {
	//cancel local notification, just in case
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]  
		&& [UIApplication sharedApplication].applicationState ==  UIApplicationStateBackground ) {
		// cancel local notif if needed
		[[UIApplication sharedApplication] cancelAllLocalNotifications];
	} else {
		if (incomingCallActionSheet) {
			[incomingCallActionSheet dismissWithClickedButtonIndex:1 animated:true];
			incomingCallActionSheet = nil;
		}
	}
    
    //TODO
    /*
     if ([[NSUserDefaults standardUserDefaults] boolForKey:@"firstlogindone_preference" ] == true) {
     //first login case, dismmis first login view																		 
     [self dismissModalViewControllerAnimated:true];
     }; */
}


- (void)displayIncomingCall:(LinphoneCall*) call{
#if 0
    const char* userNameChars=linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* userName = userNameChars?[[[NSString alloc] initWithUTF8String:userNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    const char* displayNameChars =  linphone_address_get_display_name(linphone_call_get_remote_address(call));        
	NSString* displayName = [displayNameChars?[[NSString alloc] initWithUTF8String:displayNameChars]:@"" autorelease];
    
	//TODO
    //[mMainScreenWithVideoPreview showPreview:NO]; 
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif)
		{
			notif.repeatInterval = 0;
			notif.alertBody =[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),[displayName length]>0?displayName:userName];
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
        
		incomingCallActionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil),[displayName length]>0?displayName:userName]
															   delegate:cd 
													  cancelButtonTitle:nil 
												 destructiveButtonTitle:NSLocalizedString(@"Answer",nil) 
													  otherButtonTitles:NSLocalizedString(@"Decline",nil),nil];
        
		incomingCallActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
        //TODO
        /*if ([LinphoneManager runningOnIpad]) {
            if (self.modalViewController != nil)
                [incomingCallActionSheet showInView:[self.modalViewController view]];
            else
                [incomingCallActionSheet showInView:self.parentViewController.view];
        } else */{
            [incomingCallActionSheet showInView: self.view];
        }
		[incomingCallActionSheet release];
	}
#endif
    //UICompositeViewController *controller = [[UICompositeViewController alloc] initWithNibName:@"UICompositeViewController" bundle:[NSBundle mainBundle]];
    [modalViewController changeView:[viewDescriptions objectForKey:[NSNumber numberWithInt:PhoneView_IncomingCall]]];
    [self presentModalViewController:modalViewController animated:false];
}

- (void)batteryLevelChanged: (NSNotification*) notif {
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    if (!call || !linphone_call_params_video_enabled(linphone_call_get_current_params(call)))
        return;
    LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
    if ([UIDevice currentDevice].batteryState == UIDeviceBatteryStateUnplugged) {
        float level = [UIDevice currentDevice].batteryLevel;
        ms_message("Video call is running. Battery level: %.2f", level);
        if (level < 0.1 && !appData->batteryWarningShown) {
            // notify user
            CallDelegate* cd = [[CallDelegate alloc] init];
            cd.eventType = CD_STOP_VIDEO_ON_LOW_BATTERY;
            cd.delegate = self;
            cd.call = call;
            
            if (batteryActionSheet != nil) {
                [batteryActionSheet dismissWithClickedButtonIndex:batteryActionSheet.cancelButtonIndex animated:TRUE];
            }
            NSString* title = NSLocalizedString(@"Battery is running low. Stop video ?",nil);
            batteryActionSheet = [[UIActionSheet alloc] initWithTitle:title
                                                             delegate:cd 
                                                    cancelButtonTitle:NSLocalizedString(@"Continue video",nil) 
                                               destructiveButtonTitle:NSLocalizedString(@"Stop video",nil) 
                                                    otherButtonTitles:nil];
            
            batteryActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
            [batteryActionSheet showInView: self.view];
            [batteryActionSheet release];
            appData->batteryWarningShown = TRUE;
        }
    }
}

- (void)actionSheet:(UIActionSheet *)actionSheet ofType:(enum CallDelegateType)type 
                                   clickedButtonAtIndex:(NSInteger)buttonIndex 
                                          withUserDatas:(void *)datas {
    
    switch(type) {
        case CD_NEW_CALL: 
        {
            LinphoneCall* call = (LinphoneCall*)datas;
            if (buttonIndex == actionSheet.destructiveButtonIndex) {
                linphone_core_accept_call([LinphoneManager getLc], call);	
            } else {
                linphone_core_terminate_call([LinphoneManager getLc], call);
            }
            incomingCallActionSheet = nil;
            break;
        }
        case CD_STOP_VIDEO_ON_LOW_BATTERY: 
        {
            LinphoneCall* call = (LinphoneCall*)datas;
            LinphoneCallParams* paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
            if ([batteryActionSheet destructiveButtonIndex] == buttonIndex) {
                // stop video
                linphone_call_params_enable_video(paramsCopy, FALSE);
                linphone_core_update_call([LinphoneManager getLc], call, paramsCopy);
            }
            break;
        }
        default:
            break;
    }
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [viewDescriptions removeAllObjects];
    [viewDescriptions release];
    
    [super dealloc];
}

@end