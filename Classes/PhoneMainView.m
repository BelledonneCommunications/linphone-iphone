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

@implementation ViewsDescription

- (id)copy {
    ViewsDescription *copy = [ViewsDescription alloc];
    copy->content = self->content;
    copy->tabBar = self->tabBar;
    copy->tabBarEnabled = self->tabBarEnabled;
    copy->statusEnabled = self->statusEnabled;
    copy->fullscreen = self->fullscreen;
    copy->viewId = self->viewId;
    return copy;
}
@end

@implementation PhoneMainView

@synthesize stateBarView;
@synthesize contentView;
@synthesize tabBarView;

@synthesize stateBarController;

@synthesize callTabBarController;
@synthesize mainTabBarController;
@synthesize incomingCallTabBarController;

+ (void)addSubView:(UIViewController*)controller view:(UIView*)view {
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [controller viewWillAppear:NO];
    }
    [view addSubview: controller.view];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [controller viewDidAppear:NO];
    }
}

+ (void)removeSubView:(UIViewController*)controller {
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [controller viewWillDisappear:NO];
    }
    [controller.view removeFromSuperview];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [controller viewDidDisappear:NO];
    }
}

- (void)changeView: (NSNotification*) notif {   
    NSNumber *viewId = [notif.userInfo objectForKey: @"view"];
    NSNumber *tabBar = [notif.userInfo objectForKey: @"tabBar"];
    NSNumber *fullscreen = [notif.userInfo objectForKey: @"fullscreen"];
    
    // Copy view description
    ViewsDescription *oldViewDescription = (currentViewDescription != nil)? [currentViewDescription copy]: nil;
    
    // Check view change
    if(viewId != nil) {
        PhoneView view = [viewId intValue];
        ViewsDescription* description = [viewDescriptions objectForKey:[NSNumber numberWithInt: view]];
        if(description == nil)
            return;
        description->viewId = view; // Auto-set viewId
        if(currentViewDescription == nil || description->viewId != currentViewDescription->viewId) {
            if(currentViewDescription != nil)
                [currentViewDescription dealloc];
            currentViewDescription = [description copy];
        } else {
            viewId = nil;
        }
    }

    if(currentViewDescription == nil) {
        return;
    }
    
    if(tabBar != nil) {
        currentViewDescription->tabBarEnabled = [tabBar boolValue];
    }
    
    if(fullscreen != nil) {
        currentViewDescription->fullscreen = [fullscreen boolValue];
        [[UIApplication sharedApplication] setStatusBarHidden:currentViewDescription->fullscreen withAnimation:UIStatusBarAnimationSlide ];
    } else {
        [[UIApplication sharedApplication] setStatusBarHidden:currentViewDescription->fullscreen withAnimation:UIStatusBarAnimationNone];
    }
    
    // View Transitions
    if(viewId != nil) {
        if(oldViewDescription != nil) {
            CATransition* trans = [CATransition animation];
            [trans setType:kCATransitionPush];
            [trans setDuration:0.35];
            [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
            [trans setSubtype:kCATransitionFromRight];
            [contentView.layer addAnimation:trans forKey:@"Transition"];
            if((oldViewDescription->statusEnabled == true && currentViewDescription->statusEnabled == false) ||
               (oldViewDescription->statusEnabled == false && currentViewDescription->statusEnabled == true)) {
                [stateBarView.layer addAnimation:trans forKey:@"Transition"];
            }
            if(oldViewDescription->tabBar != currentViewDescription->tabBar) {
                [tabBarView.layer addAnimation:trans forKey:@"Transition"];
            }
            [PhoneMainView removeSubView: oldViewDescription->content];
            [PhoneMainView removeSubView: oldViewDescription->tabBar];
        }
    }
    
    // Start animation
    if(tabBar != nil || fullscreen != nil) {
        [UIView beginAnimations:@"resize" context:nil];
        [UIView setAnimationDuration:0.35];
        [UIView setAnimationBeginsFromCurrentState:TRUE];
    }
    
    UIView *innerView = currentViewDescription->content.view;
    
    CGRect contentFrame = contentView.frame;
    
    // Resize StateBar
    CGRect stateBarFrame = stateBarView.frame;
    if(currentViewDescription->fullscreen)
        stateBarFrame.origin.y = -20;
    else
        stateBarFrame.origin.y = 0;
    
    if(currentViewDescription->statusEnabled) {
        stateBarView.hidden = false;
        [stateBarView setFrame: stateBarFrame];
        contentFrame.origin.y = stateBarFrame.size.height + stateBarFrame.origin.y;
    } else {
        stateBarView.hidden = true;
        contentFrame.origin.y = stateBarFrame.origin.y;
    }
    
    // Resize TabBar
    CGRect tabFrame = tabBarView.frame;
    if(currentViewDescription->tabBar != nil && currentViewDescription->tabBarEnabled) {
        tabFrame.origin.y = [[UIScreen mainScreen] bounds].size.height - 20;
        tabFrame.origin.x = [[UIScreen mainScreen] bounds].size.width;
        tabFrame.size.height = currentViewDescription->tabBar.view.frame.size.height;
        tabFrame.size.width = currentViewDescription->tabBar.view.frame.size.width;
        tabFrame.origin.y -= tabFrame.size.height;
        tabFrame.origin.x -= tabFrame.size.width;
        contentFrame.size.height = tabFrame.origin.y - contentFrame.origin.y;
        for (UIView *view in currentViewDescription->tabBar.view.subviews) {
            if(view.tag == -1) {
                contentFrame.size.height += view.frame.origin.y;
                break;
            }
        }
    } else {
        contentFrame.size.height = tabFrame.origin.y + tabFrame.size.height;
        if(currentViewDescription->fullscreen)
            contentFrame.size.height += 20;
        tabFrame.origin.y = [[UIScreen mainScreen] bounds].size.height - 20;
    }
    
    // Resize innerView
    CGRect innerContentFrame = innerView.frame;
    innerContentFrame.size = contentFrame.size;
    
    
    // Set frames
    [contentView setFrame: contentFrame];
    [innerView setFrame: innerContentFrame];
    [tabBarView setFrame: tabFrame];
    
    // Commit animation
    if(tabBar != nil || fullscreen != nil) {
        [UIView commitAnimations];
    }
    
    // Change view
    if(viewId != nil) {
        [PhoneMainView addSubView: currentViewDescription->content view:contentView];
        [PhoneMainView addSubView: currentViewDescription->tabBar view:tabBarView];
    }
    
    // Call abstractCall
    NSDictionary *dict = [notif.userInfo objectForKey: @"args"];
    if(dict != nil)
        [AbstractCall call:currentViewDescription->content dict:dict];
    
    // Dealloc old view description
    if(oldViewDescription != nil) {
        [oldViewDescription dealloc];
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    UIView *dumb;
    
    // Init view descriptions
    viewDescriptions = [[NSMutableDictionary alloc] init];
    
    // Load Bars
    dumb = mainTabBarController.view;
    
    // Status Bar
    [stateBarView addSubview: stateBarController.view];
    
    //
    // Main View
    //
    DialerViewController* myDialerViewController = [[DialerViewController alloc]  
                                                  initWithNibName:@"DialerViewController" 
                                                  bundle:[NSBundle mainBundle]];
    //[myPhoneViewController loadView];
    ViewsDescription *dialerDescription = [ViewsDescription alloc];
    dialerDescription->content = myDialerViewController;
    dialerDescription->tabBar = mainTabBarController;
    dialerDescription->statusEnabled = true;
    dialerDescription->fullscreen = false;
    dialerDescription->tabBarEnabled = true;
    [viewDescriptions setObject:dialerDescription forKey:[NSNumber numberWithInt: PhoneView_Dialer]];
    
    
    //
    // Contacts View
    //
    ContactsViewController* myContactsController = [[ContactsViewController alloc]
                                                initWithNibName:@"ContactsViewController" 
                                                bundle:[NSBundle mainBundle]];
    //[myContactsController loadView];
    ViewsDescription *contactsDescription = [ViewsDescription alloc];
    contactsDescription->content = myContactsController;
    contactsDescription->tabBar = mainTabBarController;
    contactsDescription->statusEnabled = false;
    contactsDescription->fullscreen = false;
    contactsDescription->tabBarEnabled = true;
    [viewDescriptions setObject:contactsDescription forKey:[NSNumber numberWithInt: PhoneView_Contacts]];
    
    
    //
    // Call History View
    //
    HistoryViewController* myHistoryController = [[HistoryViewController alloc]
                                              initWithNibName:@"HistoryViewController" 
                                              bundle:[NSBundle mainBundle]];
    //[myHistoryController loadView];
    ViewsDescription *historyDescription = [ViewsDescription alloc];
    historyDescription->content = myHistoryController;
    historyDescription->tabBar = mainTabBarController;
    historyDescription->statusEnabled = false;
    historyDescription->fullscreen = false;
    historyDescription->tabBarEnabled = true;
    [viewDescriptions setObject:historyDescription forKey:[NSNumber numberWithInt: PhoneView_History]];
    
    //
    // IncomingCall View
    //
    IncomingCallViewController* myIncomingCallController = [[IncomingCallViewController alloc]
                                                initWithNibName:@"IncomingCallViewController" 
                                                bundle:[NSBundle mainBundle]];
    //[myChatViewController loadView];
    ViewsDescription *incomingCallDescription = [ViewsDescription alloc];
    incomingCallDescription->content = myIncomingCallController;
    incomingCallDescription->tabBar = mainTabBarController;
    incomingCallDescription->statusEnabled = true;
    incomingCallDescription->fullscreen = false;
    incomingCallDescription->tabBarEnabled = true;
    [viewDescriptions setObject:incomingCallDescription forKey:[NSNumber numberWithInt: PhoneView_Chat]];
    
    //
    // InCall View
    //
    InCallViewController* myInCallController = [[InCallViewController alloc]
                                                initWithNibName:@"InCallViewController" 
                                                bundle:[NSBundle mainBundle]];
    //[myInCallController loadView];
    ViewsDescription *inCallDescription = [ViewsDescription alloc];
    inCallDescription->content = myInCallController;
    inCallDescription->tabBar = nil;
    inCallDescription->statusEnabled = true;
    inCallDescription->fullscreen = false;
    inCallDescription->tabBarEnabled = false;
    [viewDescriptions setObject:inCallDescription forKey:[NSNumber numberWithInt: PhoneView_InCall]];
    
    
    //
    // Settings View
    //
    SettingsViewController* mySettingsViewController = [[SettingsViewController alloc]
                                                initWithNibName:@"SettingsViewController" 
                                                bundle:[NSBundle mainBundle]];
    //[mySettingsViewController loadView];
    ViewsDescription *settingsDescription = [ViewsDescription alloc];
    settingsDescription->content = mySettingsViewController;
    settingsDescription->tabBar = mainTabBarController;
    settingsDescription->statusEnabled = false;
    settingsDescription->fullscreen = false;
    settingsDescription->tabBarEnabled = true;
    [viewDescriptions setObject:settingsDescription forKey:[NSNumber numberWithInt: PhoneView_Settings]];
    
    //
    // Chat View
    //
    ChatViewController* myChatViewController = [[ChatViewController alloc]
                                                        initWithNibName:@"ChatViewController" 
                                                        bundle:[NSBundle mainBundle]];
    //[myChatViewController loadView];
    ViewsDescription *chatDescription = [ViewsDescription alloc];
    chatDescription->content = myChatViewController;
    chatDescription->tabBar = mainTabBarController;
    chatDescription->statusEnabled = false;
    chatDescription->fullscreen = false;
    chatDescription->tabBarEnabled = true;
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

- (void)callUpdate: (NSNotification*) notif {  
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
    
    [viewDescriptions release];
    [stateBarView release];
    [stateBarController release];
    [mainTabBarController release];
    
    [super dealloc];
}

@end