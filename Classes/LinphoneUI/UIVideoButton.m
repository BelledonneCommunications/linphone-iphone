/* UIToggleVideoButton.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */       

#import "UIVideoButton.h"
#include "LinphoneManager.h"

@implementation UIVideoButton

- (void)onOn {
	LinphoneCore* lc = [LinphoneManager getLc];
    
    if (!linphone_core_video_enabled(lc))
        return;
    
    [self setEnabled: FALSE];
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (call) { 
        LinphoneCallParams* call_params =  linphone_call_params_copy(linphone_call_get_current_params(call));
        linphone_call_params_enable_video(call_params, TRUE);
        linphone_core_update_call(lc, call, call_params);
		linphone_call_params_destroy(call_params);
    } else {
		ms_warning("Cannot toggle video, because no current call");
	}
        
}

- (void)onOff {
	LinphoneCore* lc = [LinphoneManager getLc];
    
    if (!linphone_core_video_enabled(lc))
        return;
    
    [self setEnabled: FALSE];
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (call) { 
        LinphoneCallParams* call_params =  linphone_call_params_copy(linphone_call_get_current_params(call));
        linphone_call_params_enable_video(call_params, FALSE);
        linphone_core_update_call(lc, call, call_params);
		linphone_call_params_destroy(call_params);
    } else {
		ms_warning("Cannot toggle video, because no current call");
	}
}

- (bool)onUpdate {
    if([LinphoneManager isLcReady]) {
        bool val = false;
        if(linphone_core_video_enabled([LinphoneManager getLc])) {
            LinphoneCall* currentCall = linphone_core_get_current_call([LinphoneManager getLc]);
            if (currentCall) {
                LinphoneCallState state = linphone_call_get_state(currentCall);
                if (state == LinphoneCallStreamsRunning || state == LinphoneCallUpdated || state == LinphoneCallUpdatedByRemote) {
                    if (linphone_call_params_video_enabled(linphone_call_get_current_params(currentCall))) {
                        val = true;
                    }
                    [self setEnabled:TRUE];
                } else {
                    // Disable button if the call is not running
                    [self setEnabled:FALSE];
                }
            } else {
                // Disable button if there is no call
                [self setEnabled:FALSE];
            }
        } else {
            // Disable button if video is not enabled
            [self setEnabled:FALSE];
        }
        return val;
    } else {
		//not ready yet
		return false;
	}
}

@end
