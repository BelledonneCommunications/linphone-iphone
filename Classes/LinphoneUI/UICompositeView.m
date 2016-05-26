/* UICompositeView.m
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

#import "UICompositeView.h"

#import "LinphoneAppDelegate.h"
#import "Utils.h"
#import "SideMenuView.h"

@implementation UICompositeViewDescription

- (id)copy {
	UICompositeViewDescription *copy = [UICompositeViewDescription alloc];
	copy.name = self.name;
	copy.statusBar = self.statusBar;
	copy.tabBar = self.tabBar;
	copy.sideMenu = self.sideMenu;
	copy.statusBarEnabled = self.statusBarEnabled;
	copy.tabBarEnabled = self.tabBarEnabled;
	copy.sideMenuEnabled = self.sideMenuEnabled;
	copy.fullscreen = self.fullscreen;
	copy.landscapeMode = self.landscapeMode;
	copy.portraitMode = self.portraitMode;
	copy.isLeftFragment = self.isLeftFragment;
	copy.otherFragment = self.otherFragment;
	copy.darkBackground = self.darkBackground;
	return copy;
}

- (BOOL)equal:(UICompositeViewDescription *)description {
	return [self.name compare:description.name] == NSOrderedSame;
}

- (id)init:(Class)content
		 statusBar:(Class)statusBar
			tabBar:(Class)tabBar
		  sideMenu:(Class)sideMenu
		fullscreen:(BOOL)fullscreen
	isLeftFragment:(BOOL)isLeftFragment
	  fragmentWith:(Class)otherFragment {
	self.name = NSStringFromClass(content);
	self.statusBar = NSStringFromClass(statusBar);
	self.tabBar = NSStringFromClass(tabBar);
	self.sideMenu = NSStringFromClass(sideMenu);
	self.statusBarEnabled = YES;
	self.tabBarEnabled = YES;
	self.sideMenuEnabled = NO;
	self.fullscreen = fullscreen;
	self.landscapeMode = YES;
	self.portraitMode = YES;
	self.otherFragment = IPAD ? NSStringFromClass(otherFragment) : nil;
	self.isLeftFragment = isLeftFragment || (self.otherFragment == nil);
	self.darkBackground = true;

	return self;
}

@end
@interface UICompositeView ()

@property(nonatomic, strong) UIViewController *statusBarViewController;
@property(nonatomic, strong) UIViewController *tabBarViewController;
@property(nonatomic, strong) UIViewController *mainViewController;
@property(nonatomic, strong) UIViewController *detailsViewController;
@property(nonatomic, strong) UIViewController *sideMenuViewController;

@end

@implementation UICompositeView

#pragma mark - Lifecycle Functions

- (void)initUICompositeView {
	viewControllerCache = [[NSMutableDictionary alloc] init];
	currentOrientation = (UIInterfaceOrientation)UIDeviceOrientationUnknown;
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUICompositeView];
	}
	return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self) {
		[self initUICompositeView];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUICompositeView];
	}
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - ViewController Functions

- (void)updateViewsFramesAccordingToLaunchOrientation {
	CGRect frame =
		[self.view frame]; // this view has the correct size at launch (1024/768 for iPad, 320*{568,480} for iPhone)
	UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
	BOOL portrait = UIInterfaceOrientationIsPortrait(orientation);
	CGRect oppositeFrame = frame;
	oppositeFrame.size.height = frame.size.width;
	oppositeFrame.size.width = frame.size.height;

	// if we start in portrait, the landscape view must get the opposite height and width
	if (portrait || [[UIDevice currentDevice].systemVersion floatValue] < 8) {
		LOGI(@"landscape get opposite: %@", NSStringFromCGSize(oppositeFrame.size));
		[landscapeView setFrame:oppositeFrame];
	} else {
		// if we start in landscape, the landscape view has to get the current size,
		// whereas the portrait has to get the opposite
		LOGI(@"landscape get frame: %@ and portrait gets opposite: %@", NSStringFromCGSize(frame.size),
			 NSStringFromCGSize(oppositeFrame.size));
		[landscapeView setFrame:frame];
		[portraitView setFrame:oppositeFrame];
	}
}

- (void)viewDidLoad {
	/* Force landscape view to match portrait view, because portrait view inherits
	 the device screen size at load */
	[self updateViewsFramesAccordingToLaunchOrientation];
	[super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	[self.mainViewController viewWillAppear:animated];
	[self.detailsViewController viewWillAppear:animated];
	[self.tabBarViewController viewWillAppear:animated];
	[self.statusBarViewController viewWillAppear:animated];
	[self.sideMenuViewController viewWillAppear:animated];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(orientationDidChange:)
											   name:UIDeviceOrientationDidChangeNotification
											 object:nil];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	[self.mainViewController viewDidAppear:animated];
	[self.detailsViewController viewDidAppear:animated];
	[self.tabBarViewController viewDidAppear:animated];
	[self.statusBarViewController viewDidAppear:animated];
	[self.sideMenuViewController viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
	[self.mainViewController viewWillDisappear:animated];
	[self.detailsViewController viewWillDisappear:animated];
	[self.tabBarViewController viewWillDisappear:animated];
	[self.statusBarViewController viewWillDisappear:animated];
	[self.sideMenuViewController viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self name:UIDeviceOrientationDidChangeNotification object:nil];
	[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
	[super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
	[self.mainViewController viewDidDisappear:animated];
	[self.detailsViewController viewDidDisappear:animated];
	[self.tabBarViewController viewDidDisappear:animated];
	[self.statusBarViewController viewDidDisappear:animated];
	[self.sideMenuViewController viewDidDisappear:animated];
	[super viewDidDisappear:animated];
}

#pragma mark - Rotation messages

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
								duration:(NSTimeInterval)duration {
	currentOrientation = toInterfaceOrientation;
	[super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.mainViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.detailsViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.tabBarViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.statusBarViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.sideMenuViewController willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
										 duration:(NSTimeInterval)duration {
	[super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation
											duration:duration]; // Will invoke TPMultiLayout
	[self.mainViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.detailsViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.tabBarViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.statusBarViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self.sideMenuViewController willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
	[self update:nil tabBar:nil statusBar:nil sideMenu:nil fullscreen:nil];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self.mainViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self.detailsViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self.tabBarViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self.statusBarViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	[self.sideMenuViewController didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
	if (interfaceOrientation == currentOrientation)
		return YES;
	return NO;
}

#pragma mark - Event Functions

- (void)orientationDidChange:(NSNotification *)notif {
	@try {
		// Update rotation
		UIInterfaceOrientation correctOrientation =
			[self getCorrectInterfaceOrientation:[[UIDevice currentDevice] orientation]];
		if (currentOrientation != correctOrientation) {
			[UICompositeView setOrientation:correctOrientation
								   animated:currentOrientation != UIDeviceOrientationUnknown];
		}
	} @catch (NSException *exception) {
		// There are some crashes reports from iTunes connect because Linphone core is
		// not ready yet - whatever the reason is, we can safely ignore the exception
		LOGE(@"Exception: %@, ignoring", exception);
	}
}

#pragma mark -

/*
 Will simulate a device rotation
 */
+ (void)setOrientation:(UIInterfaceOrientation)orientation animated:(BOOL)animated {
	UIView *firstResponder = nil;

	UIViewController *controller = nil;

	controller = [[UIApplication sharedApplication] keyWindow].rootViewController;
	CGRect frame = [[UIScreen mainScreen] bounds];
	UIInterfaceOrientation oldOrientation = controller.interfaceOrientation;

	NSTimeInterval animationDuration = animated ? 0.3f : 0.0;

	[controller willRotateToInterfaceOrientation:orientation duration:animationDuration];
	[controller willAnimateRotationToInterfaceOrientation:orientation duration:animationDuration];
	[controller didRotateFromInterfaceOrientation:oldOrientation];
	[UIView animateWithDuration:animationDuration
					 animations:^{
					   [controller.view setFrame:frame];
					 }];

	if (firstResponder == nil) {
		firstResponder = [UICompositeView findFirstResponder:controller.view];
	}

	[[UIApplication sharedApplication] setStatusBarOrientation:orientation animated:animated];
	if (firstResponder) {
		[firstResponder resignFirstResponder];
		[firstResponder becomeFirstResponder];
	}
}

+ (UIView *)findFirstResponder:(UIView *)view {
	if (view.isFirstResponder) {
		return view;
	}
	for (UIView *subView in view.subviews) {
		UIView *ret = [UICompositeView findFirstResponder:subView];
		if (ret != nil)
			return ret;
	}
	return nil;
}

- (void)clearCache:(NSArray *)exclude {
	for (NSString *key in [viewControllerCache allKeys]) {
		bool remove = true;

		/*ImagePickerView can be used as popover and we do NOT want to free it*/;
		if ([key isEqualToString:ImagePickerView.compositeViewDescription.name]) {
			remove = false;
		} else if (exclude != nil) {
			for (UICompositeViewDescription *description in exclude) {
				if ([key isEqualToString:description.name] || [key isEqualToString:description.statusBar] ||
					[key isEqualToString:description.tabBar] || [key isEqualToString:description.sideMenu]) {
					remove = false;
					break;
				}
			}
		}
		if (remove) {
			LOGI(@"Free cached view: %@", key);
			[viewControllerCache removeObjectForKey:key];
		}
	}
}

- (UIInterfaceOrientation)currentOrientation {
	return currentOrientation;
}

- (IBAction)onRightSwipe:(id)sender {
	[self hideSideMenu:NO];
}

+ (void)addSubView:(UIViewController *)controller view:(UIView *)view {
	if (controller != nil) {
		[view addSubview:controller.view];
	}
}

+ (void)removeSubView:(UIViewController *)controller {
	if (controller != nil) {
		[controller.view removeFromSuperview];
	}
}

- (UIViewController *)getCachedController:(NSString *)name {
	UIViewController *controller = nil;
	if (name != nil) {
		controller = [viewControllerCache objectForKey:name];
		if (controller == nil) {
			controller = [[NSClassFromString(name) alloc] init];
			[viewControllerCache setValue:controller forKey:name];
			[controller view]; // Load the view
		}
	}
	return controller;
}

- (UIInterfaceOrientation)getCorrectInterfaceOrientation:(UIDeviceOrientation)deviceOrientation {
	if (currentViewDescription != nil) {
		// If unknown return status bar orientation
		if (deviceOrientation == UIDeviceOrientationUnknown && currentOrientation == UIDeviceOrientationUnknown) {
			return [UIApplication sharedApplication].statusBarOrientation;
		}

		// Don't rotate in UIDeviceOrientationFaceUp UIDeviceOrientationFaceDown
		if (!UIDeviceOrientationIsPortrait(deviceOrientation) && !UIDeviceOrientationIsLandscape(deviceOrientation)) {
			if (currentOrientation == UIDeviceOrientationUnknown) {
				return [UIApplication sharedApplication].statusBarOrientation;
			}
			deviceOrientation = (UIDeviceOrientation)currentOrientation;
		}
		if (UIDeviceOrientationIsPortrait(deviceOrientation)) {
			if ([currentViewDescription portraitMode]) {
				return (UIInterfaceOrientation)deviceOrientation;
			} else {
				return UIInterfaceOrientationLandscapeLeft;
			}
		}
		if (UIDeviceOrientationIsLandscape(deviceOrientation)) {
			if ([currentViewDescription landscapeMode]) {
				return (UIInterfaceOrientation)deviceOrientation;
			} else {
				return UIInterfaceOrientationPortrait;
			}
		}
	}
	return UIInterfaceOrientationPortrait;
}

#define IPHONE_STATUSBAR_HEIGHT 20

- (void)update:(UICompositeViewDescription *)description
		tabBar:(NSNumber *)tabBar
	 statusBar:(NSNumber *)statusBar
	  sideMenu:(NSNumber *)sideMenu
	fullscreen:(NSNumber *)fullscreen {

	UIViewController *oldMainViewController = self.mainViewController;
	UIViewController *oldDetailsViewController = self.detailsViewController;
	UIViewController *oldStatusBarViewController = self.statusBarViewController;
	UIViewController *oldTabBarViewController = self.tabBarViewController;
	UIViewController *oldSideMenuViewController = self.sideMenuViewController;
	// Copy view description
	UICompositeViewDescription *oldViewDescription = nil;

	if (description != nil) {
		oldViewDescription = currentViewDescription;
		currentViewDescription = [description copy];

		UIViewController *newMainViewController = description.isLeftFragment
													  ? [self getCachedController:description.name]
													  : [self getCachedController:description.otherFragment];
		UIViewController *newDetailsViewController = !description.isLeftFragment
														 ? [self getCachedController:description.name]
														 : [self getCachedController:description.otherFragment];
		UIViewController *newStatusBarViewController = [self getCachedController:description.statusBar];
		UIViewController *newTabBarViewController = [self getCachedController:description.tabBar];
		UIViewController *newSideMenuViewController = [self getCachedController:description.sideMenu];

		// Animate only with a previous screen
		if (oldViewDescription != nil && self.viewTransition != nil) {
			if (oldMainViewController != newMainViewController) {
				[self.mainView.layer removeAnimationForKey:@"transition"];
				[self.mainView.layer addAnimation:self.viewTransition forKey:@"transition"];
			} else {
				[self.mainView.layer removeAnimationForKey:@"transition"];
			}
			if (oldDetailsViewController != newDetailsViewController) {
				[self.detailsView.layer removeAnimationForKey:@"transition"];
				[self.detailsView.layer addAnimation:self.viewTransition forKey:@"transition"];
			} else {
				[self.detailsView.layer removeAnimationForKey:@"transition"];
			}

			if (oldStatusBarViewController != newStatusBarViewController ||
				oldViewDescription.statusBarEnabled != currentViewDescription.statusBarEnabled) {
				[self.statusBarView.layer removeAnimationForKey:@"transition"];
				[self.statusBarView.layer addAnimation:self.viewTransition forKey:@"transition"];
			} else {
				[self.statusBarView.layer removeAnimationForKey:@"transition"];
			}
			if (oldTabBarViewController != newTabBarViewController ||
				oldViewDescription.tabBarEnabled != currentViewDescription.tabBarEnabled) {
				[self.tabBarView.layer removeAnimationForKey:@"transition"];
				[self.tabBarView.layer addAnimation:self.viewTransition forKey:@"transition"];
			} else {
				[self.tabBarView.layer removeAnimationForKey:@"transition"];
			}
			if (oldSideMenuViewController != newSideMenuViewController ||
				oldViewDescription.sideMenuEnabled != currentViewDescription.sideMenuEnabled) {
				[self.sideMenuView.layer removeAnimationForKey:@"transition"];
				[self.sideMenuView.layer addAnimation:self.viewTransition forKey:@"transition"];
			}
		}

		if (oldMainViewController != nil && oldMainViewController != newMainViewController) {
			[UICompositeView removeSubView:oldMainViewController];
		}
		if (oldDetailsViewController != nil && oldDetailsViewController != newDetailsViewController) {
			[UICompositeView removeSubView:oldDetailsViewController];
		}
		if (oldTabBarViewController != nil && oldTabBarViewController != newTabBarViewController) {
			[UICompositeView removeSubView:oldTabBarViewController];
		}
		if (oldStatusBarViewController != nil && oldStatusBarViewController != newStatusBarViewController) {
			[UICompositeView removeSubView:oldStatusBarViewController];
		}
		if (oldSideMenuViewController != nil && oldSideMenuViewController != newSideMenuViewController) {
			[UICompositeView removeSubView:oldSideMenuViewController];
		}

		self.statusBarViewController = newStatusBarViewController;
		self.mainViewController = newMainViewController;
		self.detailsViewController = newDetailsViewController;
		self.tabBarViewController = newTabBarViewController;
		self.sideMenuViewController = newSideMenuViewController;

		// Update rotation
		UIInterfaceOrientation correctOrientation = [self
			getCorrectInterfaceOrientation:(UIDeviceOrientation)[UIApplication sharedApplication].statusBarOrientation];
		if (currentOrientation != correctOrientation) {
			[UICompositeView setOrientation:correctOrientation
								   animated:currentOrientation != UIDeviceOrientationUnknown];
			if (UIInterfaceOrientationIsLandscape(correctOrientation)) {
				[self.mainViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.detailsViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.tabBarViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.statusBarViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.sideMenuViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
			}
		} else {
			if (oldMainViewController != newMainViewController) {
				UIInterfaceOrientation oldOrientation = self.mainViewController.interfaceOrientation;
				[self.mainViewController willRotateToInterfaceOrientation:correctOrientation duration:0];
				[self.mainViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.mainViewController didRotateFromInterfaceOrientation:oldOrientation];
			}
			if (oldDetailsViewController != newDetailsViewController) {
				UIInterfaceOrientation oldOrientation = self.detailsViewController.interfaceOrientation;
				[self.detailsViewController willRotateToInterfaceOrientation:correctOrientation duration:0];
				[self.detailsViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.detailsViewController didRotateFromInterfaceOrientation:oldOrientation];
			}
			if (oldTabBarViewController != newTabBarViewController) {
				UIInterfaceOrientation oldOrientation = self.tabBarViewController.interfaceOrientation;
				[self.tabBarViewController willRotateToInterfaceOrientation:correctOrientation duration:0];
				[self.tabBarViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.tabBarViewController didRotateFromInterfaceOrientation:oldOrientation];
			}
			if (oldSideMenuViewController != newSideMenuViewController) {
				UIInterfaceOrientation oldOrientation = self.sideMenuViewController.interfaceOrientation;
				[self.sideMenuViewController willRotateToInterfaceOrientation:correctOrientation duration:0];
				[self.sideMenuViewController willAnimateRotationToInterfaceOrientation:correctOrientation duration:0];
				[self.sideMenuViewController didRotateFromInterfaceOrientation:oldOrientation];
			}
		}
	}

	if (currentViewDescription == nil) {
		return;
	}

	if (tabBar != nil) {
		if (currentViewDescription.tabBarEnabled != [tabBar boolValue]) {
			currentViewDescription.tabBarEnabled = [tabBar boolValue];
		} else {
			tabBar = nil; // No change = No Update
		}
	}

	if (statusBar != nil) {
		if (currentViewDescription.statusBarEnabled != [statusBar boolValue]) {
			currentViewDescription.statusBarEnabled = [statusBar boolValue];
		} else {
			statusBar = nil; // No change = No Update
		}
	}

	if (sideMenu != nil) {
		if (currentViewDescription.sideMenuEnabled != [sideMenu boolValue]) {
			currentViewDescription.sideMenuEnabled = [sideMenu boolValue];
			if (currentViewDescription.sideMenuEnabled) {
				[_sideMenuViewController viewWillAppear:YES];
			} else {
				[_sideMenuViewController viewWillDisappear:YES];
			}
		} else {
			sideMenu = nil; // No change = No Update
		}
	}

	if (fullscreen != nil) {
		if (currentViewDescription.fullscreen != [fullscreen boolValue]) {
			currentViewDescription.fullscreen = [fullscreen boolValue];
			[[UIApplication sharedApplication] setStatusBarHidden:currentViewDescription.fullscreen
													withAnimation:UIStatusBarAnimationSlide];
		} else {
			fullscreen = nil; // No change = No Update
		}
	} else {
		[[UIApplication sharedApplication] setStatusBarHidden:currentViewDescription.fullscreen
												withAnimation:UIStatusBarAnimationNone];
	}

	// Start animation
	if (tabBar != nil || statusBar != nil || sideMenu != nil || fullscreen != nil) {
		[UIView beginAnimations:@"resize" context:nil];
		[UIView setAnimationDuration:0.35];
	}

	// Compute frame for each elements
	CGRect viewFrame = self.view.frame;
	int origin = currentViewDescription.fullscreen ? 0 : IPHONE_STATUSBAR_HEIGHT;

	// 1. status bar - fixed size on top
	CGRect statusBarFrame = self.statusBarView.frame;
	if (self.statusBarViewController != nil && currentViewDescription.statusBarEnabled) {
		statusBarFrame.origin.y = origin;
		// move origin below status bar
		origin += statusBarFrame.size.height;
	} else {
		statusBarFrame.origin.y = -statusBarFrame.size.height;
	}

	//	2. side menu - fixed size, always starting below status bar (hack: except in fullscreen)
	CGRect sideMenuFrame = viewFrame;
	sideMenuFrame.origin.y = origin - (currentViewDescription.fullscreen ? statusBarFrame.size.height : 0);
	sideMenuFrame.size.height -= sideMenuFrame.origin.y;
	if (!currentViewDescription.sideMenuEnabled) {
		// hack bis: really hide; -width won't be enough since some animations may use this...
		sideMenuFrame.origin.x = -3 * sideMenuFrame.size.width;
	}

	//	3. tab bar - on portrait full width at bottom / on landscape on left, starting below status bar
	// Resize TabBar
	CGRect tabFrame = self.tabBarView.frame;
	if (self.tabBarViewController != nil && currentViewDescription.tabBarEnabled) {
		tabFrame.origin.x = 0;
		if (UIInterfaceOrientationIsPortrait([self currentOrientation])) {
			tabFrame.origin.y = viewFrame.size.height - tabFrame.size.height;
		} else {
			tabFrame.origin.y = origin;
			tabFrame.size.height = viewFrame.size.height - tabFrame.origin.y;
		}
	} else {
		tabFrame.origin.x = -tabFrame.size.width;
		tabFrame.origin.y = viewFrame.size.height;
	}

	//	4. main view and details view - space left width of 35%/65% each
	CGRect mainFrame = viewFrame;
	mainFrame.origin.y = origin;
	mainFrame.size.height -= mainFrame.origin.y;
	if (!currentViewDescription.fullscreen) {
		if (UIInterfaceOrientationIsPortrait([self currentOrientation])) {
			mainFrame.size.height -= viewFrame.size.height - tabFrame.origin.y;
		} else {
			mainFrame.origin.x = tabFrame.origin.x + tabFrame.size.width;
			mainFrame.size.width -= mainFrame.origin.x;
		}
	}
	CGRect detailsFrame = mainFrame;
	if (self.detailsViewController != nil) {
		detailsFrame = mainFrame;
		mainFrame.size.width = ceil(mainFrame.size.width * .35);
		detailsFrame.size.width -= mainFrame.size.width;
		detailsFrame.origin.x += mainFrame.size.width;
	}

	// Set frames
	// 1. main view and details view
	self.mainView.frame = mainFrame;
	self.mainViewController.view.frame = self.mainView.bounds;
	self.detailsView.frame = detailsFrame;
	self.detailsViewController.view.frame = self.detailsView.bounds;

	// 2. tab bar
	self.tabBarView.frame = tabFrame;
	CGRect frame = self.tabBarViewController.view.frame;
	frame.size = self.tabBarView.bounds.size;
	self.tabBarViewController.view.frame = frame;

	// 3. status bar
	self.statusBarView.frame = statusBarFrame;
	frame = self.statusBarViewController.view.frame;
	frame.size = self.statusBarView.bounds.size;
	self.statusBarViewController.view.frame = frame;

	// 4. side menu
	self.sideMenuView.frame = sideMenuFrame;
	self.sideMenuViewController.view.frame = self.sideMenuView.bounds;

	// Commit animation
	if (tabBar != nil || statusBar != nil || sideMenu != nil || fullscreen != nil) {
		[UIView commitAnimations];
	}

	// Change view
	if (description != nil) {
		if (oldMainViewController == nil || oldMainViewController != self.tabBarViewController) {
			[UICompositeView addSubView:self.mainViewController view:self.mainView];
		}
		if (oldDetailsViewController == nil || oldDetailsViewController != self.detailsViewController) {
			[UICompositeView addSubView:self.detailsViewController view:self.detailsView];
		}
		if (oldTabBarViewController == nil || oldTabBarViewController != self.tabBarViewController) {
			[UICompositeView addSubView:self.tabBarViewController view:self.tabBarView];
		}
		if (oldStatusBarViewController == nil || oldStatusBarViewController != self.statusBarViewController) {
			[UICompositeView addSubView:self.statusBarViewController view:self.statusBarView];
		}
		if (oldSideMenuViewController == nil || oldSideMenuViewController != self.sideMenuViewController) {
			[UICompositeView addSubView:self.sideMenuViewController view:self.sideMenuView];
		}
	}
	if (currentViewDescription.sideMenuEnabled) {
		[_sideMenuViewController viewDidAppear:YES];
	} else {
		[_sideMenuViewController viewDidDisappear:YES];
	}
	// Dealloc old view description
}

- (void)changeView:(UICompositeViewDescription *)description {
	[self view]; // Force view load
	[self update:description tabBar:nil statusBar:nil sideMenu:nil fullscreen:nil];
}

- (void)setFullscreen:(BOOL)enabled {
	[self update:nil tabBar:nil statusBar:nil sideMenu:nil fullscreen:[NSNumber numberWithBool:enabled]];
}

- (void)hideTabBar:(BOOL)hidden {
	[self update:nil tabBar:[NSNumber numberWithBool:!hidden] statusBar:nil sideMenu:nil fullscreen:nil];
}

- (void)hideStatusBar:(BOOL)hidden {
	[self update:nil tabBar:nil statusBar:[NSNumber numberWithBool:!hidden] sideMenu:nil fullscreen:nil];
}

- (void)hideSideMenu:(BOOL)hidden {
	[self update:nil tabBar:nil statusBar:nil sideMenu:[NSNumber numberWithBool:!hidden] fullscreen:nil];
}
- (UIViewController *)getCurrentViewController {
	return self.mainViewController;
}

- (BOOL)currentViewSupportsLandscape {
	return currentViewDescription ? currentViewDescription.landscapeMode : FALSE;
}

@end
