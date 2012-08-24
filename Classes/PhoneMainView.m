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

#import "LinphoneAppDelegate.h"
#import "PhoneMainView.h"
#import "Utils.h"

static PhoneMainView* phoneMainViewInstance=nil;

@implementation PhoneMainView

@synthesize mainViewController;
@synthesize currentView;

// TO READ
// If a Controller set wantFullScreenLayout then DON'T set the autoresize!
// So DON'T set autoresize for PhoneMainView

#pragma mark - Lifecycle Functions

- (void)initPhoneMainView {
    assert (!phoneMainViewInstance);
    phoneMainViewInstance = self;
    currentView = nil;
    viewStack = [[NSMutableArray alloc] init];
    loadCount = 0; // For avoiding IOS 4 bug
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
    
    [viewStack release];

    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    // Avoid IOS 4 bug
    if(loadCount++ > 0)
        return;
    
    [super viewDidLoad];

    [self.view addSubview: mainViewController.view];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [mainViewController viewWillAppear:animated];
    }   
    
    // Set observers
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(registrationUpdate:) 
                                                 name:kLinphoneRegistrationUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceived:) 
                                                 name:kLinphoneTextReceived
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(batteryLevelChanged:) 
                                                 name:UIDeviceBatteryLevelDidChangeNotification
                                               object:nil];
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:YES];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [mainViewController viewWillDisappear:animated];
    }
    
    // Remove observers
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kLinphoneRegistrationUpdate
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kLinphoneTextReceived
                                               object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:UIDeviceBatteryLevelDidChangeNotification 
                                               object:nil];
	[[UIDevice currentDevice] setBatteryMonitoringEnabled:NO];
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
    loadCount--;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if(interfaceOrientation == self.interfaceOrientation)
        return YES;
    return NO;
}

+ (UIView*)findFirstResponder:(UIView*)view {
    if (view.isFirstResponder) {
        return view;
    }
    for (UIView *subView in view.subviews) {
        UIView *ret = [PhoneMainView findFirstResponder:subView];
        if (ret != nil)
            return ret;
    }
    return nil;
}

/* 
    Will simulate a device rotation
 */
+ (void)setOrientation:(UIInterfaceOrientation)orientation animated:(BOOL)animated {
    UIView *firstResponder = nil;
    for(UIWindow *window in [[UIApplication sharedApplication] windows]) {
        if([NSStringFromClass(window.class) isEqualToString:@"UITextEffectsWindow"]) {
            continue;
        }
        UIView *view = window;
        UIViewController *controller = nil;
        CGRect frame = [view frame];
        if([window isKindOfClass:[UILinphoneWindow class]]) {
            controller = window.rootViewController;
            view = controller.view;
        }
        UIInterfaceOrientation oldOrientation = controller.interfaceOrientation;
        
        NSTimeInterval animationDuration = 0.0;
        if(animated) {
            animationDuration = 0.3f;
        }
        [controller willRotateToInterfaceOrientation:orientation duration:animationDuration];
        if(animated) {
            [UIView beginAnimations:nil context:nil];
            [UIView setAnimationDuration:animationDuration];
        }
        switch (orientation) {
            case UIInterfaceOrientationPortrait:
                [view setTransform: CGAffineTransformMakeRotation(0)];
                break;
            case UIInterfaceOrientationPortraitUpsideDown:
                [view setTransform: CGAffineTransformMakeRotation(M_PI)];
                break;
            case UIInterfaceOrientationLandscapeLeft:
                [view setTransform: CGAffineTransformMakeRotation(-M_PI / 2)];
                break;
            case UIInterfaceOrientationLandscapeRight:
                [view setTransform: CGAffineTransformMakeRotation(M_PI / 2)];
                break;
            default:
                break;
        }
        if([window isKindOfClass:[UILinphoneWindow class]]) {
            [view setFrame:frame];
        }
        [controller willAnimateRotationToInterfaceOrientation:orientation duration:animationDuration];
        if(animated) {
            [UIView commitAnimations];
        }
        [controller didRotateFromInterfaceOrientation:oldOrientation];
        if(firstResponder == nil) {
            firstResponder = [PhoneMainView findFirstResponder:view];
        }
    }
    [[UIApplication sharedApplication] setStatusBarOrientation:orientation animated:animated];
    if(firstResponder) {
        [firstResponder resignFirstResponder];
     [firstResponder becomeFirstResponder];
    }
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
    [mainViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
    [mainViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
    [mainViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (UIInterfaceOrientation)interfaceOrientation {
    return [mainViewController currentOrientation];
}


#pragma mark - Event Functions

- (void)textReceived:(NSNotification*)notif { 
    ChatModel *chat = [[notif userInfo] objectForKey:@"chat"];
    if(chat != nil) {
        [self displayMessage:chat];
    }
    [self updateApplicationBadgeNumber];
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
    
    // Don't handle call state during incoming call view
    if([[self currentView] equal:[IncomingCallViewController compositeViewDescription]] && state != LinphoneCallError && state != LinphoneCallEnd) {
        return;
    }
    
	switch (state) {					
		case LinphoneCallIncomingReceived: 
        {
			[self displayIncomingCall:call];
			break;
        }
		case LinphoneCallOutgoingInit: 
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
        case LinphoneCallStreamsRunning:
        case LinphoneCallUpdated:
        {
            [self changeCurrentView:[InCallViewController compositeViewDescription]];
            break;
        }
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);
            
            if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [self changeCurrentView:[InCallViewController compositeViewDescription]];
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
                DialerViewController *controller = DYNAMIC_CAST([self changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
                if(controller != nil) {
                    [controller setAddress:@""];
                    [controller setTransferMode:FALSE];
                }
            } else {
                [self changeCurrentView:[InCallViewController compositeViewDescription]];
			}
			break;
        }
        default:
            break;
	}
    [self updateApplicationBadgeNumber];
}


#pragma mark - 

- (void)startUp {   
    if ([[LinphoneManager instance].settingsStore boolForKey:@"enable_first_login_view_preference"] == true) {
        // Change to fist login view
        [self changeCurrentView: [FirstLoginViewController compositeViewDescription]];
    } else {
        // Change to default view
        const MSList *list = linphone_core_get_proxy_config_list([LinphoneManager getLc]);
        if(list != NULL) {
            [self changeCurrentView: [DialerViewController compositeViewDescription]];
        } else {
            [self changeCurrentView: [WizardViewController compositeViewDescription]];
        }
    }
    
    [self updateApplicationBadgeNumber]; // Update Badge at startup
}

- (void)updateApplicationBadgeNumber {
    int count = 0;
    count += linphone_core_get_missed_calls_count([LinphoneManager getLc]);
    count += [ChatModel unreadMessages];
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:count];
}

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

+ (CATransition*)getTransition:(UICompositeViewDescription *)old new:(UICompositeViewDescription *)new {
    bool left = false;
    
    if([old equal:[ChatViewController compositeViewDescription]]) {
        if([new equal:[ContactsViewController compositeViewDescription]] ||
           [new equal:[DialerViewController compositeViewDescription]] ||
           [new equal:[SettingsViewController compositeViewDescription]] ||
           [new equal:[HistoryViewController compositeViewDescription]]) {
            left = true;
        }
    } else if([old equal:[SettingsViewController compositeViewDescription]]) {
        if([new equal:[DialerViewController compositeViewDescription]] ||
           [new equal:[ContactsViewController compositeViewDescription]] ||
           [new equal:[HistoryViewController compositeViewDescription]]) {
            left = true;
        }
    } else if([old equal:[DialerViewController compositeViewDescription]]) {
        if([new equal:[ContactsViewController compositeViewDescription]] ||
           [new equal:[HistoryViewController compositeViewDescription]]) {
            left = true;
        }
    } else if([old equal:[ContactsViewController compositeViewDescription]]) {
        if([new equal:[HistoryViewController compositeViewDescription]]) {
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

- (void) showTabBar:(BOOL)show {
    [mainViewController setToolBarHidden:!show];
}

- (void) showStateBar:(BOOL)show {
    [mainViewController setStateBarHidden:!show];
}

- (void)fullScreen:(BOOL)enabled {
    [mainViewController setFullScreen:enabled];
}

- (UIViewController*)changeCurrentView:(UICompositeViewDescription *)view {
    return [self changeCurrentView:view push:FALSE];
}

- (UIViewController*)changeCurrentView:(UICompositeViewDescription*)view push:(BOOL)push {
    BOOL force = push;
    if(!push) {
        force = [viewStack count] > 1;
        [viewStack removeAllObjects];
    }
    [viewStack addObject:view];
    return [self _changeCurrentView:view transition:nil force:force];
}

- (UIViewController*)_changeCurrentView:(UICompositeViewDescription*)view transition:(CATransition*)transition force:(BOOL)force {
    [LinphoneLogger logc:LinphoneLoggerLog format:"PhoneMainView: Change current view to %@", [view name]];
    
    if(force || ![view equal: currentView]) {
        if(transition == nil)
            transition = [PhoneMainView getTransition:currentView new:view];
        if ([[LinphoneManager instance].settingsStore boolForKey:@"animations_preference"] == true) {
            [mainViewController setViewTransition:transition];
        } else {
            [mainViewController setViewTransition:nil];
        }
        [mainViewController changeView:view];
        currentView = view;
    } 
    
    NSDictionary* mdict = [NSMutableDictionary dictionaryWithObject:currentView forKey:@"view"];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneMainViewChange object:self userInfo:mdict];
    
    return [mainViewController getCurrentViewController];
}

- (void)popToView:(UICompositeViewDescription*)view {
    while([viewStack count] > 1 && ![[viewStack lastObject] equal:view]) {
        [viewStack removeLastObject];
    }
    [self _changeCurrentView:[viewStack lastObject] transition:[PhoneMainView getBackwardTransition] force:TRUE];
}

- (UICompositeViewDescription *)firstView {
    UICompositeViewDescription *view = nil;
    if([viewStack count]) {
        view = [viewStack objectAtIndex:0];
    }
    return view;
}
         
- (UIViewController*)popCurrentView {
    [LinphoneLogger logc:LinphoneLoggerLog format:"PhoneMainView: Pop view"];
    if([viewStack count] > 1) {
        [viewStack removeLastObject];
        [self _changeCurrentView:[viewStack lastObject] transition:[PhoneMainView getBackwardTransition] force:TRUE];
        return [mainViewController getCurrentViewController];
    } 
    return nil;
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
			notif.alertBody = [NSString  stringWithFormat:NSLocalizedString(@"IM_MSG",nil), address];
			notif.alertAction = NSLocalizedString(@"Show", nil);
			notif.soundName = @"msg.caf";
			notif.userInfo = [NSDictionary dictionaryWithObject:[chat remoteContact] forKey:@"chat"];
			
			[[UIApplication sharedApplication] presentLocalNotificationNow:notif];
		}
	} else {
        if(![[LinphoneManager instance] removeInhibitedEvent:kLinphoneTextReceivedSound]) {
            AudioServicesPlaySystemSound([LinphoneManager instance].sounds.message);
        }
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
            char* lAddress = linphone_address_as_string_uri_only(addr);
            if(lAddress) {
                NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:lAddress]];
                ABRecordRef contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
                if(contact) {
                    address = [FastAddressBook getContactDisplayName:contact];
                    useLinphoneAddress = false;
                }
                ms_free(lAddress);
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
			appData->notification.alertBody =[NSString  stringWithFormat:NSLocalizedString(@"IC_MSG",nil), address];
			appData->notification.alertAction = NSLocalizedString(@"Answer", nil);
			appData->notification.soundName = @"ring.caf";
			appData->notification.userInfo = [NSDictionary dictionaryWithObject:[NSData dataWithBytes:&call length:sizeof(call)] forKey:@"call"];
			
			[[UIApplication sharedApplication] presentLocalNotificationNow:appData->notification];
		}
	} else {
       IncomingCallViewController *controller = DYNAMIC_CAST([self changeCurrentView:[IncomingCallViewController compositeViewDescription] push:TRUE],IncomingCallViewController);
        if(controller != nil) {
            [controller setCall:call];
            [controller setDelegate:self];
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


#pragma mark - IncomingCallDelegate Functions

- (void)incomingCallAborted:(LinphoneCall*)call {
}

- (void)incomingCallAccepted:(LinphoneCall*)call {
    linphone_core_accept_call([LinphoneManager getLc], call);
}

- (void)incomingCallDeclined:(LinphoneCall*)call {
    linphone_core_terminate_call([LinphoneManager getLc], call);
}

@end