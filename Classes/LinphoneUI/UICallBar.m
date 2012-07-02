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

#import "CPAnimationSequence.h"
#import "CPAnimationStep.h"

#include "linphonecore.h"
#include "private.h"

@implementation UICallBar

@synthesize pauseButton;
@synthesize conferenceButton;
@synthesize videoButton;
@synthesize microButton;
@synthesize speakerButton;  
@synthesize optionsButton;
@synthesize hangupButton;
@synthesize padView;

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
@synthesize hashButton;


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
    [optionsButton release];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [pauseButton setType:UIPauseButtonType_CurrentCall call:nil];
    
    [zeroButton   initWithNumber:'0'   addressField:nil dtmf:true];
	[oneButton    initWithNumber:'1'   addressField:nil dtmf:true];
	[twoButton    initWithNumber:'2'   addressField:nil dtmf:true];
	[threeButton  initWithNumber:'3'   addressField:nil dtmf:true];
	[fourButton   initWithNumber:'4'   addressField:nil dtmf:true];
	[fiveButton   initWithNumber:'5'   addressField:nil dtmf:true];
	[sixButton    initWithNumber:'6'   addressField:nil dtmf:true];
	[sevenButton  initWithNumber:'7'   addressField:nil dtmf:true];
	[eightButton  initWithNumber:'8'   addressField:nil dtmf:true];
	[nineButton   initWithNumber:'9'   addressField:nil dtmf:true];
	[starButton   initWithNumber:'*'   addressField:nil dtmf:true];
	[hashButton   initWithNumber:'#'   addressField:nil dtmf:true];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(callUpdate:) name:@"LinphoneCallUpdate" object:nil];
    
    // Set selected+over background: IB lack !
    [videoButton setBackgroundImage:[UIImage imageNamed:@"video-ON-disabled.png"] 
                           forState:(UIControlStateDisabled | UIControlStateSelected)];
}

- (void)viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}


#pragma mark - Event Functions

- (void)callUpdate: (NSNotification*) notif {
    //LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
    LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
    
    // check LinphoneCore is initialized
    LinphoneCore* lc = nil;
    if([LinphoneManager isLcReady])
        lc = [LinphoneManager getLc];
    
    //TODO
    //[LinphoneManager set:mergeCalls hidden:!pause.hidden withName:"MERGE button" andReason:"call count"];     

    [speakerButton update];
    [microButton update];
    [pauseButton update];
    [videoButton update];
    [hangupButton update];
    
    if(linphone_core_get_calls_nb(lc) > 1) {
        if(![pauseButton isHidden]) {
            [pauseButton setHidden:true];
            [conferenceButton setHidden:false];
        }
    } else {
        if([pauseButton isHidden]) {
            [pauseButton setHidden:false];
            [conferenceButton setHidden:true];
        }
    }
    
    if(linphone_core_get_current_call(lc) == NULL) {
        [self hidePad];
    }

    switch(state) {
        LinphoneCallEnd:
        LinphoneCallError:
        LinphoneCallIncoming:
        LinphoneCallOutgoing:
            [self hidePad];
        default:
            break;
    }
}


#pragma mark -  

- (void)showPad{
    if([padView isHidden]) {
        CGRect frame = [padView frame];
        int original_y = frame.origin.y;
        frame.origin.y = [[self view] frame].size.height;
        [padView setFrame:frame];
        [padView setHidden:FALSE];
        CPAnimationSequence* move = [[CPAnimationSequence sequenceWithSteps:
                                     [[CPAnimationStep for:0.5 animate:^{ 
            CGRect frame = [padView frame];
            frame.origin.y = original_y;
            [padView setFrame:frame]; 
        }] autorelease],
                                     nil
                                     ] autorelease];
        [move run];
    }
}

- (void)hidePad{
    if(![padView isHidden]) {
        CGRect frame = [padView frame];
        int original_y = frame.origin.y;
    
        CPAnimationSequence* move = [[CPAnimationSequence sequenceWithSteps:
                                     [[CPAnimationStep for:0.5 animate:^{ 
            CGRect frame = [padView frame];
            frame.origin.y = [[self view] frame].size.height;
            [padView setFrame:frame]; 
        }] autorelease],
                                     [[CPAnimationStep after:0.0 animate:^{ 
            CGRect frame = [padView frame];
            frame.origin.y = original_y;
            [padView setHidden:TRUE];
            [padView setFrame:frame]; 
        }] autorelease], 
                                     nil
                                     ] autorelease];
    [move run];
    }
}


#pragma mark - Action Functions

- (IBAction)onPadClick:(id)sender {
    if([padView isHidden]) {
        [self showPad];
    } else {
        [self hidePad];
    }
}

- (IBAction)onOptionsClick:(id)sender {
    // Go to dialer view
    NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                           [[[NSArray alloc] initWithObjects: @"", nil] autorelease]
                           , @"setAddress:",
                           nil] autorelease];
    [[LinphoneManager instance] changeView:PhoneView_Dialer dict:dict];
}

- (IBAction)onConferenceClick:(id)sender {
    linphone_core_add_all_to_conference([LinphoneManager getLc]);
}

@end
