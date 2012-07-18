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
#import <AudioToolbox/AudioServices.h>

#import "PhoneMainView.h"
#import "Utils.h"
#import "UIView+ModalStack.h"

static PhoneMainView* phoneMainViewInstance=nil;

@implementation PhoneMainView

@synthesize mainViewController;


#pragma mark - Lifecycle Functions

- (void)initPhoneMainView {
    assert (!phoneMainViewInstance);
    phoneMainViewInstance = self;
    currentView = -1;
    viewStack = [[NSMutableArray alloc] init];
    loadCount = 0; // For avoiding IOS 4 bug
    
    // Init view descriptions
    viewDescriptions = [[NSMutableDictionary alloc] init];
}

- (id)init {
    self = [super init];
    if (self) {
		[self initPhoneMainView];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self initPhoneMainView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initPhoneMainView];
	}
    return self;
}	

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [mainViewController release];
    
    [viewDescriptions removeAllObjects];
    [viewDescriptions release];
    
    [viewStack release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    // Avoid IOS 4 bug
    if(self->loadCount++ > 0)
        return;
    
    [super viewDidLoad];

    [self.view addSubview: mainViewController.view];
    [mainViewController.view setFrame:[self.view frame]];
    
    // Init descriptions
    [viewDescriptions setObject:[ChatRoomViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_ChatRoom]];
    [viewDescriptions setObject:[ChatViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Chat]];
    [viewDescriptions setObject:[DialerViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Dialer]];
    [viewDescriptions setObject:[ContactsViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Contacts]];
    [viewDescriptions setObject:[ContactDetailsViewController compositeViewDescription] forKey:[NSNumber numberWithInt:PhoneView_ContactDetails]];
    [viewDescriptions setObject:[HistoryViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_History]];
    [viewDescriptions setObject:[HistoryDetailsViewController compositeViewDescription] forKey:[NSNumber numberWithInt:PhoneView_HistoryDetails]];
    [viewDescriptions setObject:[InCallViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_InCall]];
    [viewDescriptions setObject:[SettingsViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Settings]];
    [viewDescriptions setObject:[FirstLoginViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_FirstLogin]];
    [viewDescriptions setObject:[WizardViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Wizard]];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [mainViewController viewWillAppear:animated];
    }   
    
    // Set observers
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(registrationUpdate:) 
                                                 name:@"LinphoneRegistrationUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceived:) 
                                                 name:@"LinphoneTextReceived" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(batteryLevelChanged:) 
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [mainViewController viewWillDisappear:animated];
    }
    
    // Remove observers
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneRegistrationUpdate" 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneTextReceived" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [mainViewController viewDidAppear:animated];
    }   
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [mainViewController viewDidDisappear:animated];
    }  
}

- (void)viewDidUnload {
    [super viewDidUnload];

    // Avoid IOS 4 bug
    self->loadCount--;
}


#pragma mark - Event Functions

- (void)textReceived:(NSNotification*)notif { 
    ChatModel *chat = [[notif userInfo] objectForKey:@"chat"];
    if(chat != nil) {
        [self displayMessage:chat];
    }
}

- (void)registrationUpdate:(NSNotification*)notif { 
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
                                                      otherButtonTitles:nil,nil];
                [error show];
                [error release];
            }
		}
		
	}
}

- (void)callUpdate:(NSNotification*)notif {  
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
            [self changeView:PhoneView_InCall];
            break;
        }
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);
            
            if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [self changeView:PhoneView_InCall];
            }
            break;
        }
		case LinphoneCallError:
        {
            [self displayCallError:call message: message];
        }
		case LinphoneCallEnd: 
        {
            [self dismissIncomingCall:call];
            if (canHideInCallView) {
                // Go to dialer view
                DialerViewController *controller = DYNAMIC_CAST([self changeView:PhoneView_Dialer], DialerViewController);
                if(controller != nil) {
                    [controller setAddress:@""];
                    [controller setTransferMode:FALSE];
                }
            } else {
                [self changeView:PhoneView_InCall];
			}
			break;
        }
		case LinphoneCallStreamsRunning:
        {
            [self changeView:PhoneView_InCall];
			break;
        }
        default:
            break;
	}
}


#pragma mark - 

+ (CATransition*)getBackwardTransition {
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionPush];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:kCATransitionFromLeft];
    
    return trans;
}

+ (CATransition*)getForwardTransition {
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionPush];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:kCATransitionFromRight];
    
    return trans;
}

+ (CATransition*)getTransition:(PhoneView)old new:(PhoneView)new {
    bool left = false;
    
    if(old == PhoneView_Chat || old == PhoneView_ChatRoom) {
        if(new == PhoneView_Contacts ||
           new == PhoneView_Dialer ||
           new == PhoneView_Settings ||
           new == PhoneView_History) {
            left = true;
        }
    } else if(old == PhoneView_Settings) {
        if(new == PhoneView_Dialer ||
           new == PhoneView_Contacts ||
           new == PhoneView_History) {
            left = true;
        }
    } else if(old == PhoneView_Dialer) {
        if(new == PhoneView_Contacts ||
           new == PhoneView_History) {
            left = true;
        }
    } else if(old == PhoneView_Contacts || old == PhoneView_ContactDetails) {
        if(new == PhoneView_History) {
            left = true;
        }
    } 
    
    if(left) {
        return [PhoneMainView getBackwardTransition];
    } else {
        return [PhoneMainView getForwardTransition];
    }
}

+ (PhoneMainView *) instance {
    return phoneMainViewInstance;
}

- (void) showTabBar:(BOOL) show {
    [mainViewController setToolBarHidden:!show];
}

- (void)fullScreen:(BOOL) enabled {
    [mainViewController setFullScreen:enabled];
}

- (UIViewController*)changeView:(PhoneView)view {
    return [self changeView:view push:FALSE];
}

- (UIViewController*)changeView:(PhoneView)view push:(BOOL)push {
    if(push && currentView != -1) {
        [viewStack addObject:[NSNumber numberWithInt: currentView]];
    } else {
        [viewStack removeAllObjects];
    }
    return [self _changeView:view transition:nil];
}

- (UIViewController*)_changeView:(PhoneView)view transition:(CATransition*)transition {
    [LinphoneLogger logc:LinphoneLoggerLog format:"PhoneMainView: change view %d", view];
    UICompositeViewDescription* description = [viewDescriptions objectForKey:[NSNumber numberWithInt: view]];
    if(description == nil)
        return nil;
    
    if(view != currentView) {
        if(transition == nil)
            transition = [PhoneMainView getTransition:currentView new:view];
        [mainViewController setViewTransition:transition];
        [mainViewController changeView:description];
        currentView = view;
    } 
    
    NSDictionary* mdict = [NSMutableDictionary dictionaryWithObject: [NSNumber numberWithInt:currentView] forKey:@"view"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneMainViewChange" object:self userInfo:mdict];
    
    return [mainViewController getCurrentViewController];
}

- (UIViewController*)popView {
    return [self popView:nil];
}

- (UIViewController*)popView:(NSArray *)calls {
    [LinphoneLogger logc:LinphoneLoggerLog format:"PhoneMainView: Pop view"];
    if([viewStack count] > 0) {
        PhoneView view = [[viewStack lastObject] intValue];
        [viewStack removeLastObject];
        [self _changeView:view transition:[PhoneMainView getBackwardTransition]];
        
        return [mainViewController getCurrentViewController];
    } 
    return nil;
}

- (PhoneView)currentView {
    return currentView;
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

- (void)dismissIncomingCall:(LinphoneCall*)call {
    LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);

    if(appData != nil && appData->notification != nil) {
        // cancel local notif if needed
        [[UIApplication sharedApplication] cancelLocalNotification:appData->notification];
        [appData->notification release];
    }
}

#pragma mark - ActionSheet Functions

- (void)displayMessage:(ChatModel*)chat {
    if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
        
        NSString* address = [chat remoteContact];
        NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:address];
        ABRecordRef contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
        if(contact) {
            address = [FastAddressBook getContactDisplayName:contact];
        }
        if(address == nil) {
            address = @"Unknown";
        }
        
		// Create a new notification
		UILocalNotification* notif = [[[UILocalNotification alloc] init] autorelease];
		if (notif) {
			notif.repeatInterval = 0;
			notif.alertBody = [NSString  stringWithFormat:NSLocalizedString(@"%@ sent you a message",nil), address];
			notif.alertAction = NSLocalizedString(@"Show", nil);
			notif.soundName = UILocalNotificationDefaultSoundName;
			notif.userInfo = [NSDictionary dictionaryWithObject:[chat remoteContact] forKey:@"chat"];
			
			[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
		}
	} else {
        AudioServicesPlaySystemSound(kSystemSoundID_Vibrate);
    }
}

- (void)displayIncomingCall:(LinphoneCall*) call{
    LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)] 
		&& [UIApplication sharedApplication].applicationState !=  UIApplicationStateActive) {
        
        const LinphoneAddress *addr = linphone_call_get_remote_address(call);
        NSString* address = nil;
        if(addr != NULL) {
            BOOL useLinphoneAddress = true;
            // contact name 
            const char* lAddress = linphone_address_as_string_uri_only(addr);
            if(lAddress) {
                NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:lAddress]];
                ABRecordRef contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
                if(contact) {
                    address = [FastAddressBook getContactDisplayName:contact];
                    useLinphoneAddress = false;
                }
            }
            if(useLinphoneAddress) {
                const char* lDisplayName = linphone_address_get_display_name(addr);
                const char* lUserName = linphone_address_get_username(addr);
                if (lDisplayName) 
                    address = [NSString stringWithUTF8String:lDisplayName];
                else if(lUserName) 
                    address = [NSString stringWithUTF8String:lUserName];
            }
        }
        if(address == nil) {
            address = @"Unknown";
        }
        
		// Create a new notification
		appData->notification = [[UILocalNotification alloc] init];
		if (appData->notification) {
			appData->notification.repeatInterval = 0;
			appData->notification.alertBody =[NSString  stringWithFormat:NSLocalizedString(@" %@ is calling you",nil), address];
			appData->notification.alertAction = NSLocalizedString(@"Answer", nil);
			appData->notification.soundName = @"oldphone-mono-30s.caf";
			appData->notification.userInfo = [NSDictionary dictionaryWithObject:[NSData dataWithBytes:&call length:sizeof(call)] forKey:@"call"];
			
			[[UIApplication sharedApplication]  presentLocalNotificationNow:appData->notification];
		}
	} else {     
        IncomingCallViewController *controller = [[IncomingCallViewController alloc] init];
        [controller setWantsFullScreenLayout:TRUE];
        [controller setCall:call];
        [controller setModalDelegate:self];
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [controller viewWillAppear:NO];
        }   
        [[self view] addModalView:[controller view]];
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [controller viewDidAppear:NO];
        }  
	}
}

- (void)batteryLevelChanged:(NSNotification*)notif {
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    if (!call || !linphone_call_params_video_enabled(linphone_call_get_current_params(call)))
        return;
    LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
    if ([UIDevice currentDevice].batteryState == UIDeviceBatteryStateUnplugged) {
        float level = [UIDevice currentDevice].batteryLevel;
        [LinphoneLogger logc:LinphoneLoggerLog format:"Video call is running. Battery level: %.2f", level];
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
        case CD_STOP_VIDEO_ON_LOW_BATTERY: 
        {
            LinphoneCall* call = (LinphoneCall*)datas;
            LinphoneCallParams* paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
            if (buttonIndex == [batteryActionSheet destructiveButtonIndex]) {
                // stop video
                linphone_call_params_enable_video(paramsCopy, FALSE);
                linphone_core_update_call([LinphoneManager getLc], call, paramsCopy);
            }
            batteryActionSheet = nil;
            break;
        }
        default:
            break;
    }
}


#pragma mark - Modal Functions

- (void)modalViewDismiss:(UIModalViewController*)controller value:(id)value {
    [controller setModalDelegate:nil];
    [[self view] removeModalView:[controller view]];
}

@end