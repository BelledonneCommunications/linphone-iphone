
//
//  UIRoundBorderedButton.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 22/07/15.
//
//

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

@end
