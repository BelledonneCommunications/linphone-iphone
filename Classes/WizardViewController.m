/* WizardViewController.m
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

#import "WizardViewController.h"

#import <QuartzCore/QuartzCore.h>

#import "LinphoneManager.h"

#import "PhoneMainView.h"

typedef enum _ViewElement {
    ViewElement_Username = 100,
    ViewElement_Password = 101,
    ViewElement_Password2 = 102,
    ViewElement_Email = 103,
    ViewElement_Domain = 104,
    ViewElement_Label = 200,
    ViewElement_Error = 201
} ViewElement;

@implementation WizardViewController

@synthesize contentView;

@synthesize welcomeView;
@synthesize choiceView;
@synthesize createAccountView;
@synthesize connectAccountView;
@synthesize externalAccountView;

@synthesize backButton;
@synthesize startButton;

#pragma mark - Lifecycle Functions

- (id)init {
    self = [super initWithNibName:@"WizardViewController" bundle:[NSBundle mainBundle]];
    if (self != nil) {
        self->historyViews = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)dealloc {
    [contentView release];
    
    [welcomeView release];
    [choiceView release];
    [createAccountView release];
    [connectAccountView release];
    [externalAccountView release];
    
    [backButton release];
    [startButton release];
    
    [historyViews release];
    
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"Wizard" 
                                                                content:@"WizardViewController" 
                                                               stateBar:nil 
                                                        stateBarEnabled:false 
                                                                 tabBar:nil 
                                                          tabBarEnabled:false 
                                                             fullscreen:false
                                                          landscapeMode:false
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    [self changeView:welcomeView back:FALSE animation:FALSE];
}


#pragma mark - 

+ (UIView*)findTextField:(ViewElement)tag view:(UIView*)view {
    for(UIView *child in [view subviews]) {
        if([child tag] == tag){
            return (UITextField*)child;
        } else {
            UIView *o = [WizardViewController findTextField:tag view:child];
            if(o)
                return o;
        }
    }
    return nil;
}

- (UITextField*)findTextField:(ViewElement)tag {
    UIView *view = [WizardViewController findTextField:tag view:contentView];
    if([view isKindOfClass:[UITextField class]])
        return (UITextField*)view;
    return nil;
}

- (UILabel*)findLabel:(ViewElement)tag {
    UIView *view = [WizardViewController findTextField:tag view:contentView];
    if([view isKindOfClass:[UILabel class]])
        return (UILabel*)view;
    return nil;
}

- (void)changeView:(UIView *)view back:(BOOL)back animation:(BOOL)animation {
    if (view == welcomeView) {
        [startButton setHidden:false];
        [backButton setHidden:true];
    } else {
        [startButton setHidden:true];
        [backButton setHidden:false];
    }
    
    if(animation) {
      CATransition* trans = [CATransition animation];
      [trans setType:kCATransitionPush];
      [trans setDuration:0.35];
      [trans setTimingFunction:[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut]];
      if(back) {
          [trans setSubtype:kCATransitionFromLeft];
      }else {
          [trans setSubtype:kCATransitionFromRight];
      }
      [contentView.layer addAnimation:trans forKey:@"Transition"];
    }
    
    NSArray *childs = [contentView subviews];
    if([childs count]> 0) {
        UIView *childView = [childs objectAtIndex:0];
        if(!back)
            [historyViews addObject:childView];
        [childView removeFromSuperview];
    }
    
    [contentView addSubview:view];
    
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return YES;
}


#pragma mark - Action Functions

- (IBAction)onStartClick:(id)sender {
    [self changeView:choiceView back:FALSE animation:TRUE];
}

- (IBAction)onBackClick:(id)sender {
    if ([historyViews count] > 0) {
        UIView * view = [historyViews lastObject];
        [historyViews removeLastObject];
        [self changeView:view back:TRUE animation:TRUE];
    }
}

- (IBAction)onCancelClick:(id)sender {
    [[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
}

- (IBAction)onCreateAccountClick:(id)sender {
    [self changeView:createAccountView back:FALSE animation:TRUE];
}

- (IBAction)onConnectAccountClick:(id)sender {
    [self changeView:connectAccountView back:FALSE animation:TRUE];
}

- (IBAction)onExternalAccountClick:(id)sender {
    [self changeView:externalAccountView back:FALSE animation:TRUE];
}

@end
