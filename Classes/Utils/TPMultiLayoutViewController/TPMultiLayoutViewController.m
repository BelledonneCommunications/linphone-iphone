//
//  TPMultiLayoutViewController.m
//
//  Created by Michael Tyson on 14/08/2011.
//  Copyright 2011 A Tasty Pixel. All rights reserved.
//

#import "TPMultiLayoutViewController.h"

#define VERBOSE_MATCH_FAIL 1 // Comment this out to be less verbose when associated views can't be found

@interface TPMultiLayoutViewController ()
- (NSDictionary*)attributeTableForViewHierarchy:(UIView*)rootView associateWithViewHierarchy:(UIView*)associatedRootView;
- (void)addAttributesForSubviewHierarchy:(UIView*)view associatedWithSubviewHierarchy:(UIView*)associatedView toTable:(NSMutableDictionary*)table;
- (UIView*)findAssociatedViewForView:(UIView*)view amongViews:(NSArray*)views;
- (void)applyAttributeTable:(NSDictionary*)table toViewHierarchy:(UIView*)view;
- (NSDictionary*)attributesForView:(UIView*)view;
- (void)applyAttributes:(NSDictionary*)attributes toView:(UIView*)view;
- (BOOL)shouldDescendIntoSubviewsOfView:(UIView*)view;
@end

@implementation TPMultiLayoutViewController
@synthesize portraitView, landscapeView, viewIsCurrentlyPortrait;

#pragma mark - View lifecycle

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Construct attribute tables
	portraitAttributes = [self attributeTableForViewHierarchy:portraitView associateWithViewHierarchy:self.view];
	landscapeAttributes = [self attributeTableForViewHierarchy:landscapeView associateWithViewHierarchy:self.view];
	viewIsCurrentlyPortrait = (self.view == portraitView);

	// Don't need to retain the original template view hierarchies any more
	self.portraitView = nil;
    self.landscapeView = nil;
}

- (void)dealloc {
    portraitAttributes = nil;
    landscapeAttributes = nil;
    
}

- (void)viewWillAppear:(BOOL)animated {
    // Display correct layout for orientation
    /*if ( (UIInterfaceOrientationIsPortrait(self.interfaceOrientation) && !viewIsCurrentlyPortrait) ||
         (UIInterfaceOrientationIsLandscape(self.interfaceOrientation) && viewIsCurrentlyPortrait) ) {
        [self applyLayoutForInterfaceOrientation:self.interfaceOrientation];
    }*/
    [super viewWillAppear:animated];
}

#pragma mark - Rotation

- (void)applyLayoutForInterfaceOrientation:(UIInterfaceOrientation)newOrientation {
    NSDictionary *table = UIInterfaceOrientationIsPortrait(newOrientation) ? portraitAttributes : landscapeAttributes;
    [self applyAttributeTable:table toViewHierarchy:self.view];
    viewIsCurrentlyPortrait = UIInterfaceOrientationIsPortrait(newOrientation);
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];
    //if ( (UIInterfaceOrientationIsPortrait(toInterfaceOrientation) && !viewIsCurrentlyPortrait) ||
    //     (UIInterfaceOrientationIsLandscape(toInterfaceOrientation) && viewIsCurrentlyPortrait) ) {
        [self applyLayoutForInterfaceOrientation:toInterfaceOrientation];
    //}
}

#pragma mark - Helpers

- (NSDictionary*)attributeTableForViewHierarchy:(UIView*)rootView associateWithViewHierarchy:(UIView*)associatedRootView {
    NSMutableDictionary *table = [NSMutableDictionary dictionary];
    [self addAttributesForSubviewHierarchy:rootView associatedWithSubviewHierarchy:associatedRootView toTable:table];        
    return table;
}

- (void)addAttributesForSubviewHierarchy:(UIView*)view associatedWithSubviewHierarchy:(UIView*)associatedView toTable:(NSMutableDictionary*)table {
	[table setObject:[self attributesForView:view]
			  forKey:[NSValue valueWithPointer:(__bridge const void *)(associatedView)]];

	if (![self shouldDescendIntoSubviewsOfView:view])
		return;

	for (UIView *subview in view.subviews) {
		UIView *associatedSubView = (view == associatedView ? subview : [self findAssociatedViewForView:subview amongViews:associatedView.subviews]);
        if ( associatedSubView ) {
            [self addAttributesForSubviewHierarchy:subview associatedWithSubviewHierarchy:associatedSubView toTable:table];
        }
    }
}

- (UIView*)findAssociatedViewForView:(UIView*)view amongViews:(NSArray*)views {
    // First try to match tag
    if ( view.tag != 0 ) {
        UIView *associatedView = [[views filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"tag = %d", view.tag]] lastObject];
        if ( associatedView ) return associatedView;
    }
    
    // Next, try to match class, targets and actions, if it's a control
    if ( [view isKindOfClass:[UIControl class]] && [[(UIControl*)view allTargets] count] > 0 ) {
        for ( UIView *otherView in views ) {
            if ( [otherView isKindOfClass:[view class]]
                    && [[(UIControl*)otherView allTargets] isEqualToSet:[(UIControl*)view allTargets]] 
                    && [(UIControl*)otherView allControlEvents] == [(UIControl*)view allControlEvents] ) {
                // Try to match all actions and targets for each associated control event
                BOOL allActionsMatch = YES;
                UIControlEvents controlEvents = [(UIControl*)otherView allControlEvents];
                for ( id target in [(UIControl*)otherView allTargets] ) {
                    // Iterate over each bit in the UIControlEvents bitfield
                    for ( NSInteger i=0; i<sizeof(UIControlEvents)*8; i++ ) {
                        UIControlEvents event = 1 << i;
                        if ( !(controlEvents & event) ) continue;
                        if ( ![[(UIControl*)otherView actionsForTarget:target forControlEvent:event] isEqualToArray:[(UIControl*)view actionsForTarget:target forControlEvent:event]] ) {
                            allActionsMatch = NO;
                            break;
                        }
                    }
                    if ( !allActionsMatch ) break;
                }
                
                if ( allActionsMatch ) {
                    return otherView;
                }
            }
        }
    }
    
    // Next, try to match title or image, if it's a button
    if ( [view isKindOfClass:[UIButton class]] ) {
        for ( UIView *otherView in views ) {
            if ( [otherView isKindOfClass:[view class]] && [[(UIButton*)otherView titleForState:UIControlStateNormal] isEqualToString:[(UIButton*)view titleForState:UIControlStateNormal]] ) {
                return otherView;
            }
        }

        for ( UIView *otherView in views ) {
            if ( [otherView isKindOfClass:[view class]] && [(UIButton*)otherView imageForState:UIControlStateNormal] == [(UIButton*)view imageForState:UIControlStateNormal] ) {
                return otherView;
            }
        }
    }
    
    // Try to match by title if it's a label
    if ( [view isKindOfClass:[UILabel class]] ) {
        for ( UIView *otherView in views ) {
            if ( [otherView isKindOfClass:[view class]] && [[(UILabel*)otherView text] isEqualToString:[(UILabel*)view text]] ) {
                return otherView;
            }
        }
    }
    
    // Try to match by text/placeholder if it's a text field
    if ( [view isKindOfClass:[UITextField class]] ) {
        for ( UIView *otherView in views ) {
            if ( [otherView isKindOfClass:[view class]] && ([(UITextField*)view text] || [(UITextField*)view placeholder]) &&
                    ((![(UITextField*)view text] && ![(UITextField*)otherView text]) || [[(UITextField*)otherView text] isEqualToString:[(UITextField*)view text]]) &&
                    ((![(UITextField*)view placeholder] && ![(UITextField*)otherView placeholder]) || [[(UITextField*)otherView placeholder] isEqualToString:[(UITextField*)view placeholder]]) ) {                
                return otherView;
            }
        }
    }
    
    // Finally, try to match by class
    NSArray *matches = [views filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"class = %@", [view class]]];
    if ( [matches count] == 1 ) return [matches lastObject];
    
#if VERBOSE_MATCH_FAIL
    NSMutableString *path = [NSMutableString string];
    for ( UIView *v = view.superview; v != nil; v = v.superview ) {
        [path insertString:[NSString stringWithFormat:@"%@ => ", NSStringFromClass([v class])] atIndex:0];
    }
    NSLog(@"Couldn't find match for %@%@", path, NSStringFromClass([view class]));
    
#endif
    
    return nil;
}

- (void)applyAttributeTable:(NSDictionary*)table toViewHierarchy:(UIView*)view {
	NSDictionary *attributes = [table objectForKey:[NSValue valueWithPointer:(__bridge const void *)(view)]];
	if (attributes) {
		[self applyAttributes:attributes toView:view];
	}
    
    //if ( view.hidden ) return;
    
    if ( ![self shouldDescendIntoSubviewsOfView:view] ) return;
    
    for ( UIView *subview in view.subviews ) {
        [self applyAttributeTable:table toViewHierarchy:subview];
    }
}

- (NSDictionary*)attributesForView:(UIView*)view {
    NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
    
    [attributes setObject:[NSValue valueWithCGRect:view.frame] forKey:@"frame"];
    [attributes setObject:[NSValue valueWithCGRect:view.bounds] forKey:@"bounds"];
    [attributes setObject:[NSNumber numberWithBool:view.hidden] forKey:@"hidden"];
    [attributes setObject:[NSNumber numberWithInteger:view.autoresizingMask] forKey:@"autoresizingMask"];
    
    return attributes;
}

- (void)applyAttributes:(NSDictionary*)attributes toView:(UIView*)view {
    view.frame = [[attributes objectForKey:@"frame"] CGRectValue];
    view.bounds = [[attributes objectForKey:@"bounds"] CGRectValue];
    view.hidden = [[attributes objectForKey:@"hidden"] boolValue];
    view.autoresizingMask = [[attributes objectForKey:@"autoresizingMask"] integerValue];
}

- (BOOL)shouldDescendIntoSubviewsOfView:(UIView*)view {
    if ( [view isKindOfClass:[UISlider class]] ||
         [view isKindOfClass:[UISwitch class]] ||
         [view isKindOfClass:[UITextField class]] ||
         [view isKindOfClass:[UIWebView class]] ||
         [view isKindOfClass:[UITableView class]] ||
         [view isKindOfClass:[UIPickerView class]] ||
         [view isKindOfClass:[UIDatePicker class]] ||
         [view isKindOfClass:[UITextView class]] ||
         [view isKindOfClass:[UIProgressView class]] ||
         [view isKindOfClass:[UISegmentedControl class]] ) return NO;
    return YES;
}

@end