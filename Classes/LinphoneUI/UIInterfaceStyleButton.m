/*
* Copyright (c) 2010-2020 Belledonne Communications SARL.
*
* This file is part of linphone-iphone
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#import <Foundation/Foundation.h>
#import "UIInterfaceStyleButton.h"
#import "Utils.h"


@implementation UIInterfaceStyleButton
- (void)initUIInterfaceStyleButton {
	if(@available(iOS 13,*)){
		[super setImage:[[self imageForState:UIControlStateNormal] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate] forState:UIControlStateNormal];
		[super setImage:[[self imageForState:UIControlStateDisabled] imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate] forState:UIControlStateDisabled];
		UITraitCollection *collection = [UITraitCollection currentTraitCollection];
		switch (collection.userInterfaceStyle) {
			case UIUserInterfaceStyleDark:
				self.tintColor = [UIColor whiteColor];
				break;
			case UIUserInterfaceStyleUnspecified:
			case UIUserInterfaceStyleLight:
				self.tintColor = [UIColor darkGrayColor];
				break;
			default:
				break;
		}
	} else {
		self.tintColor = [UIColor darkGrayColor];
	}
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUIInterfaceStyleButton];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUIInterfaceStyleButton];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initUIInterfaceStyleButton];
	}
	return self;
}

-(void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
	[super traitCollectionDidChange:previousTraitCollection];
	[self initUIInterfaceStyleButton];
}

-(void)setImage:(UIImage *)image forState:(UIControlState)state {
	[super setImage:[image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate] forState:state];
}
@end
