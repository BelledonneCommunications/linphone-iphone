/* UIHangUpButton.m
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

#import "UIHangUpButton.h"
#import "LinphoneManager.h"

@implementation UIHangUpButton

-(void) touchUp:(id) sender {
    LinphoneCore* lc = [LinphoneManager getLc];
    if (!lc)
        return;
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    
    if (call)
        linphone_core_terminate_call(lc,call);
	else if (linphone_core_is_in_conference(lc)) {
		linphone_core_terminate_conference(lc);
	} else {
		const MSList* calls = linphone_core_get_calls(lc);
		if (ms_list_size(calls) == 1 
			&& !linphone_call_params_local_conference_mode(linphone_call_get_current_params((LinphoneCall*)(calls->data)))) {
			//Only one call in the list, hangin up!
			linphone_core_terminate_call(lc,(LinphoneCall*)(calls->data));
		} else {
			ms_message("Cannot make a decision on which call to terminate");
		}
	}
}

- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
    if (self) {
		[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
    }
    return self;
}
- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
    }
    return self;
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
