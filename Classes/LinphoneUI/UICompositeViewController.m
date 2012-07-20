/* UICompositeViewController.m
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

#import "UICompositeViewController.h"

@implementation UICompositeViewDescription

@synthesize name;
@synthesize content;
@synthesize stateBar;
@synthesize stateBarEnabled;
@synthesize tabBar;
@synthesize tabBarEnabled;
@synthesize fullscreen;

- (id)copy {
    UICompositeViewDescription *copy = [UICompositeViewDescription alloc];
    copy.content = self.content;
    copy.stateBar = self.stateBar;
    copy.stateBarEnabled = self.stateBarEnabled;
    copy.tabBar = self.tabBar;
    copy.tabBarEnabled = self.tabBarEnabled;
    copy.fullscreen = self.fullscreen;
    return copy;
}

- (BOOL)equal:(UICompositeViewDescription*) description {
    return [self.name compare:description.name] == NSOrderedSame;
}

- (id)init:(NSString *)aname content:(NSString *)acontent stateBar:(NSString*)astateBar 
                        stateBarEnabled:(BOOL) astateBarEnabled 
                                 tabBar:(NSString*)atabBar
                          tabBarEnabled:(BOOL) atabBarEnabled
                             fullscreen:(BOOL) afullscreen {
    self.name = aname;
    self.content = acontent;
    self.stateBar = astateBar;
    self.stateBarEnabled = astateBarEnabled;
    self.tabBar = atabBar;
    self.tabBarEnabled = atabBarEnabled;
    self.fullscreen = afullscreen;
    
    return self;
}

- (void)dealloc {
    [name release];
    [content release];
    [stateBar release];
    [tabBar release];
    [super dealloc];
}

@end

@implementation UICompositeViewController

@synthesize stateBarView;
@synthesize contentView;
@synthesize tabBarView;

@synthesize viewTransition;


#pragma mark - Lifecycle Functions

- (void)initUICompositeViewController {
    self->viewControllerCache = [[NSMutableDictionary alloc] init]; 
}

- (id)init{
    self = [super init];
    if (self) {
		[self initUICompositeViewController];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self initUICompositeViewController];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initUICompositeViewController];
	}
    return self;
}	

- (void)dealloc {
    [contentView release];
    [stateBarView release];
    [tabBarView release];
    [viewControllerCache removeAllObjects];
    [currentViewDescription release];
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [contentViewController viewWillAppear:animated];
    [tabBarViewController viewWillAppear:animated];
    [stateBarViewController viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [contentViewController viewDidAppear:animated];
    [tabBarViewController viewDidAppear:animated];
    [stateBarViewController viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [contentViewController viewWillDisappear:animated];
    [tabBarViewController viewWillDisappear:animated];
    [stateBarViewController viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    [contentViewController viewDidDisappear:animated];
    [tabBarViewController viewDidDisappear:animated];
    [stateBarViewController viewDidDisappear:animated];
}


#pragma mark - 

+ (void)addSubView:(UIViewController*)controller view:(UIView*)view {
    if(controller != nil) {
        [controller view]; // Load the view
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [controller viewWillAppear:NO];
        }
        [view addSubview: controller.view];
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [controller viewDidAppear:NO];
        }
    }
}

+ (void)removeSubView:(UIViewController*)controller {
    if(controller != nil) {
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [controller viewWillDisappear:NO];
        }
        [controller.view removeFromSuperview];
        if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
            [controller viewDidDisappear:NO];
        }
    }
}

- (UIViewController*)getCachedController:(NSString*)name {
    UIViewController *controller = nil;
    if(name != nil) {
        controller = [viewControllerCache objectForKey:name];
        if(controller == nil) {
            controller = [[NSClassFromString(name) alloc] init];
            [viewControllerCache setValue:controller forKey:name];
        }
    }
    return controller;
}

#define IPHONE_STATUSBAR_HEIGHT 20

- (void)update: (UICompositeViewDescription*) description tabBar:(NSNumber*)tabBar fullscreen:(NSNumber*)fullscreen {   
    
    // Copy view description
    UICompositeViewDescription *oldViewDescription = (currentViewDescription != nil)? [currentViewDescription copy]: nil;

    if(description != nil) {
        currentViewDescription = [description copy];
        
        // Animate only with a previous screen
        if(oldViewDescription != nil && viewTransition != nil) {
            [contentView.layer addAnimation:viewTransition forKey:@"Transition"];
            if((oldViewDescription.stateBarEnabled == true && currentViewDescription.stateBarEnabled == false) ||
               (oldViewDescription.stateBarEnabled == false && currentViewDescription.stateBarEnabled == true)) {
                [stateBarView.layer addAnimation:viewTransition forKey:@"Transition"];
            }
            if(oldViewDescription.tabBar != currentViewDescription.tabBar) {
                [tabBarView.layer addAnimation:viewTransition forKey:@"Transition"];
            }
        }
        
        [UICompositeViewController removeSubView: contentViewController];
        if(oldViewDescription != nil && tabBarViewController != nil && oldViewDescription.tabBar != currentViewDescription.tabBar) {
            [UICompositeViewController removeSubView: tabBarViewController];
        }
        if(oldViewDescription != nil && stateBarViewController != nil && oldViewDescription.stateBar != currentViewDescription.stateBar) {
            [UICompositeViewController removeSubView: stateBarViewController];
        }

        stateBarViewController = [self getCachedController:description.stateBar];
        contentViewController = [self getCachedController:description.content];
        tabBarViewController = [self getCachedController:description.tabBar];
    }
    
    if(currentViewDescription == nil) {
        return;
    }
    
    if(tabBar != nil) {
        if(currentViewDescription.tabBarEnabled != [tabBar boolValue]) {
            currentViewDescription.tabBarEnabled = [tabBar boolValue];
        } else {
            tabBar = nil; // No change = No Update
        }
    }
    
    if(fullscreen != nil) {
        if(currentViewDescription.fullscreen != [fullscreen boolValue]) {
            currentViewDescription.fullscreen = [fullscreen boolValue];
            [[UIApplication sharedApplication] setStatusBarHidden:currentViewDescription.fullscreen withAnimation:UIStatusBarAnimationSlide];
        } else {
            fullscreen = nil; // No change = No Update
        }
    } else {
        [[UIApplication sharedApplication] setStatusBarHidden:currentViewDescription.fullscreen withAnimation:UIStatusBarAnimationNone];
    }
    
    // Start animation
    if(tabBar != nil || fullscreen != nil) {
        [UIView beginAnimations:@"resize" context:nil];
        [UIView setAnimationDuration:0.35];
        [UIView setAnimationBeginsFromCurrentState:TRUE];
    }
    
    UIView *innerView = contentViewController.view;
    
    CGRect contentFrame = contentView.frame;
    CGRect viewFrame = [self.view frame];
    
    // Resize StateBar
    CGRect stateBarFrame = stateBarView.frame;
    int origin = IPHONE_STATUSBAR_HEIGHT;
    if(currentViewDescription.fullscreen)
        origin = 0;
    
    if(stateBarViewController != nil && currentViewDescription.stateBarEnabled) {
        contentFrame.origin.y = origin + stateBarFrame.size.height;
        stateBarFrame.origin.y = origin;
    } else {
        contentFrame.origin.y = origin;
        stateBarFrame.origin.y = origin - stateBarFrame.size.height;
    }
    
    // Resize TabBar
    CGRect tabFrame = tabBarView.frame;
    if(tabBarViewController != nil && currentViewDescription.tabBarEnabled) {
        tabFrame.origin.y = viewFrame.size.height;
        tabFrame.origin.x = viewFrame.size.width;
        tabFrame.size.height = tabBarViewController.view.frame.size.height;
        tabFrame.size.width = tabBarViewController.view.frame.size.width;
        tabFrame.origin.y -= tabFrame.size.height;
        tabFrame.origin.x -= tabFrame.size.width;
        contentFrame.size.height = tabFrame.origin.y - contentFrame.origin.y;
        for (UIView *view in tabBarViewController.view.subviews) {
            if(view.tag == -1) {
                contentFrame.size.height += view.frame.origin.y;
                break;
            }
        }
    } else {
        contentFrame.size.height = viewFrame.size.height - contentFrame.origin.y;
        tabFrame.origin.y = viewFrame.size.height;
    }
    
    if(currentViewDescription.fullscreen)
        contentFrame.size.height = viewFrame.size.height - contentFrame.origin.y;
    
    // Resize innerView
    CGRect innerContentFrame = innerView.frame;
    innerContentFrame.origin.x = 0;
    innerContentFrame.origin.y = 0;
    innerContentFrame.size.width = contentFrame.size.width;
    innerContentFrame.size.height = contentFrame.size.height;
    
    // Set frames
    [contentView setFrame: contentFrame];
    [innerView setFrame: innerContentFrame];
    [tabBarView setFrame: tabFrame];
    [stateBarView setFrame: stateBarFrame];
    
    // Commit animation
    if(tabBar != nil || fullscreen != nil) {
        [UIView commitAnimations];
    }
    
    // Change view
    if(description != nil) {
        [UICompositeViewController addSubView: contentViewController view:contentView];
        if(oldViewDescription == nil || oldViewDescription.tabBar != currentViewDescription.tabBar) {
            [UICompositeViewController addSubView: tabBarViewController view:tabBarView];
        }
        if(oldViewDescription == nil || oldViewDescription.stateBar != currentViewDescription.stateBar) {
            [UICompositeViewController addSubView: stateBarViewController view:stateBarView];
        }
    }
    
    // Dealloc old view description
    if(oldViewDescription != nil) {
        [oldViewDescription release];
    }
}

- (void) changeView:(UICompositeViewDescription *)description {
    [self view]; // Force view load
    [self update:description tabBar:nil fullscreen:nil];
}

- (void) setFullScreen:(BOOL) enabled {
    [self update:nil tabBar:nil fullscreen:[NSNumber numberWithBool:enabled]];
}

- (void) setToolBarHidden:(BOOL) hidden {
    [self update:nil tabBar:[NSNumber numberWithBool:!hidden] fullscreen:nil];
}

- (UIViewController *) getCurrentViewController {
    return contentViewController;
}

@end
