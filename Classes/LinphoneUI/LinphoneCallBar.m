/* LinphoneCallBar.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import "LinphoneCallBar.h"
#import "LinphoneManager.h"

#include "linphonecore.h"
#include "private.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

@implementation LinphoneCallBar

@synthesize pauseButton;
@synthesize videoButton;
@synthesize microButton;
@synthesize speakerButton;   

- (void) viewDidLoad {
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(callUpdate:) name:@"LinphoneCallUpdate" object:nil];
}

bool isInConference2(LinphoneCall* call) {
    if (!call)
        return false;
    return linphone_call_get_current_params(call)->in_conference;
}

int callCount2(LinphoneCore* lc) {
    int count = 0;
    const MSList* calls = linphone_core_get_calls(lc);
    
    while (calls != 0) {
        if (!isInConference2((LinphoneCall*)calls->data)) {
            count++;
        }
        calls = calls->next;
    }
    return count;
}

-(IBAction) onPauseClick: (id) event {
    [LinphoneManager logUIElementPressed:"PAUSE button"];
    
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* currentCall = linphone_core_get_current_call(lc);
	if (currentCall) {
        if (linphone_call_get_state(currentCall) == LinphoneCallStreamsRunning) {
            [pauseButton setSelected:NO];
            linphone_core_pause_call(lc, currentCall);
            
            // hide video view
            //TODO
            //[self disableVideoDisplay];
        }
    } else {
        if (linphone_core_get_calls_nb(lc) == 1) {
            LinphoneCall* c = (LinphoneCall*) linphone_core_get_calls(lc)->data;
            if (linphone_call_get_state(c) == LinphoneCallPaused) {
                linphone_core_resume_call(lc, c);
                [pauseButton setSelected:YES];
                
                const LinphoneCallParams* p = linphone_call_get_current_params(c);
                if (linphone_call_params_video_enabled(p)) {
                    //TODO
                    //[self enableVideoDisplay];
                }
            }
        }
    }
}

- (void) callUpdate: (NSNotification*) notif {
    bool fullUpdate = true;
    // check LinphoneCore is initialized
    LinphoneCore* lc = nil;
    @try {
        lc = [LinphoneManager getLc];
    } @catch (NSException* exc) {
        return;
    }
    // 1 call: show pause button, otherwise show merge btn
    [LinphoneManager set:pauseButton enabled:(callCount2(lc) == 1) withName:"PAUSE button" andReason:"call count"];
    //TODO
    //[LinphoneManager set:mergeCalls hidden:!pause.hidden withName:"MERGE button" andReason:"call count"];     
    
    LinphoneCall* currentCall = linphone_core_get_current_call([LinphoneManager getLc]);
    int callsCount = linphone_core_get_calls_nb(lc);
    
    // hide pause/resume if in conference    
    if (currentCall) {
        [microButton reset];
        if (linphone_core_is_in_conference(lc)) {
            [LinphoneManager set:pauseButton enabled:FALSE withName:"PAUSE button" andReason:"is in conference"];
        }
        else if (callCount2(lc) == callsCount && callsCount == 1) {
            [LinphoneManager set:pauseButton enabled:TRUE withName:"PAUSE button" andReason:"call count == 1"];
            pauseButton.selected = NO;
        } else {
            [LinphoneManager set:pauseButton enabled:FALSE withName:"PAUSE button" andReason:AT];
        }
        
        if (fullUpdate) {
            //TODO
            //videoUpdateIndicator.hidden = YES;
            LinphoneCallState state = linphone_call_get_state(currentCall);
            if (state == LinphoneCallStreamsRunning || state == LinphoneCallUpdated || state == LinphoneCallUpdatedByRemote) {
                if (linphone_call_params_video_enabled(linphone_call_get_current_params(currentCall))) {
                    [videoButton setSelected:TRUE];
                } else {
                    [videoButton setSelected:FALSE];
                }
                [videoButton setEnabled:YES];
            } else {
                [videoButton setEnabled:NO];
                //[videoCallQuality setImage:nil];
            }
        }
    } else {
        if (callsCount == 1) {
            LinphoneCall* c = (LinphoneCall*)linphone_core_get_calls(lc)->data;
            if (linphone_call_get_state(c) == LinphoneCallPaused ||
                linphone_call_get_state(c) == LinphoneCallPausing) {
                pauseButton.selected = YES;                
            }
            [LinphoneManager set:pauseButton enabled:TRUE withName:"PAUSE button" andReason:AT];
        } else {
            [LinphoneManager set:pauseButton enabled:FALSE withName:"PAUSE button" andReason:AT];
        }
        [videoButton setEnabled:NO];
    }    
}

@end
