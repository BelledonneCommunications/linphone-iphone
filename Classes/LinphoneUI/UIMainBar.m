/* UIMainBar.m
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

#import "UIMainBar.h"
#import "PhoneMainView.h"
#import "ChatModel.h"
#import "CAAnimation+Blocks.h"

@implementation UIMainBar


static NSString * const kBounceAnimation = @"bounce";
static NSString * const kAppearAnimation = @"appear";
static NSString * const kDisappearAnimation = @"disappear";

@synthesize historyButton;
@synthesize contactsButton;
@synthesize dialerButton;
@synthesize settingsButton;
@synthesize chatButton;
@synthesize historyNotificationView;
@synthesize historyNotificationLabel;
@synthesize chatNotificationView;
@synthesize chatNotificationLabel;

#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"UIMainBar" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [historyButton release];
    [contactsButton release];
    [dialerButton release];
    [settingsButton release];
    [chatButton release];
    [historyNotificationView release];
    [historyNotificationLabel release];
    [chatNotificationView release];
    [chatNotificationLabel release];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(changeViewEvent:) 
                                                 name:kLinphoneMainViewChange
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceived:) 
                                                 name:kLinphoneTextReceived
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(settingsUpdate:)
                                                 name:kLinphoneSettingsUpdate
                                               object:nil];
    [self update:FALSE];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:kLinphoneMainViewChange
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:kLinphoneCallUpdate
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:kLinphoneTextReceived
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                 name:kLinphoneSettingsUpdate
                                               object:nil];
}

- (void)viewDidLoad {
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(applicationWillEnterForeground:) 
                                                 name:UIApplicationWillEnterForegroundNotification 
                                               object:nil];
    
    {
        UIButton *historyButtonLandscape = (UIButton*) [landscapeView viewWithTag:[historyButton tag]];
        // Set selected+over background: IB lack !
        [historyButton setBackgroundImage:[UIImage imageNamed:@"history_selected.png"]
                                 forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [historyButtonLandscape setBackgroundImage:[UIImage imageNamed:@"history_selected_landscape.png"]
                                           forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStatesForTabs:historyButton];
        [LinphoneUtils buttonFixStatesForTabs:historyButtonLandscape];
    }
    
    {
        UIButton *contactsButtonLandscape = (UIButton*) [landscapeView viewWithTag:[contactsButton tag]];
        // Set selected+over background: IB lack !
        [contactsButton setBackgroundImage:[UIImage imageNamed:@"contacts_selected.png"]
                                  forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [contactsButtonLandscape setBackgroundImage:[UIImage imageNamed:@"contacts_selected_landscape.png"]
                                         forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStatesForTabs:contactsButton];
        [LinphoneUtils buttonFixStatesForTabs:contactsButtonLandscape];
    }
    {
        UIButton *dialerButtonLandscape = (UIButton*) [landscapeView viewWithTag:[dialerButton tag]];
        // Set selected+over background: IB lack !
        [dialerButton setBackgroundImage:[UIImage imageNamed:@"dialer_selected.png"]
                                forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [dialerButtonLandscape setBackgroundImage:[UIImage imageNamed:@"dialer_selected_landscape.png"]
                                           forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStatesForTabs:dialerButton];
        [LinphoneUtils buttonFixStatesForTabs:dialerButtonLandscape];
    }
    {
        UIButton *settingsButtonLandscape = (UIButton*) [landscapeView viewWithTag:[settingsButton tag]];
        // Set selected+over background: IB lack !
        [settingsButton setBackgroundImage:[UIImage imageNamed:@"settings_selected.png"]
                                  forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [settingsButtonLandscape setBackgroundImage:[UIImage imageNamed:@"settings_selected_landscape.png"]
                                       forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStatesForTabs:settingsButton];
        [LinphoneUtils buttonFixStatesForTabs:settingsButtonLandscape];
    }
    
    {
        UIButton *chatButtonLandscape = (UIButton*) [landscapeView viewWithTag:[chatButton tag]];
        // Set selected+over background: IB lack !
        [chatButton setBackgroundImage:[UIImage imageNamed:@"chat_selected.png"]
                              forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [chatButtonLandscape setBackgroundImage:[UIImage imageNamed:@"chat_selected_landscape.png"]
                              forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStatesForTabs:chatButton];
        [LinphoneUtils buttonFixStatesForTabs:chatButtonLandscape];
    }
    
    [super viewDidLoad]; // Have to be after due to TPMultiLayoutViewController
}

- (void)viewDidUnload {
    [super viewDidUnload];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:UIApplicationWillEnterForegroundNotification 
                                                  object:nil];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    // Force the animations
    [[self.view layer] removeAllAnimations];
    [historyNotificationView.layer setTransform:CATransform3DIdentity];
    [chatNotificationView.layer setTransform:CATransform3DIdentity];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    [chatNotificationView setHidden:TRUE];
    [historyNotificationView setHidden:TRUE];
    [self update:FALSE];
}


#pragma mark - Event Functions

- (void)applicationWillEnterForeground:(NSNotification*)notif { 
    // Force the animations 
    [[self.view layer] removeAllAnimations];
    [historyNotificationView.layer setTransform:CATransform3DIdentity];
    [chatNotificationView.layer setTransform:CATransform3DIdentity];
    [chatNotificationView setHidden:TRUE];
    [historyNotificationView setHidden:TRUE];
    [self update:FALSE];
}

- (void)callUpdate:(NSNotification*)notif {
    //LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    //LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc]) appear:TRUE];
}

- (void)changeViewEvent:(NSNotification*)notif {  
    //UICompositeViewDescription *view = [notif.userInfo objectForKey: @"view"];
    //if(view != nil)
    [self updateView:[[PhoneMainView instance] firstView]];
}

- (void)settingsUpdate:(NSNotification*)notif {
    if([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == false) {
        [self stopBounceAnimation:kBounceAnimation target:chatNotificationView];
        chatNotificationView.layer.transform = CATransform3DIdentity;
        [self stopBounceAnimation:kBounceAnimation target:historyNotificationView];
        historyNotificationView.layer.transform = CATransform3DIdentity;
    } else {
        if(![chatNotificationView isHidden] && [chatNotificationView.layer animationForKey:kBounceAnimation] == nil) {
            [self startBounceAnimation:kBounceAnimation target:chatNotificationView];
        }
        if(![historyNotificationView isHidden] && [historyNotificationView.layer animationForKey:kBounceAnimation] == nil) {
            [self startBounceAnimation:kBounceAnimation target:historyNotificationView];
        }
    }
}

- (void)textReceived:(NSNotification*)notif {
    [self updateUnreadMessage:[ChatModel unreadMessages] appear:TRUE];
}


#pragma mark - 

- (void)update:(BOOL)appear{
    [self updateView:[[PhoneMainView instance] firstView]];
    if([LinphoneManager isLcReady]) {
        [self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc]) appear:appear];
    } else {
        [self updateMissedCall:0 appear:TRUE];
    }
    [self updateUnreadMessage:[ChatModel unreadMessages] appear:appear];
}

- (void)updateUnreadMessage:(int)unreadMessage appear:(BOOL)appear{
    if (unreadMessage > 0) {
        if([chatNotificationView isHidden]) {
            [chatNotificationView setHidden:FALSE];
            if([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
                if(appear) {
                    [self appearAnimation:kAppearAnimation target:chatNotificationView completion:^(BOOL finished){
                        [self startBounceAnimation:kBounceAnimation target:chatNotificationView];
                        [chatNotificationView.layer removeAnimationForKey:kAppearAnimation];
                    }];
                } else {
                    [self startBounceAnimation:kBounceAnimation target:chatNotificationView];
                }
            }
        }
        [chatNotificationLabel setText:[NSString stringWithFormat:@"%i", unreadMessage]];
    } else {
        if(![chatNotificationView isHidden]) {
            [self stopBounceAnimation:kBounceAnimation target:chatNotificationView];
            if(appear) {
                [self disappearAnimation:kDisappearAnimation target:chatNotificationView completion:^(BOOL finished){
                    [chatNotificationView setHidden:TRUE];
                    [chatNotificationView.layer removeAnimationForKey:kDisappearAnimation];
                }];
            } else {
                [chatNotificationView setHidden:TRUE];
            }
        }
    }
}

- (void)updateMissedCall:(int)missedCall appear:(BOOL)appear{
    if (missedCall > 0) {
        if([historyNotificationView isHidden]) {
            [historyNotificationView setHidden:FALSE];
            if([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
                if(appear) {
                    [self appearAnimation:kAppearAnimation target:historyNotificationView completion:^(BOOL finished){
                        [self startBounceAnimation:kBounceAnimation target:historyNotificationView];
                        [historyNotificationView.layer removeAnimationForKey:kAppearAnimation];
                    }];
                } else {
                    [self startBounceAnimation:kBounceAnimation target:historyNotificationView];
                }
            }
        }
        [historyNotificationLabel setText:[NSString stringWithFormat:@"%i", missedCall]];
    } else {
        if(![historyNotificationView isHidden]) {
            [self stopBounceAnimation:kBounceAnimation target:historyNotificationView];
            if(appear) {
                [self disappearAnimation:kDisappearAnimation target:historyNotificationView completion:^(BOOL finished){
                    [historyNotificationView setHidden:TRUE];
                    [historyNotificationView.layer removeAnimationForKey:kDisappearAnimation];
                }];
            } else {
                [historyNotificationView setHidden:TRUE];
            }
        }
    }
}

- (void)appearAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    CABasicAnimation *appear = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    appear.duration = 0.4;
    appear.fromValue = [NSNumber numberWithDouble:0.0f];
    appear.toValue = [NSNumber numberWithDouble:1.0f];
    appear.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
    appear.fillMode = kCAFillModeForwards;
    appear.removedOnCompletion = NO;
    [appear setCompletion:completion];
    [target.layer addAnimation:appear forKey:animationID];
}

- (void)disappearAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    CABasicAnimation *disappear = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
    disappear.duration = 0.4;
    disappear.fromValue = [NSNumber numberWithDouble:1.0f];
    disappear.toValue = [NSNumber numberWithDouble:0.0f];
    disappear.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
    disappear.fillMode = kCAFillModeForwards;
    disappear.removedOnCompletion = NO;
    [disappear setCompletion:completion];
    [target.layer addAnimation:disappear forKey:animationID];
}

- (void)startBounceAnimation:(NSString *)animationID target:(UIView *)target {
    CABasicAnimation *bounce = [CABasicAnimation animationWithKeyPath:@"transform.translation.y"];
    bounce.duration = 0.3;
    bounce.fromValue = [NSNumber numberWithDouble:0.0f];
    bounce.toValue = [NSNumber numberWithDouble:8.0f];
    bounce.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
    bounce.autoreverses = TRUE;
    bounce.repeatCount = HUGE_VALF;
    [target.layer addAnimation:bounce forKey:animationID];
    
}

- (void)stopBounceAnimation:(NSString *)animationID target:(UIView *)target {
    [target.layer removeAnimationForKey:animationID];
}
         
- (void)updateView:(UICompositeViewDescription*) view {  
    // Update buttons
    if([view equal:[HistoryViewController compositeViewDescription]]) {
        historyButton.selected = TRUE;
    } else {
        historyButton.selected = FALSE;
    }
    if([view equal:[ContactsViewController compositeViewDescription]]) {
        contactsButton.selected = TRUE;
    } else {
        contactsButton.selected = FALSE;
    }
    if([view equal:[DialerViewController compositeViewDescription]]) {
        dialerButton.selected = TRUE;
    } else {
        dialerButton.selected = FALSE;
    }
    if([view equal:[SettingsViewController compositeViewDescription]]) {
        settingsButton.selected = TRUE;
    } else {
        settingsButton.selected = FALSE;
    }
    if([view equal:[ChatViewController compositeViewDescription]]) {
        chatButton.selected = TRUE;
    } else {
        chatButton.selected = FALSE;
    }
}


#pragma mark - Action Functions

- (IBAction)onHistoryClick:(id)event {
    [[PhoneMainView instance] changeCurrentView:[HistoryViewController compositeViewDescription]];
}

- (IBAction)onContactsClick:(id)event {
    [ContactSelection setSelectionMode:ContactSelectionModeNone];
    [ContactSelection setAddAddress:nil];
    [ContactSelection setSipFilter:FALSE];
    [ContactSelection setEmailFilter:FALSE];
    [[PhoneMainView instance] changeCurrentView:[ContactsViewController compositeViewDescription]];
}

- (IBAction)onDialerClick:(id)event {
    [[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
}

- (IBAction)onSettingsClick:(id)event {
    [[PhoneMainView instance] changeCurrentView:[SettingsViewController compositeViewDescription]];
}

- (IBAction)onChatClick:(id)event {
    [[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
}


#pragma mark - TPMultiLayoutViewController Functions

- (NSDictionary*)attributesForView:(UIView*)view {
    NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
    
    [attributes setObject:[NSValue valueWithCGRect:view.frame] forKey:@"frame"];
    [attributes setObject:[NSValue valueWithCGRect:view.bounds] forKey:@"bounds"];
    if([view isKindOfClass:[UIButton class]]) {
        UIButton *button = (UIButton *)view;
        [LinphoneUtils buttonMultiViewAddAttributes:attributes button:button];
    }
    [attributes setObject:[NSNumber numberWithInteger:view.autoresizingMask] forKey:@"autoresizingMask"];
    
    return attributes;
}

- (void)applyAttributes:(NSDictionary*)attributes toView:(UIView*)view {
    view.frame = [[attributes objectForKey:@"frame"] CGRectValue];
    view.bounds = [[attributes objectForKey:@"bounds"] CGRectValue];
    if([view isKindOfClass:[UIButton class]]) {
        UIButton *button = (UIButton *)view;
        [LinphoneUtils buttonMultiViewApplyAttributes:attributes button:button];
    }
    view.autoresizingMask = [[attributes objectForKey:@"autoresizingMask"] integerValue];
}

@end
