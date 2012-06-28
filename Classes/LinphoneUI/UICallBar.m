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

- (id)init {
    return [super initWithNibName:@"UICallBar" bundle:[NSBundle mainBundle]];
}

- (void)viewDidLoad {
    [pauseButton setType:UIPauseButtonType_CurrentCall call:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(callUpdate:) name:@"LinphoneCallUpdate" object:nil];
}

- (void)viewDidUnload {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)callUpdate: (NSNotification*) notif {
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
