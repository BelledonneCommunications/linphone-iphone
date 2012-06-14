/* UIPauseButton.m
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

#import "UIPauseButton.h"
#import "LinphoneManager.h"

#include "linphonecore.h"
#include "private.h"

@implementation UIPauseButton

- (void)onOn {
    LinphoneCall* currentCall = [UIPauseButton getCall];
    if (currentCall != nil) {
        linphone_core_pause_call([LinphoneManager getLc], currentCall);
    }
}

- (void)onOff {
    LinphoneCall* currentCall = [UIPauseButton getCall];
    if (currentCall != nil) {
        linphone_core_resume_call([LinphoneManager getLc], currentCall);
    }
}

+ (bool)isInConference: (LinphoneCall*) call {
    if (!call)
        return false;
    return linphone_call_get_current_params(call)->in_conference;
}

+ (int)notInConferenceCallCount: (LinphoneCore*) lc {
    int count = 0;
    const MSList* calls = linphone_core_get_calls(lc);
    
    while (calls != 0) {
        if (![UIPauseButton isInConference: (LinphoneCall*)calls->data]) {
            count++;
        }
        calls = calls->next;
    }
    return count;
}

- (bool)onUpdate {
    // TODO: disable pause on not running call
    @try {
        bool ret = false;
        LinphoneCall* currentCall = [UIPauseButton getCall];
        if (currentCall != nil) {
            LinphoneCallState state = linphone_call_get_state(currentCall);
            if(state == LinphoneCallPaused || state == LinphoneCallPausing) {
                ret = true;
            }
        }
        LinphoneCore* lc = [LinphoneManager getLc];
        int callsCount = linphone_core_get_calls_nb(lc);
        
        if (currentCall) {
            if (linphone_core_is_in_conference(lc)) {
                [LinphoneManager set:self enabled:FALSE withName:"PAUSE button" andReason:"is in conference"];
            } else if ([UIPauseButton notInConferenceCallCount: lc] == callsCount && callsCount == 1) {
                [LinphoneManager set:self enabled:TRUE withName:"PAUSE button" andReason:"call count == 1"];
            } else {
                [LinphoneManager set:self enabled:FALSE withName:"PAUSE button" andReason:""];
            }
        } else {
            [LinphoneManager set:self enabled:FALSE withName:"PAUSE button" andReason:""];
        }
        return ret;
    } @catch(NSException* e) {
		//not ready yet
		return false;
	}
}

+ (LinphoneCall*)getCall {
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* currentCall = linphone_core_get_current_call(lc);
	if (currentCall == nil && linphone_core_get_calls_nb(lc) == 1) {
        currentCall = (LinphoneCall*) linphone_core_get_calls(lc)->data;
    }
    return currentCall;
}

- (void)dealloc {
    [super dealloc];
}

@end
