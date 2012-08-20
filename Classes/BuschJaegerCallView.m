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
@synthesize startCallButton;
@synthesize takeCallButton;
@synthesize declineButton;
@synthesize endOrRejectCallButton;
@synthesize microButton;
@synthesize lightsButton;
@synthesize openDoorButton;


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}


#pragma mark - View lifecycle

- (void)dealloc {
    [videoView release];
    [startCallButton release];
    [takeCallButton release];
    [declineButton release];
    [endOrRejectCallButton release];
    [microButton release];
    [lightsButton release];
    [openDoorButton release];
    
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
    
    [openDoorButton setDigit:'1'];
    [lightsButton setDigit:'2'];
    [microButton setImage:[UIImage imageNamed:@"bj_mute_off.png"] forState:UIControlStateHighlighted | UIControlStateSelected];
    
    /* init gradients */
    {
        UIColor* col1 = BUSCHJAEGER_NORMAL_COLOR;
        UIColor* col2 = BUSCHJAEGER_NORMAL_COLOR2;
    
        [BuschJaegerUtils createGradientForButton:startCallButton withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForButton:openDoorButton withTopColor:col1 bottomColor:col2];
        [BuschJaegerUtils createGradientForButton:lightsButton withTopColor:col1 bottomColor:col2];
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
        
        [BuschJaegerUtils createGradientForView:takeCallButton withTopColor:col1 bottomColor:col2];
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
    
    [startCallButton setHidden:NO];
    [takeCallButton setHidden:YES];
    [microButton setHidden:NO];
    [declineButton setHidden:YES];
    [endOrRejectCallButton setHidden:YES];
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
    
    [microButton update];
    
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
    [startCallButton setHidden:YES];
    [takeCallButton setHidden:NO];
    [microButton setHidden:YES];
    [declineButton setHidden:NO];
    [endOrRejectCallButton setHidden:YES];
    [videoView setHidden:NO];
}

- (void)displayInCall {
    [startCallButton setHidden:YES];
    [takeCallButton setHidden:YES];
    [microButton setHidden:NO];
    [declineButton setHidden:YES];
    [endOrRejectCallButton setHidden:NO];
    [videoView setHidden:NO];
}

- (void)displayVideoCall {
    [startCallButton setHidden:YES];
    [takeCallButton setHidden:YES];
    [microButton setHidden:NO];
    [declineButton setHidden:YES];
    [endOrRejectCallButton setHidden:NO];
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
    [LinphoneLogger logc:LinphoneLoggerLog format:"Calling ADAPTER '%s'", adapter];
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
