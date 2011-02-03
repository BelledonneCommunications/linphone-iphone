/* UIToggleButton.m
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
#import "UIToggleButton.h"


@implementation UIToggleButton

-(void) touchUp:(id) sender {
	[self toggle];
}
-(bool) isOn {
	return mIsOn;
}
-(bool) toggle {
	if (mIsOn) {
		[self setImage:mOffImage forState:UIControlStateNormal];
		mIsOn=!mIsOn;
		[self onOff];
	} else {
		[self setImage:mOnImage forState:UIControlStateNormal];
		mIsOn=!mIsOn;
		[self onOn];
	}
	return mIsOn;
	
}
-(bool) reset {
	mIsOn = [self isInitialStateOn];
	[self setImage:mIsOn?mOnImage:mOffImage forState:UIControlStateNormal];
	return mIsOn;
}

-(void) initWithOnImage:(UIImage*) onImage offImage:(UIImage*) offImage {
	mOnImage = [onImage retain];
	mOffImage = [offImage retain];
	mIsOn=false;
	[self reset];
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
	
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
	[mOffImage release];
	[mOffImage release];
}


@end
