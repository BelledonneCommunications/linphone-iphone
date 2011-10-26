/* IncallViewController.h
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
#import "IncallViewController.h"
#import <AudioToolbox/AudioToolbox.h>
#import "linphonecore.h"



@implementation IncallViewController


@synthesize controlSubView;
@synthesize padSubView;

@synthesize peerName;
@synthesize peerNumber;
@synthesize callDuration;
@synthesize status;
@synthesize endCtrl;
@synthesize close;
@synthesize mute;
@synthesize pause;
@synthesize dialer;
@synthesize speaker;
@synthesize contacts;

@synthesize one;
@synthesize two;
@synthesize three;
@synthesize four;
@synthesize five;
@synthesize six;
@synthesize seven;
@synthesize eight;
@synthesize nine;
@synthesize star;
@synthesize zero;
@synthesize hash;
@synthesize endPad;

/*
// The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization

    }
    return self;
}
*/


// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	//Controls
	[mute initWithOnImage:[UIImage imageNamed:@"mic_muted.png"]  offImage:[UIImage imageNamed:@"mic_active.png"] ];
    [pause initWithOnImage:[UIImage imageNamed:@"resumecall.png"]  offImage:[UIImage imageNamed:@"pausecall.png"] ];
	[speaker initWithOnImage:[UIImage imageNamed:@"Speaker-32-on.png"]  offImage:[UIImage imageNamed:@"Speaker-32-off.png"] ];
	

	//Dialer init
	[zero initWithNumber:'0'];
	[one initWithNumber:'1'];
	[two initWithNumber:'2'];
	[three initWithNumber:'3'];
	[four initWithNumber:'4'];
	[five initWithNumber:'5'];
	[six initWithNumber:'6'];
	[seven initWithNumber:'7'];
	[eight initWithNumber:'8'];
	[nine initWithNumber:'9'];
	[star initWithNumber:'*'];
	[hash initWithNumber:'#'];
	
	
}




- (void)viewDidUnload {

	
}

-(void) displayStatus:(NSString*) message; {
	[status setText:message];
}

-(void) displayPad:(bool) enable {
	[controlSubView setHidden:enable];
	[padSubView setHidden:!enable];
}
-(void) displayCallInProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	//restaure view
	[self displayPad:false];
	
	if (displayName && [displayName length]>0) {
		[peerName setText:displayName];
		[peerNumber setText:username];
	} else {
		[peerName setText:username];
		[peerNumber setText:@""];
	}
	[callDuration setText:@"Calling"];
}

-(void) displayIncallFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[callDuration start];
}
-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[callDuration stop];
	[self dismissModalViewControllerAnimated:true];
}
-(void) updateUIFromLinphoneState:(UIViewController *)viewCtrl {
    [mute reset];
    [pause reset];
}

- (IBAction)doAction:(id)sender {
	
	if (sender == dialer) {
		[self displayPad:true];
		
	} else if (sender == contacts) {
		// start people picker
		myPeoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
		[myPeoplePickerController setPeoplePickerDelegate:self];
		
		[self presentModalViewController: myPeoplePickerController animated:true]; 
	} else if (sender == close) {
		[self displayPad:false];
	} 	
}

// handle people picker behavior

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker 
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person {
	return true;
	
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker 
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person 
								property:(ABPropertyID)property 
							  identifier:(ABMultiValueIdentifier)identifier {
	
	return false;
}

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[self dismissModalViewControllerAnimated:true];
}




- (void)dealloc {
    [super dealloc];
}


@end
