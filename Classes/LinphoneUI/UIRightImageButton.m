//
//  UIRightImageButton.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/10/15.
//
//

#import "UIRightImageButton.h"

@implementation UIRightImageButton

- (instancetype)invertImage {
	self.transform = CGAffineTransformMakeScale(-1.0, 1.0);
	self.titleLabel.transform = CGAffineTransformMakeScale(-1.0, 1.0);
	self.imageView.transform = CGAffineTransformMakeScale(-1.0, 1.0);

	self.contentHorizontalAlignment = (self.contentHorizontalAlignment == UIControlContentHorizontalAlignmentLeft)
										  ? UIControlContentHorizontalAlignmentRight
										  : UIControlContentHorizontalAlignmentLeft;

	return self;
}
- (instancetype)init {
	return [[super init] invertImage];
}

- (id)initWithCoder:(NSCoder *)aDecoder {
	return [[super initWithCoder:aDecoder] invertImage];
}

- (instancetype)initWithFrame:(CGRect)frame {
	return [[super initWithFrame:frame] invertImage];
}
@end
