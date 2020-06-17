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

#import "UIRoundedImageView.h"
#import <QuartzCore/QuartzCore.h>
#import "Utils.h"

@implementation UIRoundedImageView {
	UIView *borderView;
}

INIT_WITH_COMMON_CF {
	borderView = [[UIView alloc] initWithFrame:CGRectMake(0, 0, self.frame.size.width, self.frame.size.height)];
	borderView.layer.borderWidth = 10;
	borderView.layer.borderColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]].CGColor;
	borderView.hidden = YES;
	[self addSubview:borderView];

	[self setBordered:NO];
	[self setRoundRadius];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(orientationDidChange:)
											   name:UIDeviceOrientationDidChangeNotification
											 object:nil];
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)orientationDidChange:(NSNotification *)k {
	[self setRoundRadius];
	[self layoutSubviews];
}

- (void)setImage:(UIImage *)image {
	[self setImage:image bordered:NO withRoundedRadius:TRUE];
}

- (void)setImage:(UIImage *)image bordered:(BOOL)bordered withRoundedRadius:(BOOL)rounded {
	// We have to scale image to layers limits so that when we round image, we have a proper circle
	[super setImage:[image squareCrop]];
	[self setBordered:bordered];
	[self setRoundRadius];
}

- (void)setBordered:(BOOL)bordered {
	// bugged on rotation yet
	borderView.hidden = TRUE; //! bordered;
}
- (CGRect)computeBox {
	CGFloat min = MIN(self.frame.size.width, self.frame.size.height);
	CGRect box = CGRectMake((self.frame.size.width - min) / 2, (self.frame.size.height - min) / 2, min, min);
	return box;
}
- (void)setRoundRadius {
	CGRect box = [self computeBox];

	borderView.frame = box;
	borderView.layer.cornerRadius = borderView.frame.size.height / 2;

	CGPathRef path = CGPathCreateWithEllipseInRect(box, NULL);
	UIBezierPath *maskPath = [UIBezierPath bezierPathWithCGPath:path];
	CGPathRelease(path);
	CAShapeLayer *maskLayer = [CAShapeLayer layer];
	maskLayer.frame = self.bounds;
	maskLayer.path = maskPath.CGPath;
	self.layer.mask = maskLayer;
}

- (void)layoutSubviews {
	[super layoutSubviews];
	borderView.frame = [self computeBox];
	borderView.layer.cornerRadius = borderView.frame.size.height / 2;
}
@end
