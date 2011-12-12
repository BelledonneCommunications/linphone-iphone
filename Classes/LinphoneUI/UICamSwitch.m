/* UICamSwitch.m
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

#import "UICamSwitch.h"
#include "LinphoneManager.h"


@implementation UICamSwitch
@synthesize preview;
-(void) touchUp:(id) sender {
	if (nextCamId!=currentCamId) {
		ms_message("Swithcing from [%s] to [%s]",currentCamId,nextCamId);
		linphone_core_set_video_device([LinphoneManager getLc], nextCamId);
		nextCamId=currentCamId;
		currentCamId = linphone_core_get_video_device([LinphoneManager getLc]);
		linphone_core_update_call([LinphoneManager getLc]
								  , linphone_core_get_current_call([LinphoneManager getLc])
								  ,NULL);
		linphone_core_set_native_preview_window_id([LinphoneManager getLc],preview);
	}
}

- (id) init {
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
	currentCamId = (char*)linphone_core_get_video_device([LinphoneManager getLc]);
	if ([LinphoneManager instance].frontCamId !=nil ) {
		//ok, we have 2 cameras
		if (strcmp(currentCamId,[LinphoneManager instance].frontCamId)==0) {
			nextCamId = [LinphoneManager instance].backCamId;
		} else {
			nextCamId = [LinphoneManager instance].frontCamId;
		}
	} else {
		nextCamId=currentCamId;
	}
	return self;
}
- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
    if (self) {
		[self init];
    }
    return self;
}
- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self init];
	}
    return self;
}	


- (void)dealloc {
    [super dealloc];
	[preview release];
}





@end
