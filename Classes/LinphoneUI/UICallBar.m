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
@synthesize startConferenceButton;
@synthesize stopConferenceButton;
@synthesize videoButton;
@synthesize microButton;
@synthesize speakerButton;  
@synthesize optionsButton;

- (id)init {
    return [super initWithNibName:@"UICallBar" bundle:[NSBundle mainBundle]];
}

- (void)viewDidLoad {
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
    
    if(linphone_core_get_calls_nb(lc) > 1) {
        [pauseButton setHidden:true];
        LinphoneCall *currentCall = linphone_core_get_current_call(lc);
        if(currentCall == NULL || !linphone_call_get_current_params(currentCall)->in_conference) {
            [startConferenceButton setHidden:false];    
            [stopConferenceButton setHidden:true];   
        } else {
            [startConferenceButton setHidden:true];    
            [stopConferenceButton setHidden:false];
        }
    } else {
        [pauseButton setHidden:false];
        [startConferenceButton setHidden:true];
        [stopConferenceButton setHidden:true];
    }
}

- (void)dealloc {
    [pauseButton release];
    [startConferenceButton release];
    [stopConferenceButton release];
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

@end
