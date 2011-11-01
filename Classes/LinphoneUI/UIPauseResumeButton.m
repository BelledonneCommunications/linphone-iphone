/* UIMuteButton.m
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
#import "UIPauseResumeButton.h"
#include "LinphoneManager.h"


@implementation UIPauseResumeButton



-(void) onOn {
    LinphoneCall* c = linphone_core_get_current_call([LinphoneManager getLc]);
    if (c) {
        linphone_core_pause_call([LinphoneManager getLc], c);
    }
}
-(void) onOff {
	const MSList* c = linphone_core_get_calls([LinphoneManager getLc]);
    if (c) {
        linphone_core_resume_call([LinphoneManager getLc], (LinphoneCall*) c->data);
    }
}
-(bool) isInitialStateOn {
	@try {
        const MSList* c = linphone_core_get_calls([LinphoneManager getLc]);
        
        if (c) {
            return linphone_call_get_state((LinphoneCall*)c->data) == LinphoneCallPaused;
        } else {
            
            return false;
        }
	} @catch(NSException* e) {
		//not ready yet
		return false;
	}
	
}

/*
// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect {
    // Drawing code.
}
*/

- (void)dealloc {
    [super dealloc];
}


@end
