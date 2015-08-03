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
	self.layer.borderColor = [[UIColor blackColor] CGColor];
	self.layer.cornerRadius = 4.f;

	// capitalize title (should be already done though)
	UIControlState states[] = {UIControlStateNormal, UIControlStateHighlighted, UIControlStateSelected,
							   UIControlStateDisabled, UIControlStateDisabled | UIControlStateHighlighted,
							   UIControlStateSelected | UIControlStateHighlighted,
							   UIControlStateSelected | UIControlStateDisabled};
	for (int i = 0; i < sizeof(states) / sizeof(UIControlState); i++) {
		[self setTitle:[[self titleForState:states[i]] uppercaseString] forState:states[i]];
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

@end
