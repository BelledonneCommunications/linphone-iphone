/* UILongTouchButton.h
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

#import "UILongTouchButton.h"

@implementation UILongTouchButton


#pragma mark - Lifecycle Functions

- (id)initUILongTouchButton {
	[self addTarget:self action:@selector(___touchDown:) forControlEvents:UIControlEventTouchDown];
    [self addTarget:self action:@selector(___touchUp:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside];
	return self;
}

- (id)init {
    self = [super init];
    if (self) {
		[self initUILongTouchButton];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
		[self initUILongTouchButton];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUILongTouchButton];
	}
    return self;
}	

- (void)dealloc {
	[self removeTarget:self action:@selector(___touchDown:) forControlEvents:UIControlEventTouchDown];
    [self removeTarget:self action:@selector(___touchUp:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside];
    [super dealloc];
}

- (void)___touchDown:(id) sender {
    [self performSelector:@selector(doLongTouch) withObject:nil afterDelay:0.5];
}

- (void)___touchUp:(id) sender {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(doLongTouch) object:nil];
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(doRepeatTouch) object:nil];
}

- (void)doLongTouch {
    [self onLongTouch];
    [self onRepeatTouch];
	[self performSelector:@selector(doRepeatTouch) withObject:nil afterDelay:0.1];
}

- (void)doRepeatTouch {
    [self onRepeatTouch];
	[self performSelector:@selector(doRepeatTouch) withObject:nil afterDelay:0.1];
}

- (void)onRepeatTouch {
}

- (void)onLongTouch {
}

@end
