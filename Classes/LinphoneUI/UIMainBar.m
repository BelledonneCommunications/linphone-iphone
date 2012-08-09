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

@implementation UIMainBar

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
                                                 name:@"LinphoneMainViewChange" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdate:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceived:) 
                                                 name:@"LinphoneTextReceived" 
                                               object:nil];
    [self update:FALSE];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneMainViewChange" 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneCallUpdate" 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneTextReceived" 
                                                  object:nil];
}

- (void)viewDidLoad {
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(applicationWillEnterForeground:) 
                                                 name:UIApplicationWillEnterForegroundNotification 
                                               object:nil];
    
    // Set selected+over background: IB lack !
    [historyButton setImage:[UIImage imageNamed:@"history_selected.png"]
                    forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    // Set selected+over background: IB lack !
    [contactsButton setImage:[UIImage imageNamed:@"contacts_selected.png"]
                   forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    // Set selected+over background: IB lack !
    [dialerButton setImage:[UIImage imageNamed:@"dialer_selected.png"]
                   forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    // Set selected+over background: IB lack !
    [settingsButton setImage:[UIImage imageNamed:@"settings_selected.png"]
                   forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    // Set selected+over background: IB lack !
    [chatButton setImage:[UIImage imageNamed:@"chat_selected.png"]
                   forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
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
            if(appear) {
                [self appearAnimation:@"Appear" target:chatNotificationView completion:^(BOOL finished){
                    [self startBounceAnimation:@"Bounce" target:chatNotificationView];
                }];
            } else {
                [self startBounceAnimation:@"Bounce" target:chatNotificationView];
            }
        }
        [chatNotificationLabel setText:[NSString stringWithFormat:@"%i", unreadMessage]];
    } else {
        if(![chatNotificationView isHidden]) {
            [self stopBounceAnimation:@"Bounce" target:chatNotificationView];
            if(appear) {
                [self disappearAnimation:@"Disappear" target:chatNotificationView completion:^(BOOL finished){
                    [chatNotificationView setHidden:TRUE];
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
            if(appear) {
                [self appearAnimation:@"Appear" target:historyNotificationView completion:^(BOOL finished){
                    [self startBounceAnimation:@"Bounce" target:historyNotificationView];
                }];
            } else {
                [self startBounceAnimation:@"Bounce" target:historyNotificationView];
            }
        }
        [historyNotificationLabel setText:[NSString stringWithFormat:@"%i", missedCall]];
    } else {
        if(![historyNotificationView isHidden]) {
            [self stopBounceAnimation:@"Bounce" target:historyNotificationView];
            if(appear) {
                [self disappearAnimation:@"Disappear" target:historyNotificationView completion:^(BOOL finished){
                    
                }];
            } else {
                [historyNotificationView setHidden:TRUE];
            }
        }
    }
}

- (void)appearAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    target.layer.transform = CATransform3DMakeScale(0.01f, 0.01f, 1.0f);
    [UIView animateWithDuration:0.4 
                          delay:0 
                        options:UIViewAnimationOptionCurveEaseOut | UIViewAnimationOptionAllowUserInteraction
                     animations:^{
                         target.layer.transform = CATransform3DIdentity;
                     }
                     completion:completion];
}

- (void)disappearAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    CATransform3D startCGA = target.layer.transform;
    [UIView animateWithDuration:0.4 
                          delay:0 
                        options:UIViewAnimationOptionCurveEaseOut | UIViewAnimationOptionAllowUserInteraction
                     animations:^{
                         target.layer.transform = CATransform3DConcat(startCGA, CATransform3DMakeScale(0.01f, 0.01f, 1.0f));
                     }
                     completion:completion];
}

- (void)startBounceAnimation:(NSString *)animationID target:(UIView *)target { 
    CATransform3D startCGA = target.layer.transform;
    [UIView animateWithDuration: 0.3
                          delay: 0
                        options: UIViewAnimationOptionRepeat | 
     UIViewAnimationOptionAutoreverse |
     UIViewAnimationOptionAllowUserInteraction | 
     UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         target.layer.transform = CATransform3DConcat(startCGA, CATransform3DMakeTranslation(0, 8, 0));
                     }
                     completion:^(BOOL finished){
                     }];
    
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

- (IBAction)onHistoryClick: (id) sender {
    [[PhoneMainView instance] changeCurrentView:[HistoryViewController compositeViewDescription]];
}

- (IBAction)onContactsClick: (id) event {
    [ContactSelection setSelectionMode:ContactSelectionModeNone];
    [ContactSelection setAddAddress:nil];
    [ContactSelection setSipFilter:FALSE];
    [[PhoneMainView instance] changeCurrentView:[ContactsViewController compositeViewDescription]];
}

- (IBAction)onDialerClick: (id) event {
    [[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
}

- (IBAction)onSettingsClick: (id) event {
    [[PhoneMainView instance] changeCurrentView:[SettingsViewController compositeViewDescription]];
}

- (IBAction)onChatClick: (id) event {
    [[PhoneMainView instance] changeCurrentView:[ChatViewController compositeViewDescription]];
}

#pragma mark - TPMultiLayoutViewController Functions

- (NSDictionary*)attributesForView:(UIView*)view {
    NSMutableDictionary *attributes = [NSMutableDictionary dictionary];
    
    [attributes setObject:[NSValue valueWithCGRect:view.frame] forKey:@"frame"];
    [attributes setObject:[NSValue valueWithCGRect:view.bounds] forKey:@"bounds"];
    if([view isKindOfClass:[UIButton class]]) {
        UIButton *button = (UIButton *)view;
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateNormal] key:@"image-normal"];
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateHighlighted] key:@"image-highlighted"];
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateDisabled] key:@"image-disabled"];
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateSelected] key:@"image-selected"];
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateDisabled | UIControlStateHighlighted] key:@"image-disabled-highlighted"];
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateSelected | UIControlStateHighlighted] key:@"image-selected-highlighted"];
        [UIMainBar addDictEntry:attributes item:[button imageForState:UIControlStateSelected | UIControlStateDisabled] key:@"image-selected-disabled"];
        
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateNormal] key:@"background-normal"];
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateHighlighted] key:@"background-highlighted"];
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateDisabled] key:@"background-disabled"];
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateSelected] key:@"background-selected"];
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateDisabled | UIControlStateHighlighted] key:@"background-disabled-highlighted"];
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateSelected | UIControlStateHighlighted] key:@"background-selected-highlighted"];
        [UIMainBar addDictEntry:attributes item:[button backgroundImageForState:UIControlStateSelected | UIControlStateDisabled] key:@"background-selected-disabled"];
    }
    [attributes setObject:[NSNumber numberWithInteger:view.autoresizingMask] forKey:@"autoresizingMask"];
    
    return attributes;
}

- (void)applyAttributes:(NSDictionary*)attributes toView:(UIView*)view {
    view.frame = [[attributes objectForKey:@"frame"] CGRectValue];
    view.bounds = [[attributes objectForKey:@"bounds"] CGRectValue];
    if([view isKindOfClass:[UIButton class]]) {
        UIButton *button = (UIButton *)view;
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-normal"] forState:UIControlStateNormal];
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-highlighted"] forState:UIControlStateHighlighted];
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-disabled"] forState:UIControlStateDisabled];
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-selected"] forState:UIControlStateSelected];
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-disabled-highlighted"] forState:UIControlStateDisabled | UIControlStateHighlighted];
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-selected-highlighted"] forState:UIControlStateSelected | UIControlStateHighlighted];
        [button setImage:[UIMainBar getDictEntry:attributes key:@"image-selected-disabled"] forState:UIControlStateSelected | UIControlStateDisabled];
        
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-normal"] forState:UIControlStateNormal];
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-highlighted"] forState:UIControlStateHighlighted];
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-disabled"] forState:UIControlStateDisabled];
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-selected"] forState:UIControlStateSelected];
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-disabled-highlighted"] forState:UIControlStateDisabled | UIControlStateHighlighted];
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-selected-highlighted"] forState:UIControlStateSelected | UIControlStateHighlighted];
        [button setBackgroundImage:[UIMainBar getDictEntry:attributes key:@"background-selected-disabled"] forState:UIControlStateSelected | UIControlStateDisabled];
    }
    view.autoresizingMask = [[attributes objectForKey:@"autoresizingMask"] integerValue];
}

+ (void)addDictEntry:(NSMutableDictionary*)dict item:(id)item key:(id)key {
    if(item != nil && key != nil) {
        [dict setObject:item forKey:key];
    }
}

+ (id)getDictEntry:(NSDictionary*)dict key:(id)key {
    if(key != nil) {
        return [dict objectForKey:key];
    }
    return nil;
}

@end
