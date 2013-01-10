/* BuschJaegerCallView.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import "BuschJaegerCallView.h"
#import "BuschJaegerMainView.h"
#import "BuschJaegerUtils.h"
#include "linphonecore.h"
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>

@implementation BuschJaegerCallView

@synthesize incomingView;
@synthesize contactLabel;
@synthesize videoView;
@synthesize takeCallButton;
@synthesize declineButton;
@synthesize endOrRejectCallButton;
@synthesize microButton;
@synthesize lightsButton;
@synthesize openDoorButton;
@synthesize snapshotButton;
@synthesize cameraLeftSwipeGestureRecognizer;
@synthesize cameraRightSwipeGestureRecognizer;


#pragma mark - View lifecycle

- (void)initBuschJaegerCallView {
    currentCall = NULL;
    chatRoom = NULL;
}

- (id)init {
    self = [super init];
    if(self != NULL) {
        [self initBuschJaegerCallView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if(self != NULL) {
        [self initBuschJaegerCallView];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if(self != NULL) {
        [self initBuschJaegerCallView];
    }
    return self;
}

- (void)dealloc {
    [videoView release];
    [takeCallButton release];
    [declineButton release];
    [endOrRejectCallButton release];
    [microButton release];
    [lightsButton release];
    [openDoorButton release];
    [snapshotButton release];
    [cameraLeftSwipeGestureRecognizer release];
    [cameraRightSwipeGestureRecognizer release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [openDoorButton setPlay:FALSE];
    [openDoorButton setDigit:'1'];
    
    [lightsButton setPlay:FALSE];
    [lightsButton setDigit:'2'];

    [microButton setImage:[UIImage imageNamed:@"bj_mute_off.png"] forState:UIControlStateHighlighted | UIControlStateSelected];
    
    /* init gradients */
    {
        UIColor* col1 = BUSCHJAEGER_NORMAL_COLOR;
        UIColor* col2 = BUSCHJAEGER_NORMAL_COLOR2;
        [BuschJaegerUtils createGradientForButton:microButton withTopColor:col1 bottomColor:col2];
    }
    {
        UIColor* col1 = BUSCHJAEGER_RED_COLOR;
        UIColor* col2 = BUSCHJAEGER_RED_COLOR2;
        
        [BuschJaegerUtils createGradientForButton:endOrRejectCallButton withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForButton:declineButton withTopColor:col1 bottomColor:col2];
    }
    {
        UIColor* col1 = BUSCHJAEGER_GREEN_COLOR;
        UIColor* col2 = BUSCHJAEGER_GREEN_COLOR;
        
        [BuschJaegerUtils createGradientForButton:takeCallButton withTopColor:col1 bottomColor:col2];
    }
    
    linphone_core_set_native_video_window_id([LinphoneManager getLc], (unsigned long)videoView);
    linphone_core_set_native_preview_window_id([LinphoneManager getLc], 0);
    
    videoZoomHandler = [[VideoZoomHandler alloc] init];
    [videoZoomHandler setup:videoView];
    
    cameraRightSwipeGestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(doCameraSwipe:)];
    [cameraRightSwipeGestureRecognizer setDirection:UISwipeGestureRecognizerDirectionLeft];
    [cameraRightSwipeGestureRecognizer setDelegate:self];
    [videoView addGestureRecognizer:cameraRightSwipeGestureRecognizer];
    
    cameraLeftSwipeGestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(doCameraSwipe:)];
    [cameraLeftSwipeGestureRecognizer setDirection:UISwipeGestureRecognizerDirectionRight];
    [cameraLeftSwipeGestureRecognizer setDelegate:self];
    [videoView addGestureRecognizer:cameraLeftSwipeGestureRecognizer];
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(callUpdateEvent:)
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    
    [incomingView setHidden:YES];
    [takeCallButton setHidden:YES];
    [microButton setHidden:NO];
    [declineButton setHidden:YES];
    [endOrRejectCallButton setHidden:YES];
    [videoView setHidden:YES];
    
    User *usr = [[[LinphoneManager instance] configuration] getCurrentUser];
    /* init gradients for openDoorButton*/
    {
        bool enabled = (usr != nil && usr.opendoor && ![[NSUserDefaults standardUserDefaults] boolForKey:@"lockdoors_preference"]);
        UIColor* col1 = (enabled)?BUSCHJAEGER_NORMAL_COLOR:BUSCHJAEGER_GRAY_COLOR;
        UIColor* col2 = (enabled)?BUSCHJAEGER_NORMAL_COLOR2:BUSCHJAEGER_GRAY_COLOR2;
        
        [self.openDoorButton setEnabled:enabled];
        [BuschJaegerUtils createGradientForButton:openDoorButton withTopColor:col1 bottomColor:col2];
    }
    
    /* init gradients for lightsButton */
    {
        bool enabled = (usr != nil && usr.switchlight && ![[NSUserDefaults standardUserDefaults] boolForKey:@"switchlight_preference"]);
        UIColor* col1 = (enabled)?BUSCHJAEGER_NORMAL_COLOR:BUSCHJAEGER_GRAY_COLOR;
        UIColor* col2 = (enabled)?BUSCHJAEGER_NORMAL_COLOR2:BUSCHJAEGER_GRAY_COLOR2;
        
        [BuschJaegerUtils createGradientForButton:lightsButton withTopColor:col1 bottomColor:col2];
        [self.lightsButton setEnabled:enabled];
    }
    
    // Update on show
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    LinphoneCallState state = (call != NULL)?linphone_call_get_state(call): 0;
    [self callUpdate:call state:state animated:FALSE];
    
    [[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationFade];
    [[BuschJaegerMainView instance].navigationController setNavigationBarHidden:FALSE];
    [[BuschJaegerMainView instance].navigationController setNavigationBarHidden:TRUE];
    if (![LinphoneManager runningOnIpad]) {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(orientationDidChange:)
                                                     name:UIDeviceOrientationDidChangeNotification
                                                   object:nil];
        [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
        [self orientationUpdate:[[UIDevice currentDevice] orientation]];
    }
    
    UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
}

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneCallUpdate
                                                  object:nil];
    
    [[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationFade];
    if (![LinphoneManager runningOnIpad]) {
        [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
        
        [[NSNotificationCenter defaultCenter] removeObserver:self
                                                        name:UIDeviceOrientationDidChangeNotification
                                                      object:nil];
    }
    
    UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = NO;
}


#pragma mark - Event Functions

- (void)callUpdateEvent: (NSNotification*) notif {
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:call state:state animated:TRUE];
}

- (void)orientationDidChange:(NSNotification*)notif {
    [self orientationUpdate:[[UIDevice currentDevice] orientation]];
}


#pragma mark -

- (void)orientationUpdate:(UIInterfaceOrientation)orientation {
    [UIView beginAnimations:nil context:nil];
    [UIView setAnimationDuration:0.3f];
    CGRect frame = [self.view frame];
    switch (orientation) {
        case UIInterfaceOrientationPortrait:
            [self.view setTransform: CGAffineTransformMakeRotation(0)];
            break;
        case UIInterfaceOrientationPortraitUpsideDown:
            [self.view setTransform: CGAffineTransformMakeRotation(M_PI)];
            break;
        case UIInterfaceOrientationLandscapeLeft:
            [self.view setTransform: CGAffineTransformMakeRotation(-M_PI / 2)];
            break;
        case UIInterfaceOrientationLandscapeRight:
            [self.view setTransform: CGAffineTransformMakeRotation(M_PI / 2)];
            break;
        default:
            break;
    }
    [self.view setFrame:frame];
    [UIView commitAnimations];
}

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
    // Fake call update
    if(call == NULL) {
        return;
    }
    
    // Update chatroom
    if(currentCall != linphone_core_get_current_call([LinphoneManager getLc])) {
        if(currentCall != NULL) {
            linphone_chat_room_destroy(chatRoom);
            chatRoom = NULL;
        }
        currentCall = linphone_core_get_current_call([LinphoneManager getLc]);
        if(currentCall != NULL) {
            char *address = linphone_call_get_remote_address_as_string(currentCall);
            if(address != NULL) {
                chatRoom = linphone_core_create_chat_room([LinphoneManager getLc], address);
                ms_free(address);
            }
        }
        [openDoorButton setChatRoom:chatRoom];
        [lightsButton setChatRoom:chatRoom];
    }
    
    [microButton update];
    
	switch (state) {
        case LinphoneCallIncomingEarlyMedia:
		case LinphoneCallIncomingReceived:
        {
            [self displayIncomingCall:call];
            break;
        }
		case LinphoneCallOutgoingInit:
        case LinphoneCallOutgoingProgress:
        case LinphoneCallOutgoingRinging:
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
        case LinphoneCallUpdating:
        {
            [[LinphoneManager instance] setSpeakerEnabled:TRUE];
			//check video
			if (linphone_call_params_video_enabled(linphone_call_get_current_params(call))) {
				[self displayVideoCall];
			} else {
                [self displayInCall];
            }
			break;
        }
        default:
            break;
	}
    
}

- (void)displayIncomingCall:(LinphoneCall *)call {
    [incomingView setHidden:NO];
    [takeCallButton setHidden:NO];
    [microButton setHidden:YES];
    [declineButton setHidden:NO];
    [endOrRejectCallButton setHidden:YES];
    [videoView setHidden:NO];
    [snapshotButton setHidden:YES];

    NSString *contactName = NSLocalizedString(@"Unknown", nil);
    
    // Extract caller address
    const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    if(addr) {
        char *address = linphone_address_as_string_uri_only(addr);
        if(address != NULL) {
            contactName = [FastAddressBook normalizeSipURI:[NSString stringWithUTF8String:address]];
            ms_free(address);
        }
    }
    
    // Find caller in outdoor stations
    NSSet *outstations = [[LinphoneManager instance] configuration].outdoorStations;
    for(OutdoorStation *os in outstations) {
        if([[FastAddressBook normalizeSipURI:os.address] isEqualToString:contactName]) {
            contactName = os.name;
            break;
        }
    }
    [contactLabel setText:contactName];
}

- (void)displayInCall {
    [incomingView setHidden:YES];
    [takeCallButton setHidden:YES];
    [microButton setHidden:NO];
    [declineButton setHidden:YES];
    [endOrRejectCallButton setHidden:NO];
    [videoView setHidden:NO];
    [snapshotButton setHidden:YES];
}

- (void)displayVideoCall {
    [incomingView setHidden:YES];
    [takeCallButton setHidden:YES];
    [microButton setHidden:NO];
    [declineButton setHidden:YES];
    [endOrRejectCallButton setHidden:NO];
    [videoView setHidden:NO];
    [snapshotButton setHidden:NO];
}

- (void)saveImage:(NSString*)imagePath {
    UIImage *image = [UIImage imageWithContentsOfFile:imagePath];
    if(image != nil) {
        UIImageWriteToSavedPhotosAlbum(image, nil, nil, nil);
    }
}


#pragma mark - Actions Functions

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

- (IBAction)doCameraSwipe:(UISwipeGestureRecognizer *)sender {
    if(videoZoomHandler.zoomLevel == 1) {
        char digit = 0;
        if (sender.direction == UISwipeGestureRecognizerDirectionLeft) {
            digit = '7';
        } else if (sender.direction == UISwipeGestureRecognizerDirectionRight) {
            digit = '8';
        }
        if(digit != 0) {
            if(chatRoom == NULL) {
                linphone_core_send_dtmf([LinphoneManager getLc], digit);
            } else {
                linphone_chat_room_send_message(chatRoom, [[NSString stringWithFormat:@"%c", digit] UTF8String]);
            }
        }
    }
}

- (IBAction)takeCall:(id)sender {
    const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);	
    
    while(calls) {
        LinphoneCall* c = (LinphoneCall*) calls->data;
        if (linphone_call_get_state(c) == LinphoneCallIncoming || linphone_call_get_state(c) == LinphoneCallIncomingEarlyMedia) {
            linphone_core_accept_call([LinphoneManager getLc], c);
            return;
        }
        calls = calls->next;
    }
}

- (IBAction)onSnapshotClick:(id)sender {
    if(chatRoom != NULL) {
        linphone_chat_room_send_message(chatRoom, "3");
    }
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if ([LinphoneManager runningOnIpad]) {
        return YES;
    } else {
        return interfaceOrientation == UIInterfaceOrientationPortrait;
    }
}

- (NSUInteger)supportedInterfaceOrientations {
    if ([LinphoneManager runningOnIpad]) {
        return UIInterfaceOrientationMaskAll;
    } else {
        return UIInterfaceOrientationMaskPortrait;
    }
}

@end
