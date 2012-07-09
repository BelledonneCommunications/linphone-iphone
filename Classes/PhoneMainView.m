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

#import "FirstLoginViewController.h"
#import "IncomingCallViewController.h"

#import "ChatRoomViewController.h"
#import "ChatViewController.h"
#import "DialerViewController.h"
#import "ContactsViewController.h"
#import "HistoryViewController.h"
#import "InCallViewController.h"
#import "SettingsViewController.h"
#import "FirstLoginViewController.h"
#import "WizardViewController.h"
#import "ContactDetailsViewController.h"

#import "AbstractCall.h"

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
    modalControllers = [[NSMutableArray alloc] init];
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
    
    [modalControllers removeAllObjects];
    [modalControllers release];
    
    [viewStack release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    // Avoid IOS 4 bug
    if(self->loadCount++ > 0)
        return;
    
    [super viewDidLoad];
    [[self view] addSubview: mainViewController.view];
    
    // Init descriptions
    [viewDescriptions setObject:[ChatRoomViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_ChatRoom]];
    [viewDescriptions setObject:[ChatViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Chat]];
    [viewDescriptions setObject:[DialerViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Dialer]];
    [viewDescriptions setObject:[ContactsViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Contacts]];
    [viewDescriptions setObject:[HistoryViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_History]];
    [viewDescriptions setObject:[InCallViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_InCall]];
    [viewDescriptions setObject:[SettingsViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Settings]];
    [viewDescriptions setObject:[FirstLoginViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_FirstLogin]];
    [viewDescriptions setObject:[WizardViewController compositeViewDescription] forKey:[NSNumber numberWithInt: PhoneView_Wizard]];
    [viewDescriptions setObject:[ContactDetailsViewController compositeViewDescription] forKey:[NSNumber numberWithInt:PhoneView_ContactDetails]];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
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
                                             selector:@selector(batteryLevelChanged:) 
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    // Remove observers
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneRegistrationUpdate" 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
}

- (void)viewDidUnload {
    [super viewDidUnload];

    // Avoid IOS 4 bug
    self->loadCount--;
}


#pragma mark - Event Functions

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
                NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                                        [[[NSArray alloc] initWithObjects: @"", nil] autorelease]
                                        , @"setAddress:",
                                        [[[NSArray alloc] initWithObjects: [NSNumber numberWithInt: FALSE], nil] autorelease]
                                        , @"setTransferMode:",
                                        nil] autorelease];
                [self changeView:PhoneView_Dialer dict:dict];
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
    
    if(old == PhoneView_Chat) {
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
    } else if(old == PhoneView_Contacts) {
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

- (void)changeView:(PhoneView)view {
    [self changeView:view dict:nil push:FALSE];
}

- (void)changeView:(PhoneView)view dict:(NSDictionary *)dict {
    [self changeView:view dict:dict push:FALSE];
}

- (void)changeView:(PhoneView)view push:(BOOL)push {
    [self changeView:view dict:nil push:push];
}

- (void)changeView:(PhoneView)view dict:(NSDictionary *)dict push:(BOOL)push {
    if(push && currentView != -1) {
        [viewStack addObject:[NSNumber numberWithInt: currentView]];
    } else {
        [viewStack removeAllObjects];
    }
    [self _changeView:view dict:dict transition:nil];
}

- (void)_changeView:(PhoneView)view dict:(NSDictionary *)dict transition:(CATransition*)transition {
    UICompositeViewDescription* description = [viewDescriptions objectForKey:[NSNumber numberWithInt: view]];
    if(description == nil)
        return;
    
    if(view != currentView) {
        if(transition == nil)
            transition = [PhoneMainView getTransition:currentView new:view];
        [mainViewController setViewTransition:transition];
        [mainViewController changeView:description];
        currentView = view;
    } 
    
    // Call abstractCall
    if(dict != nil)
        [AbstractCall call:[mainViewController getCurrentViewController] dict:dict];
    
    NSDictionary* mdict = [NSMutableDictionary dictionaryWithObject: [NSNumber numberWithInt:currentView] forKey:@"view"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneMainViewChange" object:self userInfo:mdict];
}

- (void)popView {
    [self popView:nil];
}

- (void)popView:(NSDictionary *)dict {
    if([viewStack count] > 0) {
        PhoneView view = [[viewStack lastObject] intValue];
        [viewStack removeLastObject];
        [self _changeView:view dict:dict transition:[PhoneMainView getBackwardTransition]];
    } 
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
	//cancel local notification, just in case
	if ([[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]  
		&& [UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
		// cancel local notif if needed
		[[UIApplication sharedApplication] cancelAllLocalNotifications];
	}
}


#pragma mark - ActionSheet Functions

- (void)displayIncomingCall:(LinphoneCall*) call{

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
	} else {     
        IncomingCallViewController *controller = [[IncomingCallViewController alloc] init];
        [controller setCall:call];
        [self addModalViewController:controller];
	}
}

- (void)batteryLevelChanged:(NSNotification*)notif {
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

- (void)modalViewDismiss:(UIModalViewController*)controller value:(int)value {
    [self removeModalViewController:controller];
}

- (void)addModalViewController:(UIModalViewController*)controller {
    [controller setModalDelegate:self];
    [modalControllers insertObject:controller atIndex:0];
    
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionFade];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:kCATransitionFromRight];
    [[self view].layer addAnimation:trans forKey:@"Appear"];
    
    [[self view] addSubview:[controller view]];
    [[self view] bringSubviewToFront:[controller view]];
}

- (void)removeModalViewController:(UIModalViewController*)controller {
    [controller setModalDelegate:nil];
    [modalControllers removeObject:controller];
    
    CATransition* trans = [CATransition animation];
    [trans setType:kCATransitionFade];
    [trans setDuration:0.35];
    [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
    [trans setSubtype:kCATransitionFromRight];
    [[self view].layer addAnimation:trans forKey:@"Disappear"];
    
    [[controller view] removeFromSuperview];
}

@end