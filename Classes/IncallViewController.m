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
@synthesize callControlSubView;
@synthesize padSubView;

@synthesize addToConf;
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
    [endCtrl addTarget:self action:@selector(endCallPressed) forControlEvents:UIControlEventTouchUpInside];
    [addToConf addTarget:self action:@selector(addToConfCallPressed) forControlEvents:UIControlEventTouchUpInside];
    
    [mergeCalls setHidden:YES];
    
    //selectedCall = nil;
}

-(void) addCallPressed {
    [self dismissModalViewControllerAnimated:true];
}

-(void) mergeCallsPressed {
    LinphoneCore* lc = [LinphoneManager getLc];
    
    linphone_core_add_all_to_conference(lc);
}

-(void) addToConfCallPressed {
    if (!selectedCall)
        return;
    linphone_core_add_to_conference([LinphoneManager getLc], selectedCall);
}



-(void)updateCallsDurations {
    [self updateUIFromLinphoneState: nil]; 
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
    }
}

- (void) viewDidDisappear:(BOOL)animated {
    if (durationRefreasher != nil) {
        [durationRefreasher invalidate];
        durationRefreasher=nil;
    }
}

- (void)viewDidUnload {

	
}

-(void) displayStatus:(NSString*) message; {
    [self updateUIFromLinphoneState: nil]; 
}

-(void) displayPad:(bool) enable {
	[controlSubView setHidden:enable];
	[padSubView setHidden:!enable];
}
-(void) displayCall:(LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	//restaure view
	[self displayPad:false];
	dismissed = false;
    
    [self updateUIFromLinphoneState: nil]; 
}

-(void) displayIncomingCall:(LinphoneCall *)call NotificationFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    
}

-(void) displayInCall:(LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    dismissed = false;
    if (call)
        selectedCall = call;
    [self updateUIFromLinphoneState: nil]; 
}
-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	[self dismissModalViewControllerAnimated:true];
    dismissed = true;
    [self updateUIFromLinphoneState: nil]; 
}
-(void) updateUIFromLinphoneState:(UIViewController *)viewCtrl {
    [mute reset];
    [pause reset];
    
    LinphoneCore* lc;
    
    @try {
        lc = [LinphoneManager getLc];
        
        if (callCount([LinphoneManager getLc]) > 1) {
            [pause setHidden:YES];
            [mergeCalls setHidden:NO];
        } else {
            [pause setHidden:NO];
            [mergeCalls setHidden:YES];        
        }
        
        [callTableView reloadData];       
    } @catch (NSException* exc) {
        return;
    }
    
    // hide call control subview if no call selected
    [callControlSubView setHidden:(selectedCall == NULL)];
    // hide add to conf if no conf exist
    if (!callControlSubView.hidden) {
        [addToConf setHidden:(linphone_core_get_conference_size(lc) == 0 ||
                            isInConference(selectedCall))];
    }
    // hide pause/resume if in conference
    [pause setHidden:linphone_core_is_in_conference(lc)];
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
    
    if (!conf && linphone_core_get_conference_size([LinphoneManager getLc]))
        index--;
    
    while (calls != 0) {
        if (isInConference((LinphoneCall*)calls->data) == conf) {
            if (index == 0)
                break;
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



- (void) updateCell:(UITableViewCell*)cell at:(NSIndexPath*) path withCall:(LinphoneCall*) call conferenceActive:(bool)confActive{
    if (call == NULL) {
        ms_warning("UpdateCell called with null call");
        [cell.textLabel setText:@""];
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
        
    
    if (linphone_core_get_current_call([LinphoneManager getLc]) == call) {
        cell.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:0.5];
    } else if (confActive && isInConference(call)) {
        cell.backgroundColor = [UIColor colorWithRed:0 green:0 blue:1 alpha:0.5];
    } else{
        cell.backgroundColor = [UIColor colorWithRed:1 green:0.5 blue:0 alpha:0.5];
    }
    
    if (call == selectedCall) {
        // [cell setSelected:YES animated:NO];
        cell.accessoryType = UITableViewCellAccessoryCheckmark;
    }else{
        //[cell setSelected:NO animated:NO];
        cell.accessoryType = UITableViewCellAccessoryNone;
    }
}

-(void) updateConferenceCell:(UITableViewCell*) cell at:(NSIndexPath*)indexPath {
    [cell.textLabel setText:@"Conference"];
    
    LinphoneCore* lc = [LinphoneManager getLc];
    
    cell.accessoryType = UITableViewCellAccessoryNone;
    
    NSMutableString* ms = [[NSMutableString alloc] init ];
    const MSList* calls = linphone_core_get_calls(lc);
    while (calls) {
        LinphoneCall* call = (LinphoneCall*)calls->data;
        if (isInConference(call)) {
             const LinphoneAddress* addr = linphone_call_get_remote_address(call);
            
            const char* n = linphone_address_get_display_name(addr);
            if (n) 
                [ms appendFormat:@"%s ", n, nil];
            else
                [ms appendFormat:@"%s ", linphone_address_get_username(addr), nil];
            
            
            if (call == selectedCall)
                cell.accessoryType = UITableViewCellAccessoryCheckmark;
        }
        calls = calls->next;
    }
    [cell.detailTextLabel setText:ms];
    
    if (linphone_core_is_in_conference(lc))
        cell.backgroundColor = [UIColor colorWithRed:0 green:1 blue:0 alpha:0.5];
    else
        cell.backgroundColor = [UIColor colorWithRed:1 green:0 blue:0 alpha:0.5];
}


// UITableViewDataSource (required)
- (UITableViewCell*) tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [callTableView dequeueReusableCellWithIdentifier:@"MyIdentifier"];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"MyIdentifier"] autorelease];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
    }
    
    LinphoneCore* lc = [LinphoneManager getLc];
    if (indexPath.row == 0 && linphone_core_get_conference_size(lc) > 0)
        [self updateConferenceCell:cell at:indexPath];
    else
        [self updateCell:cell at:indexPath withCall: [self retrieveCallAtIndex:indexPath.row inConference:NO]
            conferenceActive:linphone_core_is_in_conference(lc)];

    cell.userInteractionEnabled = YES;
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    cell.selectionStyle = UITableViewCellSelectionStyleBlue;
    
    
    
    /*NSString *path = [[NSBundle mainBundle] pathForResource:[item objectForKey:@"imageKey"] ofType:@"png"];
    UIImage *theImage = [UIImage imageWithContentsOfFile:path];
    cell.imageView.image = theImage;*/
    return cell;
}


// UITableViewDataSource (required)
- (NSInteger) tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    LinphoneCore* lc = [LinphoneManager getLc];
    
    return callCount(lc) + (int)(linphone_core_get_conference_size(lc) > 0);
    
    if (section == 0 && linphone_core_get_conference_size(lc) > 0)
        return linphone_core_get_conference_size(lc) - linphone_core_is_in_conference(lc);
    
    return callCount(lc);
}

// UITableViewDataSource
- (NSInteger) numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
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
    return @"Calls";
    if (section == 0 && linphone_core_get_conference_size([LinphoneManager getLc]) > 0)
        return @"Conference";
    else
        return @"Calls";
}

// UITableViewDataSource
- (NSString*) tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
    return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:NO];

    LinphoneCore* lc = [LinphoneManager getLc];
    
    [[callTableView cellForRowAtIndexPath:indexPath] setSelected:YES animated:NO];
        
    bool inConf = (indexPath.row == 0 && linphone_core_get_conference_size(lc) > 0);
    
    selectedCall = [self retrieveCallAtIndex:indexPath.row inConference:inConf];
    
    if (inConf) {
        if (linphone_core_is_in_conference(lc))
            return;
        LinphoneCall* current = linphone_core_get_current_call(lc);
        if (current)
            linphone_core_pause_call(lc, current);
        linphone_core_enter_conference([LinphoneManager getLc]);
    } else if (selectedCall) {
        if (linphone_core_is_in_conference(lc)) {
            linphone_core_leave_conference(lc);
        }
        linphone_core_resume_call([LinphoneManager getLc], selectedCall);
    }
    
    [self updateUIFromLinphoneState: nil];    
}

-(void) endCallPressed {
    if (selectedCall == NULL) {
        ms_error("No selected call");
        return;
    }
    
    LinphoneCore* lc = [LinphoneManager getLc];
    if (isInConference(selectedCall)) {
        linphone_core_terminate_conference(lc);
        /*
        linphone_core_remove_from_conference(lc, selectedCall);
        if ((linphone_core_get_conference_size(lc) - (int)linphone_core_is_in_conference(lc)) == 0)
            linphone_core_terminate_conference(lc);
         */
    } else {
        linphone_core_terminate_call(lc, selectedCall);
    }
    selectedCall = NULL;
}

@end
