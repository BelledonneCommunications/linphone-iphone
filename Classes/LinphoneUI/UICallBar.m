/* UICallBar.m
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

#import "UICallBar.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils.h"
#import "CAAnimation+Blocks.h"

#include "linphonecore.h"

@implementation UICallBar

@synthesize pauseButton;
@synthesize conferenceButton;
@synthesize videoButton;
@synthesize microButton;
@synthesize speakerButton;
@synthesize routesButton;
@synthesize optionsButton;
@synthesize hangupButton;
@synthesize routesBluetoothButton;
@synthesize routesReceiverButton;
@synthesize routesSpeakerButton;
@synthesize optionsAddButton;
@synthesize optionsTransferButton;
@synthesize dialerButton;

@synthesize padView;
@synthesize routesView;
@synthesize optionsView;

@synthesize oneButton;
@synthesize twoButton;
@synthesize threeButton;
@synthesize fourButton;
@synthesize fiveButton;
@synthesize sixButton;
@synthesize sevenButton;
@synthesize eightButton;
@synthesize nineButton;
@synthesize starButton;
@synthesize zeroButton;
@synthesize sharpButton;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"UICallBar" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [pauseButton release];
    [conferenceButton release];
    [videoButton release];
    [microButton release];
    [speakerButton release];
    [routesButton release];
    [optionsButton release];
    [routesBluetoothButton release];
    [routesReceiverButton release];
    [routesSpeakerButton release];
    [optionsAddButton release];
    [optionsTransferButton release];
    [dialerButton release];
    
    [oneButton release];
	[twoButton release];
	[threeButton release];
	[fourButton release];
	[fiveButton release];
	[sixButton release];
	[sevenButton release];
	[eightButton release];
	[nineButton release];
	[starButton release];
	[zeroButton release];
	[sharpButton release];
    
    [padView release];
    [routesView release];
    [optionsView release];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [pauseButton setType:UIPauseButtonType_CurrentCall call:nil];
    
    [zeroButton setDigit:'0'];
    [zeroButton setDtmf:true];
	[oneButton    setDigit:'1'];
    [oneButton setDtmf:true];
	[twoButton    setDigit:'2'];
    [twoButton setDtmf:true];
	[threeButton  setDigit:'3'];
    [threeButton setDtmf:true];
	[fourButton   setDigit:'4'];
    [fourButton setDtmf:true];
	[fiveButton   setDigit:'5'];
    [fiveButton setDtmf:true];
	[sixButton    setDigit:'6'];
    [sixButton setDtmf:true];
	[sevenButton  setDigit:'7'];
    [sevenButton setDtmf:true];
	[eightButton  setDigit:'8'];
    [eightButton setDtmf:true];
	[nineButton   setDigit:'9'];
    [nineButton setDtmf:true];
	[starButton   setDigit:'*'];
    [starButton setDtmf:true];
	[sharpButton  setDigit:'#'];
    [sharpButton setDtmf:true];

    {
        UIButton *videoButtonLandscape = (UIButton*)[landscapeView viewWithTag:[videoButton tag]];
        // Set selected+disabled background: IB lack !
        [videoButton setBackgroundImage:[UIImage imageNamed:@"video_on_disabled.png"]
                               forState:(UIControlStateDisabled | UIControlStateSelected)];
        [videoButtonLandscape setBackgroundImage:[UIImage imageNamed:@"video_on_disabled_landscape.png"]
                                        forState:(UIControlStateDisabled | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [videoButton setBackgroundImage:[UIImage imageNamed:@"video_on_over.png"]
                               forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [videoButtonLandscape setBackgroundImage:[UIImage imageNamed:@"video_on_over_landscape.png"]
                                        forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:videoButton];
        [LinphoneUtils buttonFixStates:videoButtonLandscape];
    }
    
    {
        UIButton *speakerButtonLandscape = (UIButton*) [landscapeView viewWithTag:[speakerButton tag]];
        // Set selected+disabled background: IB lack !
        [speakerButton setBackgroundImage:[UIImage imageNamed:@"speaker_on_disabled.png"]
                                 forState:(UIControlStateDisabled | UIControlStateSelected)];
        [speakerButtonLandscape setBackgroundImage:[UIImage imageNamed:@"speaker_on_disabled_landscape.png"]
                                          forState:(UIControlStateDisabled | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [speakerButton setBackgroundImage:[UIImage imageNamed:@"speaker_on_over.png"]
                                 forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [speakerButtonLandscape setBackgroundImage:[UIImage imageNamed:@"sspeaker_on_over_landscape.png"]
                                          forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:speakerButton];
        [LinphoneUtils buttonFixStates:speakerButtonLandscape];
    }
    
    if (![LinphoneManager runningOnIpad]) {
        UIButton *routesButtonLandscape = (UIButton*) [landscapeView viewWithTag:[routesButton tag]];
        // Set selected+over background: IB lack !
        [routesButton setBackgroundImage:[UIImage imageNamed:@"routes_over.png"]
                                 forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [routesButtonLandscape setBackgroundImage:[UIImage imageNamed:@"routes_over_landscape.png"]
                                          forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:routesButton];
        [LinphoneUtils buttonFixStates:routesButtonLandscape];
    }
    
    {
        UIButton *microButtonLandscape = (UIButton*) [landscapeView viewWithTag:[microButton tag]];
        // Set selected+disabled background: IB lack !
        [microButton setBackgroundImage:[UIImage imageNamed:@"micro_on_disabled.png"]
                               forState:(UIControlStateDisabled | UIControlStateSelected)];
        [microButtonLandscape setBackgroundImage:[UIImage imageNamed:@"micro_on_disabled_landscape.png"]
                                        forState:(UIControlStateDisabled | UIControlStateSelected)];
        
        // Set selected+over background: IB lack !
        [microButton setBackgroundImage:[UIImage imageNamed:@"micro_on_over.png"]
                               forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [microButtonLandscape setBackgroundImage:[UIImage imageNamed:@"micro_on_over_landscape.png"]
                                        forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:microButton];
        [LinphoneUtils buttonFixStates:microButtonLandscape];
    }
    
    {
        UIButton *optionsButtonLandscape = (UIButton*) [landscapeView viewWithTag:[optionsButton tag]];
        // Set selected+over background: IB lack !
        [optionsButton setBackgroundImage:[UIImage imageNamed:@"options_over.png"]
                                 forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [optionsButtonLandscape setBackgroundImage:[UIImage imageNamed:@"options_over_landscape.png"]
                                          forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:optionsButton];
        [LinphoneUtils buttonFixStates:optionsButtonLandscape];
    }
    
    {
        UIButton *pauseButtonLandscape = (UIButton*) [landscapeView viewWithTag:[pauseButton tag]];
        // Set selected+over background: IB lack !
        [pauseButton setBackgroundImage:[UIImage imageNamed:@"pause_on_over.png"]
                               forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [pauseButtonLandscape setBackgroundImage:[UIImage imageNamed:@"pause_on_over_landscape.png"]
                                        forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:pauseButton];
        [LinphoneUtils buttonFixStates:pauseButtonLandscape];
    }
    
    {
        UIButton *dialerButtonLandscape = (UIButton*) [landscapeView viewWithTag:[dialerButton tag]] ;
        // Set selected+over background: IB lack !
        [dialerButton setBackgroundImage:[UIImage imageNamed:@"dialer_alt_back_over.png"]
                                forState:(UIControlStateHighlighted | UIControlStateSelected)];
        [dialerButtonLandscape setBackgroundImage:[UIImage imageNamed:@"dialer_alt_back_over_landscape.png"] 
                                        forState:(UIControlStateHighlighted | UIControlStateSelected)];
        
        [LinphoneUtils buttonFixStates:dialerButton];
        [LinphoneUtils buttonFixStates:dialerButtonLandscape];
    }
    
    [super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdateEvent:) 
                                                 name:kLinphoneCallUpdate
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(bluetoothAvailabilityUpdateEvent:)
                                                 name:kLinphoneBluetoothAvailabilityUpdate
                                               object:nil];
    // Update on show
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    LinphoneCallState state = (call != NULL)?linphone_call_get_state(call): 0;
    [self callUpdate:call state:state];
    [self hideRoutes:FALSE];
    [self hideOptions:FALSE];
    [self hidePad:FALSE];
    [self showSpeaker];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:kLinphoneCallUpdate
                                                  object:nil];
	if (linphone_core_get_calls_nb([LinphoneManager getLc]) == 0) {
		//reseting speaker button because no more call
		speakerButton.selected=FALSE; 
	}
}

#pragma mark - Event Functions

- (void)callUpdateEvent:(NSNotification*)notif {
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:call state:state];
}

- (void)bluetoothAvailabilityUpdateEvent:(NSNotification*)notif {
    bool available = [[notif.userInfo objectForKey:@"available"] intValue];
    [self bluetoothAvailabilityUpdate:available];
}


#pragma mark - 

- (void)callUpdate:(LinphoneCall*)call state:(LinphoneCallState)state {  
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update call bar: Linphone core not ready"];
        return;
    }
    LinphoneCore* lc = [LinphoneManager getLc]; 

    [speakerButton update];
    [microButton update];
    [pauseButton update];
    [videoButton update];
    [hangupButton update];
    
    
    // Show Pause/Conference button following call count
    if(linphone_core_get_calls_nb(lc) > 1) {
        if(![pauseButton isHidden]) {
            [pauseButton setHidden:true];
            [conferenceButton setHidden:false];
        }
        bool enabled = true;
        const MSList *list = linphone_core_get_calls(lc);
        while(list != NULL) {
            LinphoneCall *call = (LinphoneCall*) list->data;
            LinphoneCallState state = linphone_call_get_state(call);
            if(state == LinphoneCallIncomingReceived ||
               state == LinphoneCallOutgoingInit ||
               state == LinphoneCallOutgoingProgress ||
               state == LinphoneCallOutgoingRinging ||
               state == LinphoneCallOutgoingEarlyMedia ||
               state == LinphoneCallConnected) {
                enabled = false;
            }
            list = list->next;
        }
        [conferenceButton setEnabled:enabled];
    } else {
        if([pauseButton isHidden]) {
            [pauseButton setHidden:false];
            [conferenceButton setHidden:true];
        }
    }

    // Disable transfert in conference
    if(linphone_core_get_current_call(lc) == NULL) {
        [optionsTransferButton setEnabled:FALSE];
    } else {
        [optionsTransferButton setEnabled:TRUE];
    }
    
    switch(state) {
        LinphoneCallEnd:
        LinphoneCallError:
        LinphoneCallIncoming:
        LinphoneCallOutgoing:
            [self hidePad:TRUE];
            [self hideOptions:TRUE];
            [self hideRoutes:TRUE];
        default:
            break;
    }
}

- (void)bluetoothAvailabilityUpdate:(bool)available {
    if (available) {
        [self hideSpeaker];
    } else {
        [self showSpeaker];
    }
}


#pragma mark -

- (void)showAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    CGRect frame = [target frame];
    int original_y = frame.origin.y;
    frame.origin.y = [[self view] frame].size.height;
    [target setFrame:frame];
    [target setHidden:FALSE];
    [UIView animateWithDuration:0.5
                          delay:0.0
                        options:UIViewAnimationOptionCurveEaseOut
                     animations:^{
                         CGRect frame = [target frame];
                         frame.origin.y = original_y;
                         [target setFrame:frame];
                     }
                     completion:^(BOOL finished){
                         CGRect frame = [target frame];
                         frame.origin.y = original_y;
                         [target setFrame:frame];
                         completion(finished);
                     }];
}

- (void)hideAnimation:(NSString*)animationID target:(UIView*)target completion:(void (^)(BOOL finished))completion {
    CGRect frame = [target frame];
    int original_y = frame.origin.y;
    [UIView animateWithDuration:0.5
                          delay:0.0
                        options:UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         CGRect frame = [target frame];
                         frame.origin.y = [[self view] frame].size.height;
                         [target setFrame:frame];
                     }
                     completion:^(BOOL finished){
                         CGRect frame = [target frame];
                         frame.origin.y = original_y;
                         [target setHidden:TRUE];
                         [target setFrame:frame];
                         completion(finished);
                     }];
}

- (void)showPad:(BOOL)animated {
    [dialerButton setOn];
    if([padView isHidden]) {
        if(animated) {
            [self showAnimation:@"show" target:padView completion:^(BOOL finished){}];
        } else {
            [padView setHidden:FALSE];
        }
    }
}

- (void)hidePad:(BOOL)animated {
    [dialerButton setOff];
    if(![padView isHidden]) {
        if(animated) {
            [self hideAnimation:@"hide" target:padView completion:^(BOOL finished){}];
        } else {
            [padView setHidden:TRUE];
        }
    }
}

- (void)showRoutes:(BOOL)animated {
    if (![LinphoneManager runningOnIpad]) {
        [routesButton setOn];
        [routesBluetoothButton setSelected:[[LinphoneManager instance] bluetoothEnabled]];
        [routesSpeakerButton setSelected:[[LinphoneManager instance] speakerEnabled]];
        [routesReceiverButton setSelected:!([[LinphoneManager instance] bluetoothEnabled] || [[LinphoneManager instance] speakerEnabled])];
        if([routesView isHidden]) {
            if(animated) {
                [self showAnimation:@"show" target:routesView completion:^(BOOL finished){}];
            } else {
                [routesView setHidden:FALSE];
            }
        }
    }
}

- (void)hideRoutes:(BOOL)animated {
    if (![LinphoneManager runningOnIpad]) {
        [routesButton setOff];
        if(![routesView isHidden]) {
            if(animated) {
                [self hideAnimation:@"hide" target:routesView completion:^(BOOL finished){}];
            } else {
                [routesView setHidden:TRUE];
            }
        }
    }
}

- (void)showOptions:(BOOL)animated {
    [optionsButton setOn];
    if([optionsView isHidden]) {
        if(animated) {
            [self showAnimation:@"show" target:optionsView completion:^(BOOL finished){}];
        } else {
            [optionsView setHidden:FALSE];
        }
    }
}

- (void)hideOptions:(BOOL)animated {
    [optionsButton setOff];
    if(![optionsView isHidden]) {
        if(animated) {
            [self hideAnimation:@"hide" target:optionsView completion:^(BOOL finished){}];
        } else {
            [optionsView setHidden:TRUE];
        }
    }
}

- (void)showSpeaker {
    if (![LinphoneManager runningOnIpad]) {
        [speakerButton setHidden:FALSE];
        [routesButton setHidden:TRUE];
    }
}

- (void)hideSpeaker {
    if (![LinphoneManager runningOnIpad]) {
        [speakerButton setHidden:TRUE];
        [routesButton setHidden:FALSE];
    }
}


#pragma mark - Action Functions

- (IBAction)onPadClick:(id)sender {
    if([padView isHidden]) {
        [self showPad:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
    } else {
        [self hidePad:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
    }
}

- (IBAction)onRoutesBluetoothClick:(id)sender {
    [self hideRoutes:TRUE];
    [[LinphoneManager instance] setBluetoothEnabled:TRUE];
}

- (IBAction)onRoutesReceiverClick:(id)sender {
    [self hideRoutes:TRUE];
    [[LinphoneManager instance] setSpeakerEnabled:FALSE];
    [[LinphoneManager instance] setBluetoothEnabled:FALSE];
}

- (IBAction)onRoutesSpeakerClick:(id)sender {
    [self hideRoutes:TRUE];
    [[LinphoneManager instance] setSpeakerEnabled:TRUE];
}

- (IBAction)onRoutesClick:(id)sender {
    if([routesView isHidden]) {
        [self showRoutes:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
    } else {
        [self hideRoutes:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
    }
}

- (IBAction)onOptionsTransferClick:(id)sender {
    [self hideOptions:TRUE];
    // Go to dialer view   
    DialerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
    if(controller != nil) {
        [controller setAddress:@""];
        [controller setTransferMode:TRUE];
    }
}

- (IBAction)onOptionsAddClick:(id)sender {
    [self hideOptions:TRUE];
    // Go to dialer view   
    DialerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]], DialerViewController);
    if(controller != nil) {
        [controller setAddress:@""];
        [controller setTransferMode:FALSE];
    }
}

- (IBAction)onOptionsClick:(id)sender {
    if([optionsView isHidden]) {
        [self showOptions:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
    } else {
        [self hideOptions:[[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"]];
    }
}

- (IBAction)onConferenceClick:(id)sender {
    linphone_core_add_all_to_conference([LinphoneManager getLc]);
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
