//
//  TPMultiLayoutViewController.h
//
//  Created by Michael Tyson on 14/08/2011.
//  Copyright 2011 A Tasty Pixel. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TPMultiLayoutViewController : UIViewController {
    UIView *portraitView;
    UIView *landscapeView;
    
    @private
    NSDictionary *portraitAttributes;
    NSDictionary *landscapeAttributes;
    BOOL viewIsCurrentlyPortrait;
}

// Call directly to use with custom animation (override willRotateToInterfaceOrientation to disable the switch there)
- (void)applyLayoutForInterfaceOrientation:(UIInterfaceOrientation)newOrientation;

@property(nonatomic, strong) IBOutlet UIView *landscapeView;
@property(nonatomic, strong) IBOutlet UIView *portraitView;
@property (assign) BOOL viewIsCurrentlyPortrait;

@end
