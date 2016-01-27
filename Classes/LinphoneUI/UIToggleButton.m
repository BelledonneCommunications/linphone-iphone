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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UIToggleButton.h"

@implementation UIToggleButton

#pragma mark - Lifecycle Functions

- (void)initUIToggleButton {
	[self update];
	[self addTarget:self action:@selector(touchUp:) forControlEvents:UIControlEventTouchUpInside];
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUIToggleButton];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initUIToggleButton];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUIToggleButton];
	}
	return self;
}
#pragma mark -

- (void)touchUp:(id)sender {
	[self toggle];
}

- (bool)toggle {
	if (self.selected) {
		self.selected = !self.selected;
		[self onOff];
	} else {
		self.selected = !self.selected;
		[self onOn];
	}
	return self.selected;
}

- (void)setOn {
	if (!self.selected) {
		[self toggle];
	}
}

- (void)setOff {
	if (self.selected) {
		[self toggle];
	}
}

- (bool)update {
	self.selected = [self onUpdate];
	return self.selected;
}

#pragma mark - UIToggleButtonDelegate Functions

- (void)onOn {
	/*[NSException raise:NSInternalInconsistencyException
				format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];*/
}

- (void)onOff {
	/*[NSException raise:NSInternalInconsistencyException
				format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];*/
}

- (bool)onUpdate {
	/*[NSException raise:NSInternalInconsistencyException
				format:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)];*/
	return false;
}

@end
