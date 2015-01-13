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

@implementation UIVideoButton {
    BOOL last_update_state;
}

@synthesize waitView;

- (void)initUIVideoButton {
    last_update_state = FALSE;
}

- (id)init{
    self = [super init];
    if (self) {
		[self initUIVideoButton];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUIVideoButton];
	}
    return self;
}	

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
		[self initUIVideoButton];
    }
    return self;
}

- (void)onOn {
	LinphoneCore* lc = [LinphoneManager getLc];
    
    if (!linphone_core_video_enabled(lc))
        return;
    
    [self setEnabled: FALSE];
    [waitView startAnimating];
    
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (call) {
		LinphoneCallAppData* callAppData = (LinphoneCallAppData*)linphone_call_get_user_pointer(call);
		callAppData->videoRequested=TRUE; /* will be used later to notify user if video was not activated because of the linphone core*/
        LinphoneCallParams* call_params =  linphone_call_params_copy(linphone_call_get_current_params(call));
        linphone_call_params_enable_video(call_params, TRUE);
        linphone_core_update_call(lc, call, call_params);
		linphone_call_params_destroy(call_params);
    } else {
		[LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot toggle video button, because no current call"];
	}   
}

- (void)onOff {
	LinphoneCore* lc = [LinphoneManager getLc];
    
    if (!linphone_core_video_enabled(lc))
        return;
    
    [self setEnabled: FALSE];
    [waitView startAnimating];
    
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
	if (call) { 
        LinphoneCallParams* call_params =  linphone_call_params_copy(linphone_call_get_current_params(call));
        linphone_call_params_enable_video(call_params, FALSE);
        linphone_core_update_call(lc, call, call_params);
		linphone_call_params_destroy(call_params);
    } else {
		[LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot toggle video button, because no current call"];
	}
}

- (bool)onUpdate {
    bool video_enabled = false;

#ifdef VIDEO_ENABLED
    LinphoneCall* currentCall = linphone_core_get_current_call([LinphoneManager getLc]);
    if( linphone_core_video_enabled([LinphoneManager getLc])
       && currentCall
       && !linphone_call_media_in_progress(currentCall)
       && linphone_call_get_state(currentCall) == LinphoneCallStreamsRunning) {
        video_enabled = TRUE;
    }
#endif //VIDEO_ENABLED

    [self setEnabled:video_enabled];
    if( last_update_state != video_enabled )
        [waitView stopAnimating];
    if( video_enabled ){
        video_enabled = linphone_call_params_video_enabled(linphone_call_get_current_params(currentCall));
    }
    last_update_state = video_enabled;

    return video_enabled;
}

- (void)dealloc {
    [waitView release];
    [super dealloc];
}

@end
