/* UIDuration.m
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



#import "UIDuration.h"
#import "LinphoneManager.h"


@implementation UIDuration

-(void)updateCallDuration {
	int lDuration = linphone_core_get_current_call_duration([LinphoneManager getLc]); 
	if (lDuration < 60) {
		[self setText:[NSString stringWithFormat: @"%02i s", lDuration]];
	} else {
		[self setText:[NSString stringWithFormat: @"%02i:%02i", lDuration/60,lDuration - 60 *(lDuration/60)]];
	}
}

-(void) start {
	[self setText:@"00 s"];
	durationRefreasher = [NSTimer	scheduledTimerWithTimeInterval:1 
														  target:self 
														selector:@selector(updateCallDuration) 
														userInfo:nil 
														 repeats:YES];
}
-(void) stop {
	[durationRefreasher invalidate];
	durationRefreasher=nil;
	
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
	[durationRefreasher invalidate];
}


@end
