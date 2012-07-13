/* DialerViewController.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */               

#import <AVFoundation/AVAudioSession.h>
#import <AudioToolbox/AudioToolbox.h>

#import "DialerViewController.h"
#import "IncallViewController.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#include "linphonecore.h"
#include "private.h"

@implementation DialerViewController

@synthesize addressField;
@synthesize addContactButton;
@synthesize cancelButton;
@synthesize addCallButton;
@synthesize transferButton;
@synthesize callButton;
@synthesize eraseButton;

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
    self = [super initWithNibName:@"DialerViewController" bundle:[NSBundle mainBundle]];
    if(self) {
        self->transferMode = FALSE;
    }
    return self;
}

- (void)dealloc {
	[addressField release];
    [addContactButton release];
    [cancelButton release];
    [eraseButton release];
	[callButton release];
    [addCallButton release];
    [transferButton release];
    
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
    
    
    // Remove all observers
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
	[super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

+ (UICompositeViewDescription *)compositeViewDescription {
    UICompositeViewDescription *description = [UICompositeViewDescription alloc];
    description->content = @"DialerViewController";
    description->tabBar = @"UIMainBar";
    description->tabBarEnabled = true;
    description->stateBar = @"UIStateBar";
    description->stateBarEnabled = true;
    description->fullscreen = false;
    return description;
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(callUpdateEvent:) 
                                                 name:@"LinphoneCallUpdate" 
                                               object:nil];
    // Update on show
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    LinphoneCallState state = (call != NULL)?linphone_call_get_state(call): 0;
    [self callUpdate:call state:state];
} 

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneCallUpdate" 
                                                  object:nil];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
	[zeroButton    setDigit:'0'];
	[oneButton     setDigit:'1'];
	[twoButton     setDigit:'2'];
	[threeButton   setDigit:'3'];
	[fourButton    setDigit:'4'];
	[fiveButton    setDigit:'5'];
	[sixButton     setDigit:'6'];
	[sevenButton   setDigit:'7'];
	[eightButton   setDigit:'8'];
	[nineButton    setDigit:'9'];
	[starButton    setDigit:'*'];
	[sharpButton   setDigit:'#'];
}

#pragma mark - Event Functions

- (void)callUpdateEvent:(NSNotification*)notif { 
    LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    [self callUpdate:call state:state];
}


#pragma mark -

- (void)callUpdate:(LinphoneCall*)call state:(LinphoneCallState)state {
    if([LinphoneManager isLcReady]) {
        LinphoneCore *lc = [LinphoneManager getLc];
        if(linphone_core_get_calls_nb(lc) > 0) {
            if(transferMode) {
                [addCallButton setHidden:true];
                [transferButton setHidden:false];
            } else {
                [addCallButton setHidden:false];
                [transferButton setHidden:true];
            }
            [callButton setHidden:true];
            [cancelButton setHidden:false]; 
            [addContactButton setHidden:true];
        } else {
            [addCallButton setHidden:true];
            [callButton setHidden:false];
            [cancelButton setHidden:true];
            [addContactButton setHidden:false];
            [transferButton setHidden:true];
        }
    }
}

- (void)setAddress:(NSString*) address {
    [addressField setText:address];
}

- (void)setTransferMode:(NSNumber*) n {
    transferMode = [n boolValue];
    LinphoneCall* call = linphone_core_get_current_call([LinphoneManager getLc]);
    LinphoneCallState state = (call != NULL)?linphone_call_get_state(call): 0;
    [self callUpdate:call state:state];
}

- (void)call:(NSString*)address {
    [self call:address displayName:nil];
}

- (void)call:(NSString*)address displayName:(NSString *)displayName {
    [[LinphoneManager instance] call:address displayName:displayName transfer:transferMode];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    if (textField == addressField) {
        [addressField resignFirstResponder];
    } 
    return YES;
}


#pragma mark - Action Functions

- (IBAction)onAddContactClick: (id) event {
    // Go to Contact details view
    NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                           [[[NSArray alloc] initWithObjects:[addressField text], nil] autorelease]
                           , @"setAddress:",
                           nil] autorelease];
    [[PhoneMainView instance] changeView:PhoneView_Contacts dict:dict push:TRUE];
}

- (IBAction)onBackClick: (id) event {
    [[PhoneMainView instance] changeView:PhoneView_InCall];
}

- (IBAction)onAddressChange: (id)sender {
    if([[addressField text] length] > 0) {
        [addContactButton setEnabled:TRUE];
        [eraseButton setEnabled:TRUE];
        [callButton setEnabled:TRUE];
        [addCallButton setEnabled:TRUE];
        [transferButton setEnabled:TRUE];
    } else {
        [addContactButton setEnabled:FALSE];
        [eraseButton setEnabled:FALSE];
        [callButton setEnabled:FALSE];
        [addCallButton setEnabled:FALSE];
        [transferButton setEnabled:FALSE];
    }
}

@end
