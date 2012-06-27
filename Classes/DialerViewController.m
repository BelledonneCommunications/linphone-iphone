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
@synthesize hashButton;

- (id)init {
    return [super initWithNibName:@"DialerViewController" bundle:[NSBundle mainBundle]];
}

- (void)viewWillAppear:(BOOL)animated {
   // [[LinphoneManager instance] setRegistrationDelegate:self];
    
    //TODO
    //[mMainScreenWithVideoPreview showPreview:YES];
    
    if([LinphoneManager isLcReady]) {
        LinphoneCore *lc = [LinphoneManager getLc];
        if(linphone_core_get_calls_nb(lc) > 0) {
            [addCallButton setHidden:false];
            [callButton setHidden:true];
            [cancelButton setHidden:false]; 
            [addContactButton setHidden:true];
        } else {
            [addCallButton setHidden:true];
            [callButton setHidden:false];
            [cancelButton setHidden:true];
            [addContactButton setHidden:false];
        }
    }
    [super viewWillAppear:animated];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
	[zeroButton   initWithNumber:'0'   addressField:addressField dtmf:false];
	[oneButton    initWithNumber:'1'   addressField:addressField dtmf:false];
	[twoButton    initWithNumber:'2'   addressField:addressField dtmf:false];
	[threeButton  initWithNumber:'3'   addressField:addressField dtmf:false];
	[fourButton   initWithNumber:'4'   addressField:addressField dtmf:false];
	[fiveButton   initWithNumber:'5'   addressField:addressField dtmf:false];
	[sixButton    initWithNumber:'6'   addressField:addressField dtmf:false];
	[sevenButton  initWithNumber:'7'   addressField:addressField dtmf:false];
	[eightButton  initWithNumber:'8'   addressField:addressField dtmf:false];
	[nineButton   initWithNumber:'9'   addressField:addressField dtmf:false];
	[starButton   initWithNumber:'*'   addressField:addressField dtmf:false];
	[hashButton   initWithNumber:'#'   addressField:addressField dtmf:false];
	[callButton   initWithAddress:addressField];
	[addCallButton   initWithAddress:addressField];
	[eraseButton  initWithAddressField:addressField];
}

- (void)setAddress:(NSString*) address {
    [addressField setText:address];
}

- (void)dealloc {
	[addressField release];
    [addContactButton release];
    [cancelButton release];
    [eraseButton release];
	[callButton release];
    [addCallButton release];
    
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
	[hashButton release];
	[super dealloc];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    if (textField == addressField) {
        [addressField resignFirstResponder];
    } 
    return YES;
}

- (IBAction)onAddContactClick: (id) event {
    
}

- (IBAction)onCancelClick: (id) event {
    [[LinphoneManager instance] changeView:PhoneView_InCall];
}

- (IBAction)onAddCallClick: (id) event {
    
}

- (IBAction)onAddressChange: (id)sender {
    if([[addressField text] length] > 0) {
        [addContactButton setEnabled:TRUE];
        [eraseButton setEnabled:TRUE];
        [callButton setEnabled:TRUE];
        [addCallButton setEnabled:TRUE];
    } else {
        [addContactButton setEnabled:FALSE];
        [eraseButton setEnabled:FALSE];
        [callButton setEnabled:FALSE];
        [addCallButton setEnabled:FALSE];
    }
}

@end
