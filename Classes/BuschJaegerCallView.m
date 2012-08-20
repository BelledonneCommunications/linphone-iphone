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
#import "BuschJaegerUtils.h"
#include "linphonecore.h"
#import <QuartzCore/QuartzCore.h>
#import <AudioToolbox/AudioToolbox.h>

@implementation BuschJaegerCallView

@synthesize videoView;
@synthesize startCall;
@synthesize takeCall;
@synthesize decline;
@synthesize endOrRejectCall;
@synthesize mute;
@synthesize lights;
@synthesize openDoor;


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


#pragma mark - View lifecycle

- (void)dealloc {
    [videoView release];
    [startCall release];
    [takeCall release];
    [decline release];
    [endOrRejectCall release];
    [mute release];
    [lights release];
    [openDoor release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}

// 59x47
// 54
// 54 -> 67
// 59 -> 73 x 58
// 257
- (void)viewDidLoad {
    [super viewDidLoad];
    
    [openDoor setDigit:'1'];
    [lights setDigit:'2'];
    [mute setImage:[UIImage imageNamed:@"bj_mute_on.png"] forState:UIControlStateHighlighted | UIControlStateSelected];
    
    /* init gradients */
    {
        UIColor* col1 = [UIColor colorWithRed:32.0/255 green:45.0/255 blue:62.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:18.0/255 green:26.0/255 blue:41.0/255 alpha:1.0];
    
        [BuschJaegerUtils createGradientForView:startCall withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForView:openDoor withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForView:lights withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForView:mute withTopColor:col1 bottomColor:col2];
    }
    {
        UIColor* col1 = [UIColor colorWithRed:153.0/255 green:48.0/255 blue:48.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:66.0/255 green:15.0/255 blue:15.0/255 alpha:1.0];
        
        [BuschJaegerUtils createGradientForView:endOrRejectCall withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForView:decline withTopColor:col1 bottomColor:col2];
    }
    {
        UIColor* col1 = [UIColor colorWithRed:91.0/255 green:161.0/255 blue:89.0/255 alpha:1.0];
        UIColor* col2 = [UIColor colorWithRed:25.0/255 green:54.0/255 blue:24.0/255 alpha:1.0];
        
        [BuschJaegerUtils createGradientForView:takeCall withTopColor:col1 bottomColor:col2];
    }
    
    linphone_core_set_native_video_window_id([LinphoneManager getLc],(unsigned long)videoView);
    linphone_core_set_native_preview_window_id([LinphoneManager getLc],0);
    
    videoZoomHandler = [[VideoZoomHandler alloc] init];
    [videoZoomHandler setup:videoView];
}

- (void)viewDidUnload {
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(callUpdateEvent:)
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    
    [startCall setHidden:NO];
    [takeCall setHidden:YES];
    [mute setHidden:NO];
    [decline setHidden:YES];
    [endOrRejectCall setHidden:YES];
    [videoView setHidden:YES];
    
    if (!chatRoom) {
        NSString* s = [NSString stringWithFormat:@"sip:100000001@%@", [[NSUserDefaults standardUserDefaults] stringForKey:@"adapter_ip_preference"]];
        const char* adapter = [s cStringUsingEncoding:[NSString defaultCStringEncoding]];
        chatRoom = linphone_core_create_chat_room([LinphoneManager getLc], adapter);
        
        //lights->chatRoom = chatRoom;
        //openDoor->chatRoom = chatRoom;
    }
    
    // Update on show
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    LinphoneCallState state = (call != NULL)?linphone_call_get_state(call): 0;
    [self callUpdate:call state:state animated:FALSE];
}

- (void)vieWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneCallUpdate
                                                  object:nil];
}


#pragma mark - Event Functions

- (void)callUpdateEvent: (NSNotification*) notif {
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:call state:state animated:TRUE];
}


#pragma mark - 

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {    
    // Fake call update
    if(call == NULL) {
        return;
    }
    
	switch (state) {
		case LinphoneCallIncomingReceived:
        {
            [self displayIncomingCall:call];
        }
		case LinphoneCallOutgoingInit:
		case LinphoneCallConnected:
		case LinphoneCallStreamsRunning:
        case LinphoneCallUpdated:
        {
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
    [startCall setHidden:YES];
    [takeCall setHidden:NO];
    [mute setHidden:YES];
    [decline setHidden:NO];
    [endOrRejectCall setHidden:YES];
    [videoView setHidden:NO];
}

- (void)displayInCall {
    [startCall setHidden:YES];
    [takeCall setHidden:YES];
    [mute setHidden:NO];
    [decline setHidden:YES];
    [endOrRejectCall setHidden:NO];
    [videoView setHidden:NO];
}

- (void)displayVideoCall {
    [startCall setHidden:YES];
    [takeCall setHidden:YES];
    [mute setHidden:NO];
    [decline setHidden:YES];
    [endOrRejectCall setHidden:NO];
    [videoView setHidden:NO];
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

- (IBAction)startCall:(id)sender {
    // no pending call, call adapter
    NSString* s = [NSString stringWithFormat:@"sip:100000001@%@", [[NSUserDefaults standardUserDefaults] stringForKey:@"adapter_ip_preference"]];
    const char* adapter = [s cStringUsingEncoding:[NSString defaultCStringEncoding]];
    ms_message("Calling ADAPTER '%s'", adapter);
    LinphoneCallParams* lcallParams = linphone_core_create_default_call_parameters([LinphoneManager getLc]);
    linphone_call_params_enable_video(lcallParams, true);
    LinphoneCall* lc = linphone_core_invite_with_params([LinphoneManager getLc], adapter,lcallParams);
    if (!lc) {
        ms_error("Failed to start a new call");
        return;
    }
    linphone_call_enable_camera(lc, false);  
    linphone_call_params_destroy(lcallParams);
}

@end
