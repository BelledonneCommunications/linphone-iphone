/* ImageViewController.m
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

#import "ImageView.h"
#import "PhoneMainView.h"

@implementation UIImageScrollView

@synthesize image;
@synthesize imageView;

#pragma mark - Lifecycle Functions

- (void)initUIImageScrollView {
	imageView = [[UIImageView alloc] init];
	self.delegate = self;
	[self addSubview:imageView];
}

- (id)init {
	self = [super init];
	if (self != nil) {
		[self initUIImageScrollView];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
	self = [super initWithCoder:aDecoder];
	if (self != nil) {
		[self initUIImageScrollView];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self != nil) {
		[self initUIImageScrollView];
	}
	return self;
}

#pragma mark - ViewController Functions

- (void)layoutSubviews {
	[super layoutSubviews];
	// center the image as it becomes smaller than the size of the screen
	CGSize boundsSize = self.bounds.size;
	CGRect frameToCenter = imageView.frame;

	// center horizontally
	if (frameToCenter.size.width < boundsSize.width)
		frameToCenter.origin.x = (boundsSize.width - frameToCenter.size.width) / 2;
	else
		frameToCenter.origin.x = 0;

	// center vertically
	if (frameToCenter.size.height < boundsSize.height)
		frameToCenter.origin.y = (boundsSize.height - frameToCenter.size.height) / 2;
	else
		frameToCenter.origin.y = 0;

	imageView.frame = frameToCenter;
}

#pragma mark - Property Functions

- (void)setImage:(UIImage *)aimage {
	self.minimumZoomScale = 0;
	self.zoomScale = 1;

	CGRect rect = CGRectMake(0, 0, aimage.size.width, aimage.size.height);
	imageView.image = aimage;
	imageView.frame = rect;
	self.contentSize = rect.size;
	[self zoomToRect:rect animated:FALSE];
	self.minimumZoomScale = self.zoomScale;
}

- (UIImage *)image {
	return imageView.image;
}

#pragma mark - UIScrollViewDelegate Functions

- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView {
	return imageView;
}

@end

@implementation ImageView

@synthesize scrollView;
@synthesize backButton;
@synthesize image;

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:nil];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - Property Functions

- (void)setImage:(UIImage *)aimage {
	scrollView.image = aimage;
}

- (UIImage *)image {
	return scrollView.image;
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)sender {
	if ([[PhoneMainView.instance currentView] equal:ImageView.compositeViewDescription]) {
		[PhoneMainView.instance popCurrentView];
	}
}

@end
