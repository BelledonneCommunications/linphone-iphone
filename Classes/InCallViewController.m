/* InCallViewController.h
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

#import <AudioToolbox/AudioToolbox.h>
#import <AddressBook/AddressBook.h>
#import <QuartzCore/CAAnimation.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

#import "IncallViewController.h"
#import "UICallCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "UILinphone.h"

#include "linphonecore.h"
#include "private.h"

const NSInteger SECURE_BUTTON_TAG=5;


@implementation InCallViewController

@synthesize callTableController;
@synthesize callTableView;

@synthesize videoGroup;
@synthesize videoView;
@synthesize videoPreview;
@synthesize videoCameraSwitch;
@synthesize videoWaitingForFirstImage;
#ifdef TEST_VIDEO_VIEW_CHANGE
@synthesize testVideoView;
#endif


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"InCallViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [callTableController release];
    [callTableView release];
    
    [videoGroup release];
    [videoView release];
    [videoPreview release];
#ifdef TEST_VIDEO_VIEW_CHANGE
    [testVideoView release];
#endif
    [videoCameraSwitch release];
    
    [videoWaitingForFirstImage release];
    
    [videoZoomHandler release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"InCall" 
                                                                content:@"InCallViewController" 
                                                               stateBar:@"UIStateBar" 
                                                        stateBarEnabled:true 
                                                                 tabBar:@"UICallBar" 
                                                          tabBarEnabled:true 
                                                             fullscreen:false
                                                          landscapeMode:true
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if (visibleActionSheet != nil) {
        [visibleActionSheet dismissWithClickedButtonIndex:visibleActionSheet.cancelButtonIndex animated:NO];
    }
    if (hideControlsTimer != nil) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [callTableController viewWillDisappear:animated];
    }
    
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:UIDeviceOrientationDidChangeNotification 
                                                  object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [callTableController viewWillAppear:animated];
    }   
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdateEvent:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(orientationChanged:) 
                                                 name:UIDeviceOrientationDidChangeNotification 
                                               object:nil];
    
    // Update on show
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    LinphoneCallState state = (call != NULL)?linphone_call_get_state(call): 0;
    [self callUpdate:call state:state animated:FALSE];
    [self orientationUpdate];
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [callTableController viewDidAppear:animated];
    }  
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    
	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
	UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = NO;
    
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [callTableController viewDidDisappear:animated];
    }  
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set windows (warn memory leaks)
    linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)videoView);	
    linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)videoPreview);
    
    UITapGestureRecognizer* singleFingerTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(showControls:)];
    [singleFingerTap setNumberOfTapsRequired:1];
    [singleFingerTap setCancelsTouchesInView: FALSE];
    [[[UIApplication sharedApplication].delegate window] addGestureRecognizer:singleFingerTap];
    [singleFingerTap release];
    
    videoZoomHandler = [[VideoZoomHandler alloc] init];
    [videoZoomHandler setup:videoGroup];
    videoGroup.alpha = 0;
    
    [videoCameraSwitch setPreview:videoPreview];
    
    removeTableBackground([callTableController view]);
}


#pragma mark -

- (void)orientationUpdate {
    int oldLinphoneOrientation = linphone_core_get_device_rotation([LinphoneManager getLc]);
    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    int newRotation = 0;
    switch (orientation) {
        case UIInterfaceOrientationLandscapeRight:
            newRotation = 270;
            break;
        case UIInterfaceOrientationLandscapeLeft:
            newRotation = 90;
            break;
        default:
            newRotation = 0;
    }
    if (oldLinphoneOrientation != newRotation) {
        linphone_core_set_device_rotation([LinphoneManager getLc], newRotation);
        LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
        if (call && linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
            //Orientation has changed, must call update call
            linphone_core_update_call([LinphoneManager getLc], call, NULL);
        }
    }
}

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
    // Update table
    [callTableView reloadData];  
    
    // Fake call update
    if(call == NULL) {
        return;
    }
    
    // Handle data associated with the call
    if(state == LinphoneCallReleased) {
        [callTableController removeCallData: call];
    } else {
        [callTableController addCallData: call];
    }
    
	switch (state) {					
		case LinphoneCallIncomingReceived: 
		case LinphoneCallOutgoingInit: 
        {
            if(linphone_core_get_calls_nb([LinphoneManager getLc]) > 1) {
                [callTableController minimizeAll];
            }
        }
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
        case LinphoneCallUpdated:
        {
			//check video
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				[self displayVideoCall:animated];
			} else {
                [self displayTableCall:animated];
            }
			break;
        }
        case LinphoneCallUpdatedByRemote:
        {
            const LinphoneCallParams* current = linphone_call_get_current_params(call);
            const LinphoneCallParams* remote = linphone_call_get_remote_params(call);
            
            /* remote wants to add video */
            if (!linphone_call_params_video_enabled(current) && 
                linphone_call_params_video_enabled(remote) && 
                !linphone_core_get_video_policy([LinphoneManager getLc])->automatically_accept) {
                linphone_core_defer_call_update([LinphoneManager getLc], call);
                [self displayAskToEnableVideoCall:call];
            } else if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [self displayTableCall:animated];
            }
            break;
        }
        case LinphoneCallPausing:
        case LinphoneCallPaused:
        case LinphoneCallPausedByRemote:
        {
            [self displayTableCall:animated];
            break;
        }
        case LinphoneCallEnd:
        case LinphoneCallError:
        {
            if(linphone_core_get_calls_nb([LinphoneManager getLc]) <= 2) {
                [callTableController maximizeAll];
            }
            break;
        }
        default:
            break;
	}
    
}

- (void)showControls:(id)sender {
    if (hideControlsTimer) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }
    
    if([[[PhoneMainView instance] currentView] equal:[InCallViewController compositeViewDescription]] && videoShown) {
        // show controls
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:0.3];
        [[PhoneMainView instance] showTabBar: true];
        [videoCameraSwitch setAlpha:1.0];
        [UIView commitAnimations];
        
        // hide controls in 5 sec
        hideControlsTimer = [NSTimer scheduledTimerWithTimeInterval:5.0
                                                             target:self
                                                           selector:@selector(hideControls:)
                                                           userInfo:nil
                                                            repeats:NO];
    }
}

- (void)hideControls:(id)sender {
    if (hideControlsTimer) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }
    
    if([[[PhoneMainView instance] currentView] equal:[InCallViewController compositeViewDescription]] && videoShown) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:0.3];
        [videoCameraSwitch setAlpha:0.0];
        [UIView commitAnimations];
        
        
        [[PhoneMainView instance] showTabBar: false];
    }
}

#ifdef TEST_VIDEO_VIEW_CHANGE
// Define TEST_VIDEO_VIEW_CHANGE in IncallViewController.h to enable video view switching testing
- (void)_debugChangeVideoView {
    static bool normalView = false;
    if (normalView) {
        linphone_core_set_native_video_window_id([LinphoneManager getLc], (unsigned long)videoView);
    } else {
        linphone_core_set_native_video_window_id([LinphoneManager getLc], (unsigned long)testVideoView);
    }
    normalView = !normalView;
}
#endif

- (void)enableVideoDisplay:(BOOL)animation {
    if(videoShown && animation)
        return;
    
    videoShown = true;
    
    [videoZoomHandler resetZoom];
    
    if(animation) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:1.0];
    }
    
    [videoGroup setAlpha:1.0];
    [callTableView setAlpha:0.0];
    
    if(animation) {
        [UIView commitAnimations];
    }
    
    if([[[LinphoneManager instance] settingsStore] boolForKey:@"self_video_preference"]) {
        [videoPreview setHidden:FALSE];
    } else {
        [videoPreview setHidden:TRUE];
    }
    
    if ([LinphoneManager instance].frontCamId !=nil) {
        // only show camera switch button if we have more than 1 camera
        [videoCameraSwitch setHidden:FALSE];
    }
    [videoCameraSwitch setAlpha:0.0];
    
    [[PhoneMainView instance] fullScreen: true];
    [[PhoneMainView instance] showTabBar: false];
    
#ifdef TEST_VIDEO_VIEW_CHANGE
    [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(_debugChangeVideoView) userInfo:nil repeats:YES];
#endif
    // [self batteryLevelChanged:nil];
    
    [videoWaitingForFirstImage setHidden: NO];
    [videoWaitingForFirstImage startAnimating];
    
    LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
    if (call != NULL && call->videostream) {
        linphone_call_set_next_video_frame_decoded_callback(call, hideSpinner, self);
    }
}

- (void)disableVideoDisplay:(BOOL)animation {
    if(!videoShown && animation)
        return;
    
    videoShown = false;
    if(animation) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:1.0];
    }
    
    [videoGroup setAlpha:0.0];
    [[PhoneMainView instance] showTabBar: true];
    [callTableView setAlpha:1.0];
    [videoCameraSwitch setHidden:TRUE];
    
    if(animation) {
        [UIView commitAnimations];
    }
    
    if (hideControlsTimer != nil) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }
    
    [[PhoneMainView instance] fullScreen:false];
}

- (void)displayVideoCall:(BOOL)animated { 
    [self enableVideoDisplay:animated];
}

- (void)displayTableCall:(BOOL)animated {
    [self disableVideoDisplay:animated];
}


#pragma mark - Spinner Functions

- (void)hideSpinnerIndicator: (LinphoneCall*)call {
    videoWaitingForFirstImage.hidden = TRUE;
}

static void hideSpinner(LinphoneCall* call, void* user_data) {
    InCallViewController* thiz = (InCallViewController*) user_data;
    [thiz hideSpinnerIndicator:call];
}


#pragma mark - Event Functions

- (void)orientationChanged:(NSNotification*) notif {   
    [self orientationUpdate];
}

- (void)callUpdateEvent: (NSNotification*) notif {
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:call state:state animated:TRUE];
}


#pragma mark - ActionSheet Functions

- (void)dismissActionSheet: (id)o {
    if (visibleActionSheet != nil) {
        [visibleActionSheet dismissWithClickedButtonIndex:visibleActionSheet.cancelButtonIndex animated:TRUE];
        visibleActionSheet = nil;
    }
}

- (void)displayAskToEnableVideoCall:(LinphoneCall*) call {
    if (linphone_core_get_video_policy([LinphoneManager getLc])->automatically_accept)
        return;
    
    const char* lUserNameChars = linphone_address_get_username(linphone_call_get_remote_address(call));
    NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
    const char* lDisplayNameChars =  linphone_address_get_display_name(linphone_call_get_remote_address(call));        
	NSString* lDisplayName = [lDisplayNameChars?[[NSString alloc] initWithUTF8String:lDisplayNameChars]:@"" autorelease];
    
    // ask the user if he agrees
    CallDelegate* cd = [[CallDelegate alloc] init];
    cd.eventType = CD_VIDEO_UPDATE;
    cd.delegate = self;
    cd.call = call;
    
    if (visibleActionSheet != nil) {
        [visibleActionSheet dismissWithClickedButtonIndex:visibleActionSheet.cancelButtonIndex animated:TRUE];
    }
    NSString* title = [NSString stringWithFormat : NSLocalizedString(@"'%@' would like to enable video",nil), ([lDisplayName length] > 0)?lDisplayName:lUserName];
    visibleActionSheet = [[UIActionSheet alloc] initWithTitle:title
                                                    delegate:cd 
                                           cancelButtonTitle:NSLocalizedString(@"Decline",nil) 
                                      destructiveButtonTitle:NSLocalizedString(@"Accept",nil) 
                                           otherButtonTitles:nil];
    
    visibleActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
    [visibleActionSheet showInView:[[UIApplication sharedApplication].delegate window]];
    
    /* start cancel timer */
    cd.timeout = [NSTimer scheduledTimerWithTimeInterval:30 target:self selector:@selector(dismissActionSheet:) userInfo:nil repeats:NO];
    [visibleActionSheet release];
}

- (void)actionSheet:(UIActionSheet *)actionSheet ofType:(enum CallDelegateType)type clickedButtonAtIndex:(NSInteger)buttonIndex withUserDatas:(void *)datas {
    LinphoneCall* call = (LinphoneCall*)datas;
    // maybe we could verify call validity

    switch (type) {
        case CD_ZRTP: {
            if (buttonIndex == 0)
                linphone_call_set_authentication_token_verified(call, YES);
            else if (buttonIndex == 1)
                linphone_call_set_authentication_token_verified(call, NO);
            visibleActionSheet = nil;
            break;
        }
        case CD_VIDEO_UPDATE: {
            LinphoneCall* call = (LinphoneCall*)datas;
            LinphoneCallParams* paramsCopy = linphone_call_params_copy(linphone_call_get_current_params(call));
            if ([visibleActionSheet destructiveButtonIndex] == buttonIndex) {
                // accept video
                linphone_call_params_enable_video(paramsCopy, TRUE);
                linphone_core_accept_call_update([LinphoneManager getLc], call, paramsCopy);
            } else {
                // decline video
                [LinphoneLogger logc:LinphoneLoggerLog format:"User declined video proposal"];
                linphone_core_accept_call_update([LinphoneManager getLc], call, NULL);
            }
            linphone_call_params_destroy(paramsCopy);
            visibleActionSheet = nil;
            break;
        }
        default:
            [LinphoneLogger logc:LinphoneLoggerError format:"Unhandled CallDelegate event of type: %d received - ignoring", type];
    }
}

@end
