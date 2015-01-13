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


#pragma mark - Lifecycle Functions

- (id)initUICamSwitch {
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
	return self;
}

- (id)init {
    self = [super init];
    if (self) {
		[self initUICamSwitch];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    
    self = [super initWithFrame:frame];
    if (self) {
		[self initUICamSwitch];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUICamSwitch];
	}
    return self;
}	


- (void)dealloc {
    [super dealloc];
	[preview release];
}


#pragma mark - 

-(void) touchUp:(id) sender {
	const char *currentCamId = (char*)linphone_core_get_video_device([LinphoneManager getLc]);
	const char **cameras=linphone_core_get_video_devices([LinphoneManager getLc]);
	const char *newCamId=NULL;
	int i;
	
	for (i=0;cameras[i]!=NULL;++i){
		if (strcmp(cameras[i],"StaticImage: Static picture")==0) continue;
		if (strcmp(cameras[i],currentCamId)!=0){
			newCamId=cameras[i];
			break;
		}
	}
	if (newCamId){
		[LinphoneLogger logc:LinphoneLoggerLog format:"Switching from [%s] to [%s]", currentCamId, newCamId];
		linphone_core_set_video_device([LinphoneManager getLc], newCamId);
		LinphoneCall *call = linphone_core_get_current_call([LinphoneManager getLc]);
        if(call != NULL) {
            linphone_core_update_call([LinphoneManager getLc], call, NULL);
        }
		
	}
}

@end
