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

#import "UIRoundBorderedButton.h"

#import "Utils.h"

@implementation UIRoundBorderedButton

- (id)initBorders {
	self.layer.borderWidth = .8;
	self.layer.borderColor = [self.titleLabel.textColor CGColor];
	self.layer.cornerRadius = 4.f;
	self.layer.masksToBounds = YES;

	// capitalize title (should be already done though)
	UIControlState states[] = {UIControlStateNormal,
							   UIControlStateHighlighted,
							   UIControlStateSelected,
							   UIControlStateDisabled,
							   UIControlStateDisabled | UIControlStateHighlighted,
							   UIControlStateSelected | UIControlStateHighlighted,
							   UIControlStateSelected | UIControlStateDisabled};
	for (int i = 0; i < sizeof(states) / sizeof(UIControlState); i++) {
		if (![[self titleForState:UIControlStateNormal]
					.uppercaseString isEqualToString:[self titleForState:states[i]]]) {
			[self setTitle:[[self titleForState:states[i]] uppercaseString] forState:states[i]];
		}
	}
	return self;
}

- (id)init {
	return [[super init] initBorders];
}

- (id)initWithCoder:(NSCoder *)aDecoder {
	return [[super initWithCoder:aDecoder] initBorders];
}

- (id)initWithFrame:(CGRect)frame {
	return [[super initWithFrame:frame] initBorders];
}

- (void)setEnabled:(BOOL)enabled {
	[super setEnabled:enabled];
	self.layer.borderColor = [self.titleLabel.textColor CGColor];
}

- (BOOL)becomeFirstResponder {
	if ([super becomeFirstResponder]) {
		[LinphoneUtils findAndResignFirstResponder:self.superview];
		return YES;
	}
	return NO;
}

-(void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection {
	self.layer.borderColor = [self.titleLabel.textColor CGColor];
}

@end
