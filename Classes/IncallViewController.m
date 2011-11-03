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
#include "LinphoneManager.h"
#include "private.h"

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
@synthesize callTableView;
@synthesize addCall;
@synthesize mergeCalls;

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


bool isInConference(LinphoneCall* call) {
    return linphone_call_get_current_params(call)->in_conference;
}

int callCount(LinphoneCore* lc) {
    int count = 0;
    const MSList* calls = linphone_core_get_calls(lc);
    
    while (calls != 0) {
        if (!isInConference((LinphoneCall*)calls->data)) {
            count++;
        }
        calls = calls->next;
    }
    return count;
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	//Controls
	[mute initWithOnImage:[UIImage imageNamed:@"mic_muted.png"]  offImage:[UIImage imageNamed:@"mic_active.png"] ];
    UIImage* rc = [UIImage imageNamed:@"resumecall.png"];
    UIImage* pc = [UIImage imageNamed:@"pausecall.png"];
    [pause initWithOnImage:rc  offImage:pc ];
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
    
    [addCall addTarget:self action:@selector(addCallPressed) forControlEvents:UIControlEventTouchDown];
    [mergeCalls addTarget:self action:@selector(mergeCallsPressed) forControlEvents:UIControlEventTouchDown];  
    
    [mergeCalls setHidden:YES];
}

-(void) addCallPressed {
    [self dismissModalViewControllerAnimated:true];
}

-(void) mergeCallsPressed {
    LinphoneCore* lc = [LinphoneManager getLc];
    const MSList* calls = linphone_core_get_calls(lc);
    
    while (calls != 0) {
        LinphoneCall* call = (LinphoneCall*)calls->data;
        if (!isInConference(call)) {
            linphone_core_add_to_conference(lc, call);
        }
        calls = calls->next;
    }
}

-(void)updateCallsDurations {
    [callTableView reloadData];
}

-(void)viewDidAppear:(BOOL)animated {
    if (dismissed) {
        [self dismissModalViewControllerAnimated:true];
    } else {
        [self updateCallsDurations];
        durationRefreasher = [NSTimer	scheduledTimerWithTimeInterval:1 
                                                              target:self 
                                                            selector:@selector(updateCallsDurations) 
                                                            userInfo:nil 
                                                             repeats:YES];
       selectedCell = nil;
    }
}

- (void) viewDidDisappear:(BOOL)animated {
    if (durationRefreasher != nil) {
        [durationRefreasher invalidate];
        durationRefreasher=nil;
        selectedCell = nil;
    }
}

- (void)viewDidUnload {

	
}

-(void) displayStatus:(NSString*) message; {
	[status setText:message];
    
        [callTableView reloadData];
}

-(void) displayPad:(bool) enable {
	[controlSubView setHidden:enable];
	[padSubView setHidden:!enable];
}
-(void) displayCall:(LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
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
    dismissed = false;
    
        [callTableView reloadData];
}

-(void) displayIncomingCall:(LinphoneCall *)call NotificationFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    
}

-(void) displayInCall:(LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    dismissed = false;
    [callTableView reloadData];
}
-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[callDuration stop];
	[self dismissModalViewControllerAnimated:true];
    dismissed = true;
        [callTableView reloadData];
}
-(void) updateUIFromLinphoneState:(UIViewController *)viewCtrl {
    [mute reset];
    [pause reset];
    
    if (callCount([LinphoneManager getLc]) > 1) {
        [pause setHidden:YES];
        [mergeCalls setHidden:NO];
    } else {
        [pause setHidden:NO];
        [mergeCalls setHidden:YES];        
    }
    
    [callTableView reloadData];
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

-(LinphoneCall*) retrieveCallAtIndex: (NSInteger) index inConference:(bool) conf{
    const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);
    
    while (calls != 0 && index > 0) {
        if (isInConference((LinphoneCall*)calls->data) == conf) {
            index--;
        }
        calls = calls->next;
    }
    
    if (calls == 0) {
        ms_error("Cannot find call with index %d (in conf: %d)", index, conf);
        return nil;
    } else {
        return (LinphoneCall*)calls->data;
    }
}



- (void) updateCell:(UITableViewCell*)cell withCall:(LinphoneCall*) call conferenceActive:(bool)confActive{
    if (call == NULL) {
        ms_error("UpdateCell called with null call");
        [cell.textLabel setText:@"BUG IN APP - call is null"];
        return;
    }
    const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    if (addr) {
        NSMutableString* mss = [[NSMutableString alloc] init];
        
        const char* n = linphone_address_get_display_name(addr);
        if (n) 
            [mss appendFormat:@"%s", n, nil];
        else
            [mss appendFormat:@"%s", linphone_address_get_username(addr), nil];
        [cell.textLabel setText:mss];
    } else
        [cell.textLabel setText:@"plop"];
    
    NSMutableString* ms = [[NSMutableString alloc] init ];
    if (linphone_call_get_state(call) == LinphoneCallStreamsRunning) {
        int duration = linphone_call_get_duration(call);
        if (duration >= 60)
            [ms appendFormat:@"%02i:%02i", (duration/60), duration - 60*(duration/60), nil];
        else
            [ms appendFormat:@"%02i sec", duration, nil];
    } else {
        [ms appendFormat:@"%s", linphone_call_state_to_string(linphone_call_get_state(call)), nil];
    }
    [cell.detailTextLabel setText:ms];
    
    
    if (linphone_core_get_current_call([LinphoneManager getLc]) == call)
        cell.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:0.5];
    else if (confActive && isInConference(call))
       cell.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:0.5];
    else
       cell.backgroundColor = [UIColor colorWithRed:1 green:0.5 blue:0 alpha:0.5];
     
    
    /*if (cell == selectedCell)
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
    else
        cell.accessoryType = UITableViewCellAccessoryNone;*/
}


// UITableViewDataSource (required)
- (UITableViewCell*) tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [callTableView dequeueReusableCellWithIdentifier:@"MyIdentifier"];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"MyIdentifier"] autorelease];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    }
    
    LinphoneCore* lc = [LinphoneManager getLc];
    if (indexPath.section == 0 && linphone_core_get_conference_size(lc) > 0)
        [self updateCell:cell withCall: [self retrieveCallAtIndex:indexPath.row inConference:true] conferenceActive:linphone_core_is_in_conference(lc)];
    else
        [self updateCell:cell withCall: [self retrieveCallAtIndex:indexPath.row inConference:false]
            conferenceActive:linphone_core_is_in_conference(lc)];

    
    /*NSString *path = [[NSBundle mainBundle] pathForResource:[item objectForKey:@"imageKey"] ofType:@"png"];
    UIImage *theImage = [UIImage imageWithContentsOfFile:path];
    cell.imageView.image = theImage;*/
    return cell;
}


// UITableViewDataSource (required)
- (NSInteger) tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    LinphoneCore* lc = [LinphoneManager getLc];
    
    if (section == 0 && linphone_core_get_conference_size(lc) > 0)
        return linphone_core_get_conference_size(lc) - linphone_core_is_in_conference(lc);
    
    return callCount(lc);
}

// UITableViewDataSource
- (NSInteger) numberOfSectionsInTableView:(UITableView *)tableView {
    LinphoneCore* lc = [LinphoneManager getLc];
    int count = 0;
    
    if (callCount(lc) > 0)
        count++;
    
    if (linphone_core_get_conference_size([LinphoneManager getLc]) > 0)
        count ++;
    
    return count;
}

// UITableViewDataSource
//- (NSArray*) sectionIndexTitlesForTableView:(UITableView *)tableView {
//   return [NSArray arrayWithObjects:@"Conf", @"Calls", nil ];
//}

// UITableViewDataSource
- (NSString*) tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    if (section == 0 && linphone_core_get_conference_size([LinphoneManager getLc]) > 0)
        return @"CONF";
    else
        return @"CALLS";
}

// UITableViewDataSource
- (NSString*) tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
    return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    //selectedCell = [tableView cellForRowAtIndexPath:indexPath];
    
    bool inConf = (indexPath.section == 0 && linphone_core_get_conference_size([LinphoneManager getLc]) > 0);
    
    if (inConf) {
        linphone_core_enter_conference([LinphoneManager getLc]);
    } else {    
        LinphoneCall* call = [self retrieveCallAtIndex:indexPath.row inConference:NO];
        linphone_core_resume_call([LinphoneManager getLc], call);
    }
    
    [self updateUIFromLinphoneState: nil];    
}

@end
