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

- (id)init {
    return [super initWithNibName:@"InCallViewController" bundle:[NSBundle mainBundle]];
}

- (void)orientationChanged: (NSNotification*) notif {   
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
        linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)videoView);
        
        LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
        if (call && linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
            //Orientation has changed, must call update call
            linphone_core_update_call([LinphoneManager getLc], call, NULL);
        
        
            /* animate button images rotation */
#define degreesToRadians(x) (M_PI * x / 180.0)
            CGAffineTransform transform = CGAffineTransformIdentity;
            switch (orientation) {
                case UIInterfaceOrientationLandscapeRight:
                    transform = CGAffineTransformMakeRotation(degreesToRadians(90));
                    break;
                case UIInterfaceOrientationLandscapeLeft:
                    transform = CGAffineTransformMakeRotation(degreesToRadians(-90));
                    break;
                default:
                    transform = CGAffineTransformIdentity;
                    break;
            }
        
            [UIView beginAnimations:nil context:NULL];
            [UIView setAnimationDuration:0.2f];
                        //TODO
            //endCtrl.imageView.transform = transform;
            //mute.imageView.transform = transform;
            //speaker.imageView.transform = transform;
            //pause.imageView.transform = transform;
            //contacts.imageView.transform = transform;
            //addCall.imageView.transform = transform;
            //addVideo.imageView.transform = transform;
            //dialer.imageView.transform = transform;
            [UIView commitAnimations];
        }
    }    
}

- (void)showControls:(id)sender {
    if (hideControlsTimer) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }
    
    // show controls    
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:0.3];
    [[LinphoneManager instance] showTabBar: true];
    if ([LinphoneManager instance].frontCamId !=nil ) {
        // only show camera switch button if we have more than 1 camera
        [videoCameraSwitch setAlpha:1.0];
    }
    [UIView commitAnimations];
    
    // hide controls in 5 sec
    hideControlsTimer = [NSTimer scheduledTimerWithTimeInterval:5.0 
                                                         target:self 
                                                       selector:@selector(hideControls:) 
                                                       userInfo:nil 
                                                        repeats:NO];
}

- (void)hideControls:(id)sender {
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:0.3];
    [videoCameraSwitch setAlpha:0.0];
    [UIView commitAnimations];
    
    if([[LinphoneManager instance] currentView] == PhoneView_InCall && videoShown)
        [[LinphoneManager instance] showTabBar: false];

    if (hideControlsTimer) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
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

- (void)enableVideoDisplay:(BOOL)animation  {
    videoShown = true;
    [self orientationChanged:nil];
    
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
    
    videoView.alpha = 1.0;
    videoView.hidden = FALSE;
    
    linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)videoView);	
    linphone_core_set_native_preview_window_id([LinphoneManager getLc],(unsigned long)videoPreview);
    
    [[LinphoneManager instance] fullScreen: true];
    [[LinphoneManager instance] showTabBar: false];
    
#ifdef TEST_VIDEO_VIEW_CHANGE
    [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(_debugChangeVideoView) userInfo:nil repeats:YES];
#endif
   // [self batteryLevelChanged:nil];
    
    [self updateUIFromLinphoneState: YES];
    videoWaitingForFirstImage.hidden = NO;
    [videoWaitingForFirstImage startAnimating];
    
    // TODO
    LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
    if (call != NULL && call->videostream) {
        linphone_call_set_next_video_frame_decoded_callback(call, hideSpinner, self);
    }
}

- (void)disableVideoDisplay:(BOOL)animation {
    videoShown = false;
    if(animation) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:1.0];
    }

    [videoGroup setAlpha:0.0];
    [[LinphoneManager instance] showTabBar: true];
    [callTableView setAlpha:1.0];
    [videoCameraSwitch setAlpha:0.0];

    if(animation) {
        [UIView commitAnimations];
    }
    
    if (hideControlsTimer != nil) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }

    /* restore buttons orientation */
    //endCtrl.imageView.transform = CGAffineTransformIdentity;
    //TODO
    //mute.imageView.transform = CGAffineTransformIdentity;
    //speaker.imageView.transform = CGAffineTransformIdentity;
    //pause.imageView.transform = CGAffineTransformIdentity;
    //contacts.imageView.transform = CGAffineTransformIdentity;
    //addCall.imageView.transform = CGAffineTransformIdentity;
    //dialer.imageView.transform = CGAffineTransformIdentity;
    //videoCallQuality.transform = CGAffineTransformIdentity;
    
    [[LinphoneManager instance] fullScreen:false];
}

/* Update in call view buttons (visibility, state, ...) and call duration text.
 This is called periodically. The fullUpdate boolean is set when called after an event (call state change for instance) */
- (void)updateUIFromLinphoneState:(BOOL) fullUpdate {
    // check LinphoneCore is initialized
    LinphoneCore* lc = nil;
    if([LinphoneManager isLcReady])
        lc = [LinphoneManager getLc];
    
    // 1 call: show pause button, otherwise show merge btn
    // [LinphoneManager set:mergeCalls hidden:!pause.hidden withName:"MERGE button" andReason:"call count"];
    // reload table (glow update + call duration)
    [callTableView reloadData];       

    // update conference details view if displayed
    //TODO
    /*if (self.presentedViewController == conferenceDetail) {
        if (!linphone_core_is_in_conference(lc))
            [self dismissModalViewControllerAnimated:YES];
        else
            [conferenceDetail.table reloadData];
    }*/
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(callUpdate:) name:@"LinphoneCallUpdate" object:nil];
    
	//Controls
	/*[mute initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] debugName:"MUTE button"];
    [speaker initWithOnImage:[UIImage imageNamed:@"HP_inverse.png"]  offImage:[UIImage imageNamed:@"HP.png"] debugName:"SPEAKER button"];
    */

	//Dialer init
	/*[zero initWithNumber:'0'];
	[one initWithNumber:'1'];
	[two initWithNumber:'2'];
	[three initWithNumber:'3'];
	[four initWithNumber:'4'];
	[five initWithNumber:'5'];
	[six initWithNumber:'6'];
	[seven initWithNumber:'7'];
	[eight initWithNumber:'8'];
	[nine initWithNumber:'9'];
	[star initWithNumber:'*'];
	[hash initWithNumber:'#'];
    
    [addCall addTarget:self action:@selector(addCallPressed) forControlEvents:UIControlEventTouchUpInside];
    [mergeCalls addTarget:self action:@selector(mergeCallsPressed) forControlEvents:UIControlEventTouchUpInside];
    [LinphoneManager set:mergeCalls hidden:YES withName:"MERGE button" andReason:"initialisation"];*/
    
    //TODO
    /*
    if ([LinphoneManager runningOnIpad]) {
        ms_message("Running on iPad");
        conferenceDetail = [[ConferenceCallDetailView alloc]  initWithNibName:@"ConferenceCallDetailView-ipad" 
                                                                       bundle:[NSBundle mainBundle]];
    } else {
        conferenceDetail = [[ConferenceCallDetailView alloc]  initWithNibName:@"ConferenceCallDetailView" 
                                                                       bundle:[NSBundle mainBundle]];
    }*/
    
    UITapGestureRecognizer* singleFingerTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(showControls:)];
    [singleFingerTap setNumberOfTapsRequired:1];
    [singleFingerTap setCancelsTouchesInView: FALSE];
    [[[UIApplication sharedApplication].delegate window] addGestureRecognizer:singleFingerTap];
    [singleFingerTap release];
    
    videoZoomHandler = [[VideoZoomHandler alloc] init];
    [videoZoomHandler setup:videoGroup];
    videoGroup.alpha = 0;

    //selectedCall = nil;
    
    //callTableView.rowHeight = 80;
    
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(orientationChanged:) name:UIDeviceOrientationDidChangeNotification object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(batteryLevelChanged:) name:UIDeviceBatteryLevelDidChangeNotification object:nil];
    
    
    [videoCameraSwitch setPreview:videoPreview];
    //addVideo.videoUpdateIndicator = videoUpdateIndicator;
    
    //[transfer addTarget:self action:@selector(transferPressed) forControlEvents:UIControlEventTouchUpInside];
    
    // prevent buttons resizing
    /*
    endCtrl.imageView.contentMode = UIViewContentModeCenter;
    mute.imageView.contentMode = UIViewContentModeCenter;
    speaker.imageView.contentMode = UIViewContentModeCenter;
    pause.imageView.contentMode = UIViewContentModeCenter;
    contacts.imageView.contentMode = UIViewContentModeCenter;
    addCall.imageView.contentMode = UIViewContentModeCenter;
    dialer.imageView.contentMode = UIViewContentModeCenter;*/
}

- (void)transferPressed {
    /* allow only if call is active */
    if (!linphone_core_get_current_call([LinphoneManager getLc]))
        return;
    
    /* build UIActionSheet */
    if (visibleActionSheet != nil) {
        [visibleActionSheet dismissWithClickedButtonIndex:visibleActionSheet.cancelButtonIndex animated:TRUE];
    }
    
    CallDelegate* cd = [[CallDelegate alloc] init];
    cd.eventType = CD_TRANSFER_CALL;
    cd.delegate = self;
    cd.call = linphone_core_get_current_call([LinphoneManager getLc]);
    NSString* title = NSLocalizedString(@"Transfer to ...",nil);
    visibleActionSheet = [[UIActionSheet alloc] initWithTitle:title
                                                     delegate:cd 
                                            cancelButtonTitle:nil  
                                       destructiveButtonTitle:nil // NSLocalizedString(@"Other...",nil)
                                            otherButtonTitles:nil];
    
    // add button for each trasnfer-to valid call
    const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);
    while (calls) {
        LinphoneCall* call = (LinphoneCall*) calls->data;
        LinphoneCallAppData* data = ((LinphoneCallAppData*)linphone_call_get_user_pointer(call));
        if (call != cd.call && !linphone_call_get_current_params(call)->in_conference) {
            const LinphoneAddress* addr = linphone_call_get_remote_address(call);
            NSString* btnTitle = [NSString stringWithFormat : NSLocalizedString(@"%s",nil), (linphone_address_get_display_name(addr) ?linphone_address_get_display_name(addr):linphone_address_get_username(addr))];
            data->transferButtonIndex = [visibleActionSheet addButtonWithTitle:btnTitle];
        } else {
            data->transferButtonIndex = -1;
        }
        calls = calls->next;
    }
    
    if (visibleActionSheet.numberOfButtons == 0) {
        [visibleActionSheet release];
        visibleActionSheet = nil;
        
        //TODO
        /*[UICallButton enableTransforMode:YES];*/
        [[LinphoneManager instance] changeView:PhoneView_Dialer];
    } else {
        // add 'Other' option
        [visibleActionSheet addButtonWithTitle:NSLocalizedString(@"Other...",nil)];
        
        // add cancel button on iphone
        if (![LinphoneManager runningOnIpad]) {
            [visibleActionSheet addButtonWithTitle:NSLocalizedString(@"Cancel",nil)];
        }

        visibleActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
        if ([LinphoneManager runningOnIpad]) {
            //[visibleActionSheet showFromRect:transfer.bounds inView:transfer animated:NO];
        } else
            [visibleActionSheet showInView:[[UIApplication sharedApplication].delegate window]];
    }
}

- (void)viewDidAppear:(BOOL)animated {
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    [super viewDidAppear:animated]; 
}

- (void)viewWillDisappear:(BOOL)animated {
    if (visibleActionSheet != nil) {
        [visibleActionSheet dismissWithClickedButtonIndex:visibleActionSheet.cancelButtonIndex animated:NO];
    }
    if (hideControlsTimer != nil) {
        [hideControlsTimer invalidate];
        hideControlsTimer = nil;
    }
}

- (void)viewDidDisappear:(BOOL)animated {
	if (!videoShown) [[UIApplication sharedApplication] setIdleTimerDisabled:false];
}

- (void)viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)displayPad:(bool) enable {
    /*if (videoView.hidden)
        [LinphoneManager set:callTableView hidden:enable withName:"CALL_TABLE view" andReason:AT];*/
    /*[LinphoneManager set:hangUpView hidden:enable withName:"HANG_UP view" andReason:AT];
    [LinphoneManager set:controlSubView hidden:enable withName:"CONTROL view" andReason:AT];
    [LinphoneManager set:padSubView hidden:!enable withName:"PAD view" andReason:AT];*/
}

- (void)displayVideoCall:(LinphoneCall*) call {
	UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
	if (call !=nil  && linphone_call_get_dir(call)==LinphoneCallIncoming) {
		//if ([speaker isOn]) [speaker toggle];
	}
    [self updateUIFromLinphoneState: YES];
    
    [self enableVideoDisplay: TRUE];
}

- (void)displayInCall:(LinphoneCall*) call {
	UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
	if (call !=nil  && linphone_call_get_dir(call)==LinphoneCallIncoming) {
		//if ([speaker isOn]) [speaker toggle];
	}
    [self updateUIFromLinphoneState: YES];
    
    [self disableVideoDisplay: TRUE];
}

static void hideSpinner(LinphoneCall* lc, void* user_data);

- (void)hideSpinnerIndicator: (LinphoneCall*)call {
    if (!videoWaitingForFirstImage.hidden) {
        videoWaitingForFirstImage.hidden = TRUE;
    } else {
        linphone_call_set_next_video_frame_decoded_callback(call, hideSpinner, self);
    }
}

- (void)callUpdate: (NSNotification*) notif {  
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    
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
        case LinphoneCallPausedByRemote:
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
        case LinphoneCallUpdated:
			//check video
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				[self displayVideoCall:call];
			} else {
                [self displayInCall:call];
            }
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
                [self displayAskToEnableVideoCall:call];
            } else if (linphone_call_params_video_enabled(current) && !linphone_call_params_video_enabled(remote)) {
                [self displayInCall:call];
            }
            break;
        }
        case LinphoneCallPausing:
        case LinphoneCallPaused:
        {
            [self disableVideoDisplay: TRUE];
            break;
        }
        case LinphoneCallEnd:
        case LinphoneCallError:
        {
            if(linphone_core_get_calls_nb([LinphoneManager getLc]) <= 1) {
                [callTableController maximizeAll];
            }
            //[self updateUIFromLinphoneState: YES];
            break;
        }
        default:
            break;
	}
    
}

static void hideSpinner(LinphoneCall* call, void* user_data) {
    InCallViewController* thiz = (InCallViewController*) user_data;
    [thiz hideSpinnerIndicator:call];
}

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

- (void)firstVideoFrameDecoded: (LinphoneCall*) call {
    // hide video in progress view indicator
    videoWaitingForFirstImage.hidden = TRUE;
}

- (void)dealloc {
    [videoGroup release];
    [callTableView release];
    [videoView release];
    [videoPreview release];
#ifdef TEST_VIDEO_VIEW_CHANGE
    [testVideoView release];
#endif
    [videoCameraSwitch release];
    [videoWaitingForFirstImage release];
     
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc]; 
}

//TODO
/*
+ (void)updateCellImageView:(UIImageView*)imageView Label:(UILabel*)label DetailLabel:(UILabel*)detailLabel AndAccessoryView:(UIView*)accessoryView withCall:(LinphoneCall*) call {
    if (call == NULL) {
        ms_warning("UpdateCell called with null call");
        [label setText:@""];
        return;
    }
    const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    
    label.adjustsFontSizeToFitWidth = YES;
    
    if (addr) {
		const char* lUserNameChars=linphone_address_get_username(addr);
		NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
        NSMutableString* mss = [[NSMutableString alloc] init];
        // contact name 
        const char* n = linphone_address_get_display_name(addr);
        if (n) 
            [mss appendFormat:@"%s", n, nil];
        else
            [mss appendFormat:@"%@",lUserName , nil];
        
        if ([mss compare:label.text] != 0 || imageView.image == nil) {
            [label setText:mss];
        
            imageView.image = [[LinphoneManager instance] getImageFromAddressBook:lUserName];
        }
		[mss release];
    } else {
        [label setText:@"plop"];
        imageView.image = nil;
    }
    
    if (detailLabel != nil) {
        NSMutableString* ms = [[NSMutableString alloc] init ];
        if (linphone_call_get_state(call) == LinphoneCallStreamsRunning) {
            int duration = linphone_call_get_duration(call);
            if (duration >= 60)
                [ms appendFormat:@"%02i:%02i", (duration/60), duration - 60*(duration/60), nil];
            else
                [ms appendFormat:@"%02i sec", duration, nil];
        } else {
            switch (linphone_call_get_state(call)) {
                case LinphoneCallPaused:
                    if(!linphone_core_sound_resources_locked(linphone_call_get_core(call))) {
                        [ms appendFormat:@"%@", NSLocalizedString(@"Paused (tap to resume)", nil), nil];
                    } else {
                        [ms appendFormat:@"%@", NSLocalizedString(@"Paused", nil), nil];
                    }
                    break;
                case LinphoneCallOutgoingInit:
                case LinphoneCallOutgoingProgress:
                    [ms appendFormat:@"%@...", NSLocalizedString(@"In progress", nil), nil];
                    break;
                case LinphoneCallOutgoingRinging:
                    [ms appendFormat:@"%@...", NSLocalizedString(@"Ringing...", nil), nil];
                    break;
                case LinphoneCallPausedByRemote:
                {
                    switch (linphone_call_get_transfer_state(call)) {
                        case LinphoneCallOutgoingInit:
                        case LinphoneCallOutgoingProgress:
                            [ms appendFormat:@"%@...", NSLocalizedString(@"Transfer in progress", nil), nil];
                            break;
                        case LinphoneCallConnected:
                            [ms appendFormat:@"%@", NSLocalizedString(@"Transfer successful", nil), nil];
                            break;
                        case LinphoneCallError:
                            [ms appendFormat:@"%@", NSLocalizedString(@"Transfer failed", nil), nil];
                            break;
                        case LinphoneCallIdle:
                        default:
                            [ms appendFormat:@"%@...", NSLocalizedString(@"Paused by remote", nil), nil];
                            break;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        [detailLabel setText:ms];
		[ms release];
    }
}*/
/*
- (void)updateConferenceCell:(UITableViewCell*) cell at:(NSIndexPath*)indexPath {
    LinphoneCore* lc = [LinphoneManager getLc];
    
    NSString* t= [NSString stringWithFormat:
                  NSLocalizedString(@"Conference", nil), 
                  linphone_core_get_conference_size(lc) - linphone_core_is_in_conference(lc)];
    [cell.textLabel setText:t];
    
    [self updateActive:NO cell:cell];
    cell.selected = NO;
    
    [callTableView deselectRowAtIndexPath:indexPath animated:NO];
    
    if (!linphone_core_is_in_conference(lc)) {
        [cell.detailTextLabel setText:NSLocalizedString(@"(tap to enter conference)", nil)];
    } else {
        [cell.detailTextLabel setText:
         [NSString stringWithFormat:NSLocalizedString(@"(me + %d participants)", nil), linphone_core_get_conference_size(lc) - linphone_core_is_in_conference(lc)]];
    }	
    cell.imageView.image = nil;
}

- (void)secureIconPressed:(UIControl*) button withEvent: (UIEvent*) evt {
    NSSet* touches = [evt allTouches];
    UITouch* touch = [touches anyObject];
    CGPoint currentTouchPos = [touch locationInView:self.callTableView];
    NSIndexPath *path = [self.callTableView indexPathForRowAtPoint:currentTouchPos];
    if (path) {
        LinphoneCall* call = [InCallViewController retrieveCallAtIndex:path.row inConference:NO];
        // start action sheet to validate/unvalidate zrtp code
        CallDelegate* cd = [[CallDelegate alloc] init];
        cd.eventType = CD_ZRTP;
        cd.delegate = self;
        cd.call = call;
        UIView* container=(UIView*)[callTableView cellForRowAtIndexPath:path].accessoryView;
        UIButton *button=(UIButton*)[container viewWithTag:SECURE_BUTTON_TAG];
        [button setImage:nil forState:UIControlStateNormal];
            
        if (visibleActionSheet != nil) {
            [visibleActionSheet dismissWithClickedButtonIndex:visibleActionSheet.cancelButtonIndex animated:TRUE];
        }
		visibleActionSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@" Mark auth token '%s' as:",nil),linphone_call_get_authentication_token(call)]
                                                    delegate:cd 
                                                    cancelButtonTitle:NSLocalizedString(@"Unverified",nil) 
                                                    destructiveButtonTitle:NSLocalizedString(@"Verified",nil) 
                                                    otherButtonTitles:nil];
        
		visibleActionSheet.actionSheetStyle = UIActionSheetStyleDefault;
		[visibleActionSheet showInView:[[UIApplication sharedApplication].delegate window]];
		[visibleActionSheet release];
    }
}
*/
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
                ms_message("User declined video proposal");
                linphone_core_accept_call_update([LinphoneManager getLc], call, NULL);
            }
            linphone_call_params_destroy(paramsCopy);
            visibleActionSheet = nil;
            break;
        }
        case CD_TRANSFER_CALL: {
            LinphoneCall* call = (LinphoneCall*)datas;
            // browse existing call and trasnfer to the one matching the btn id
            const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);
            while (calls) {
                LinphoneCall* call2 = (LinphoneCall*) calls->data;
                LinphoneCallAppData* data = ((LinphoneCallAppData*)linphone_call_get_user_pointer(call2));
                if (data->transferButtonIndex == buttonIndex) {
                    linphone_core_transfer_call_to_another([LinphoneManager getLc], call, call2);
                    return;
                }
                data->transferButtonIndex = -1;
                calls = calls->next;
            }
            if (![LinphoneManager runningOnIpad] && buttonIndex == (actionSheet.numberOfButtons - 1)) {
                // cancel button
                return;
            }
            // user must jhave pressed 'other...' button as we did not find a call
            // with the correct indice
            //TODO
            //[UICallButton enableTransforMode:YES];
            [[LinphoneManager instance] changeView:PhoneView_Dialer];
            break;
        }
        default:
            ms_error("Unhandled CallDelegate event of type: %d received - ignoring", type);
    }
}
@end
