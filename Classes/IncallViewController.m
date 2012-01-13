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
#import "VideoViewController.h"
#import <AudioToolbox/AudioToolbox.h>
#import <AddressBook/AddressBook.h>
#import "linphonecore.h"
#include "LinphoneManager.h"
#include "private.h"
#import "ContactPickerDelegate.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

const NSInteger SECURE_BUTTON_TAG=5;

@implementation IncallViewController

@synthesize controlSubView;
@synthesize padSubView;
@synthesize hangUpView;
@synthesize conferenceDetail;

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
@synthesize videoViewController;

@synthesize addVideo;


+ (UIImage*) stat_sys_signal_0 {
	return [UIImage imageNamed:@"stat_sys_signal_0.png"];
}

+ (UIImage*) stat_sys_signal_1 {
	return [UIImage imageNamed:@"stat_sys_signal_1.png"];
}

+ (UIImage*) stat_sys_signal_2 {
	return [UIImage imageNamed:@"stat_sys_signal_2.png"];
}

+ (UIImage*) stat_sys_signal_3 {
	return [UIImage imageNamed:@"stat_sys_signal_3.png"];
}

+ (UIImage*) stat_sys_signal_4 {
	return [UIImage imageNamed:@"stat_sys_signal_4.png"];
}

bool isInConference(LinphoneCall* call) {
    if (!call)
        return false;
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

-(void) updateUIFromLinphoneState:(UIViewController *)viewCtrl {
    activeCallCell = nil;
    [mute reset];
    
    LinphoneCore* lc;
    
    @try {
        lc = [LinphoneManager getLc];

        [LinphoneManager set:pause hidden:(callCount([LinphoneManager getLc]) > 1) withName:"PAUSE button" andReason:"call count"];
        [LinphoneManager set:mergeCalls hidden:!pause.hidden withName:"MERGE button" andReason:"call count"];

        [callTableView reloadData];       
    } @catch (NSException* exc) {
        return;
    }
    LinphoneCall* selectedCall = linphone_core_get_current_call([LinphoneManager getLc]);
    int callsCount = linphone_core_get_calls_nb(lc);
    // hide pause/resume if in conference    
    if (selectedCall) {
        if (linphone_core_is_in_conference(lc)) {
            [LinphoneManager set:pause hidden:YES withName:"PAUSE button" andReason:"is in conference"];
        }
        else if (callCount(lc) == callsCount && callsCount == 1) {
            [LinphoneManager set:pause hidden:NO withName:"PAUSE button" andReason:"call count == 1"];
            pause.selected = NO;
        } else {
            [LinphoneManager set:pause hidden:YES withName:"PAUSE button" andReason:AT];
        }
    } else {
        if (callsCount == 1) {
            LinphoneCall* c = (LinphoneCall*)linphone_core_get_calls(lc)->data;
            if (linphone_call_get_state(c) == LinphoneCallPaused ||
                linphone_call_get_state(c) == LinphoneCallPausing) {
                pause.selected = YES;                
            }
            [LinphoneManager set:pause hidden:NO withName:"PAUSE button" andReason:AT];
        } else {
            [LinphoneManager set:pause hidden:YES withName:"PAUSE button" andReason:AT];
        }
    }
    [LinphoneManager set:mergeCalls hidden:!pause.hidden withName:"MERGE button" andReason:AT];
    
    // update conference details view if diaplsyed
    if (self.presentedViewController == conferenceDetail) {
        if (!linphone_core_is_in_conference(lc))
            [self dismissModalViewControllerAnimated:YES];
        else
            [conferenceDetail.table reloadData];
    }
}

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	//Controls
	[mute initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] debugName:"MUTE button"];
    [speaker initWithOnImage:[UIImage imageNamed:@"HP_inverse.png"]  offImage:[UIImage imageNamed:@"HP.png"] debugName:"SPEAKER button"];
    
    verified = [[UIImage imageNamed:@"secured.png"] retain];
    unverified = [[UIImage imageNamed:@"unverified.png"] retain];

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
    
    [addCall addTarget:self action:@selector(addCallPressed) forControlEvents:UIControlEventTouchUpInside];
    [mergeCalls addTarget:self action:@selector(mergeCallsPressed) forControlEvents:UIControlEventTouchUpInside];
    [pause addTarget:self action:@selector(pauseCallPressed) forControlEvents:UIControlEventTouchUpInside];
    [LinphoneManager set:mergeCalls hidden:YES withName:"MERGE button" andReason:"initialisation"];
    
    if ([LinphoneManager runningOnIpad]) {
        ms_message("Running on iPad");
        mVideoViewController =  [[VideoViewController alloc]  initWithNibName:@"VideoViewController-ipad" 
                                                                       bundle:[NSBundle mainBundle]];
        conferenceDetail = [[ConferenceCallDetailView alloc]  initWithNibName:@"ConferenceCallDetailView-ipad" 
                                                                       bundle:[NSBundle mainBundle]];

    } else {
        mVideoViewController =  [[VideoViewController alloc]  initWithNibName:@"VideoViewController" 
																							 bundle:[NSBundle mainBundle]];
        conferenceDetail = [[ConferenceCallDetailView alloc]  initWithNibName:@"ConferenceCallDetailView" 
                                                                       bundle:[NSBundle mainBundle]];

    }
    
    mVideoShown=FALSE;
	mIncallViewIsReady=FALSE;
	mVideoIsPending=FALSE;
    //selectedCall = nil;
    
    callTableView.rowHeight = 80;
    
}

-(void) addCallPressed {
    [LinphoneManager logUIElementPressed:"CALL button"];
    [self dismissModalViewControllerAnimated:true];
}


-(void) mergeCallsPressed {
    [LinphoneManager logUIElementPressed:"MERGE button"];
    LinphoneCore* lc = [LinphoneManager getLc];
    linphone_core_add_all_to_conference(lc);
}

-(void) pauseCallPressed {
    [LinphoneManager logUIElementPressed:"PAUSE button"];
    LinphoneCore* lc = [LinphoneManager getLc];
    
    LinphoneCall* currentCall = linphone_core_get_current_call(lc);
	if (currentCall) {
        if (linphone_call_get_state(currentCall) == LinphoneCallStreamsRunning) {
            [pause setSelected:NO];
            linphone_core_pause_call(lc, currentCall);
        }
    } else {
        if (linphone_core_get_calls_nb(lc) == 1) {
            LinphoneCall* c = (LinphoneCall*) linphone_core_get_calls(lc)->data;
            if (linphone_call_get_state(c) == LinphoneCallPaused) {
                linphone_core_resume_call(lc, c);
                [pause setSelected:YES];
            }
        }
    }
}


-(void)updateCallsDurations {
    [self updateUIFromLinphoneState: nil]; 
}

-(void) viewWillAppear:(BOOL)animated {}

-(void)viewDidAppear:(BOOL)animated {
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    [super viewDidAppear:animated];
	if (dismissed) {
        [self dismissModalViewControllerAnimated:true];
    } else {
        [self updateCallsDurations];
        durationRefreasher = [NSTimer	scheduledTimerWithTimeInterval:1
                                                              target:self 
                                                            selector:@selector(updateCallsDurations) 
                                                            userInfo:nil 
                                                             repeats:YES];
        glowingTimer = [NSTimer	scheduledTimerWithTimeInterval:0.1 
                                                              target:self 
                                                            selector:@selector(updateGlow) 
                                                            userInfo:nil 
                                                             repeats:YES];
        glow = 0;
		mIncallViewIsReady=TRUE; 
		if (mVideoIsPending) {
			mVideoIsPending=FALSE;
			[self displayVideoCall:nil FromUI:self 
						   forUser:nil 
				   withDisplayName:nil];
			
		}

		
		UIDevice* device = [UIDevice currentDevice];
		if ([device respondsToSelector:@selector(isMultitaskingSupported)]
			&& [device isMultitaskingSupported]) {
			bool enableVideo = [[NSUserDefaults standardUserDefaults] boolForKey:@"enable_video_preference"];
			bool startVideo = [[NSUserDefaults standardUserDefaults] boolForKey:@"start_video_preference"];
            
            [LinphoneManager set:contacts hidden:(enableVideo && !startVideo) withName:"CONTACT button" andReason:AT];
            [LinphoneManager set:addVideo hidden:!contacts.hidden withName:"ADD_VIDEO button" andReason:AT];
		}    
    }
}

-(void) viewWillDisappear:(BOOL)animated {
    if (zrtpVerificationSheet != nil) {
        [zrtpVerificationSheet dismissWithClickedButtonIndex:2 animated:NO];
    }
}

- (void) viewDidDisappear:(BOOL)animated {
    if (durationRefreasher != nil) {
        [durationRefreasher invalidate];
        durationRefreasher=nil;
        [glowingTimer invalidate];
        glowingTimer = nil;
    }
	if (!mVideoShown) [[UIApplication sharedApplication] setIdleTimerDisabled:false];
	mIncallViewIsReady=FALSE;
}

- (void)viewDidUnload {
    [verified release];
    [unverified release];
}



-(void) displayStatus:(NSString*) message; {

}

-(void) displayPad:(bool) enable {
    [LinphoneManager set:callTableView hidden:enable withName:"CALL_TABLE view" andReason:AT];
    [LinphoneManager set:hangUpView hidden:enable withName:"HANG_UP view" andReason:AT];
    [LinphoneManager set:controlSubView hidden:enable withName:"CONTROL view" andReason:AT];
    [LinphoneManager set:padSubView hidden:!enable withName:"PAD view" andReason:AT];
}
-(void) displayCall:(LinphoneCall*) call InProgressFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	//restaure view
	[self displayPad:false];
	dismissed = false;
	UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
	if ([speaker isOn]) 
		[speaker toggle];
    [self updateUIFromLinphoneState: nil]; 
}

-(void) displayIncomingCall:(LinphoneCall *)call NotificationFromUI:(UIViewController *)viewCtrl forUser:(NSString *)username withDisplayName:(NSString *)displayName {
    
}

-(void) dismissVideoView {
	[[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationNone]; 
	[self dismissModalViewControllerAnimated:FALSE];//just in case
	 mVideoShown=FALSE;
}
-(void) displayInCall:(LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
    dismissed = false;
	UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = YES;
	if (call !=nil  && linphone_call_get_dir(call)==LinphoneCallIncoming) {
		if ([speaker isOn]) [speaker toggle];
	}
    [self updateUIFromLinphoneState: nil];
	if (self.presentedViewController == (UIViewController*)mVideoViewController) {
		[self dismissVideoView];
	}
}
-(void) displayDialerFromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName {
	UIViewController* modalVC = self.modalViewController;
	UIDevice *device = [UIDevice currentDevice];
    device.proximityMonitoringEnabled = NO;
    if (modalVC != nil) {
        // clear previous native window ids
        if (modalVC == mVideoViewController) {
            mVideoShown=FALSE;
            linphone_core_set_native_video_window_id([LinphoneManager getLc],0);	
            linphone_core_set_native_preview_window_id([LinphoneManager getLc],0);
        }
		[[UIApplication sharedApplication] setStatusBarHidden:NO withAnimation:UIStatusBarAnimationNone]; 
		[self dismissModalViewControllerAnimated:FALSE];//just in case
    }

	[self dismissModalViewControllerAnimated:FALSE]; //disable animation to avoid blanc bar just below status bar*/
    dismissed = true;
    [self updateUIFromLinphoneState: nil]; 
}
-(void) displayVideoCall:(LinphoneCall*) call FromUI:(UIViewController*) viewCtrl forUser:(NSString*) username withDisplayName:(NSString*) displayName { 
	if (mIncallViewIsReady) {
	[[UIApplication sharedApplication] setStatusBarHidden:YES withAnimation:UIStatusBarAnimationNone];
	mVideoShown=TRUE;
	[self presentModalViewController:mVideoViewController animated:true];
	} else {
		//postepone presentation
		mVideoIsPending=TRUE;
	}
}

- (IBAction)doAction:(id)sender {
	
	if (sender == dialer) {
		[self displayPad:true];
		
	} else if (sender == contacts) {
		// start people picker
		myPeoplePickerController = [[[ABPeoplePickerNavigationController alloc] init] autorelease];
		[myPeoplePickerController setPeoplePickerDelegate:[[[ContactPickerDelegate alloc] init] autorelease]];
		
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

+(LinphoneCall*) retrieveCallAtIndex: (NSInteger) index inConference:(bool) conf{
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

-(void) updateActive:(bool_t)active cell:(UITableViewCell*) cell {
    if (!active) {
        
        cell.backgroundColor = [UIColor colorWithRed:0.2 green:0.2 blue:0.2 alpha:0.2];
        
        UIColor* c = [[UIColor blackColor] colorWithAlphaComponent:0.5];
        [cell.textLabel setTextColor:c];
        [cell.detailTextLabel setTextColor:c];
    } else {
        cell.backgroundColor = [UIColor colorWithRed:0.4 green:0.4 blue:0.4 alpha:(0.7+sin(2*glow)*0.3)];
        [cell.textLabel setTextColor:[UIColor whiteColor]];  
        [cell.detailTextLabel setTextColor:[UIColor whiteColor]];
    } 
    [cell.textLabel setBackgroundColor:[UIColor clearColor]];
    [cell.detailTextLabel setBackgroundColor:[UIColor clearColor]];
}

-(void) updateGlow {
    if (!activeCallCell)
        return;
    
    glow += 0.1;

    [self updateActive:YES cell:activeCallCell];
    [activeCallCell.backgroundView setNeedsDisplay];
    [activeCallCell setNeedsDisplay];
    [callTableView setNeedsDisplay];
}

-(void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
    [self updateActive:(cell == activeCallCell) cell:cell];
}

+ (void) updateCellImageView:(UIImageView*)imageView Label:(UILabel*)label DetailLabel:(UILabel*)detailLabel AndAccessoryView:(UIView*)accessoryView withCall:(LinphoneCall*) call {
    if (call == NULL) {
        ms_warning("UpdateCell called with null call");
        [label setText:@""];
        return;
    }
    const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    
    if (addr) {
		const char* lUserNameChars=linphone_address_get_username(addr);
		NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
        NSMutableString* mss = [[NSMutableString alloc] init];
        /* contact name */
        const char* n = linphone_address_get_display_name(addr);
        if (n) 
            [mss appendFormat:@"%s", n, nil];
        else
            [mss appendFormat:@"%@",lUserName , nil];
        
        if ([mss compare:label.text] != 0 || imageView.image == nil) {
            [label setText:mss];
        
            imageView.image = [[LinphoneManager instance] getImageFromAddressBook:lUserName];
        }
		[mss release];
    } else {
        [label setText:@"plop"];
        imageView.image = nil;
    }
    
    if (detailLabel != nil) {
        NSMutableString* ms = [[NSMutableString alloc] init ];
        if (linphone_call_get_state(call) == LinphoneCallStreamsRunning) {
            int duration = linphone_call_get_duration(call);
            if (duration >= 60)
                [ms appendFormat:@"%02i:%02i", (duration/60), duration - 60*(duration/60), nil];
            else
                [ms appendFormat:@"%02i sec", duration, nil];
        } else {
            switch (linphone_call_get_state(call)) {
                case LinphoneCallPaused:
                    [ms appendFormat:@"%@", NSLocalizedString(@"Paused (tap to resume)", nil), nil];
                    break;
                case LinphoneCallOutgoingProgress:
                    [ms appendFormat:@"%@...", NSLocalizedString(@"In progress", nil), nil];
                    break;
                default:
                    break;
            }
        }
        [detailLabel setText:ms];
		[ms release];
    }
}


-(void) updateConferenceCell:(UITableViewCell*) cell at:(NSIndexPath*)indexPath {
    LinphoneCore* lc = [LinphoneManager getLc];
    
    NSString* t= [NSString stringWithFormat:
                  NSLocalizedString(@"Conference", nil), 
                  linphone_core_get_conference_size(lc) - linphone_core_is_in_conference(lc)];
    [cell.textLabel setText:t];
    
    [self updateActive:NO cell:cell];
    cell.selected = NO;
    
    [callTableView deselectRowAtIndexPath:indexPath animated:NO];
    
    if (!linphone_core_is_in_conference(lc)) {
        [cell.detailTextLabel setText:NSLocalizedString(@"(tap to enter conference)", nil)];
    } else {
        [cell.detailTextLabel setText:
         [NSString stringWithFormat:NSLocalizedString(@"(me + %d participants)", nil), linphone_core_get_conference_size(lc) - linphone_core_is_in_conference(lc)]];
    }	
    cell.imageView.image = nil;
}

-(void) tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
    // show conference detail view
    [self presentModalViewController:conferenceDetail animated:true];

}
       
// UITableViewDataSource (required)
- (UITableViewCell*) tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [callTableView dequeueReusableCellWithIdentifier:@"MyIdentifier"];
    if (cell == nil) {
        cell = [[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"MyIdentifier"] autorelease];
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        
        cell.textLabel.font = [UIFont systemFontOfSize:40];
        cell.textLabel.autoresizingMask = UIViewAutoresizingFlexibleHeight;
    }
    
    LinphoneCore* lc = [LinphoneManager getLc];
	
	if (indexPath.row == 0 && linphone_core_get_conference_size(lc) > 0) {
        [self updateConferenceCell:cell at:indexPath];
        if (linphone_core_is_in_conference(lc))
            activeCallCell = cell;
		cell.accessoryView = nil;
        if (linphone_core_is_in_conference(lc))
            cell.accessoryType = UITableViewCellAccessoryDetailDisclosureButton;
        else
            cell.accessoryType = UITableViewCellAccessoryNone;
    } else {
        LinphoneCall* call = [IncallViewController retrieveCallAtIndex:indexPath.row inConference:NO];
		LinphoneMediaEncryption enc = linphone_call_params_get_media_encryption(linphone_call_get_current_params(call));
        if (call == nil)
            return nil;
		if (cell.accessoryView == nil) {
			UIView *containerView = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 28, 28)] autorelease];
			cell.accessoryView = containerView;
		}
		else {
			for (UIView *view in cell.accessoryView.subviews) {
				[view removeFromSuperview];
			}
		}
        [IncallViewController updateCellImageView:cell.imageView Label:cell.textLabel DetailLabel:cell.detailTextLabel AndAccessoryView:(UIView*)cell.accessoryView withCall:call];
        if (linphone_core_get_current_call(lc) == call)
            activeCallCell = cell;
        cell.accessoryType = UITableViewCellAccessoryNone;
		
		// Call Quality Indicator
		UIImageView* callquality = [UIImageView new];
		[callquality setFrame:CGRectMake(0, 0, 28, 28)];
		if (call->state == LinphoneCallStreamsRunning) 
		{
			if (linphone_call_get_average_quality(call) >= 4) {
				[callquality setImage: [IncallViewController stat_sys_signal_4]];
			}
			else if (linphone_call_get_average_quality(call) >= 3) {
				[callquality setImage: [IncallViewController stat_sys_signal_3]];
			}
			else if (linphone_call_get_average_quality(call) >= 2) {
				[callquality setImage: [IncallViewController stat_sys_signal_2]];
			}
			else if (linphone_call_get_average_quality(call) >= 1) {
				[callquality setImage: [IncallViewController stat_sys_signal_1]];
			}
			else {
				[callquality setImage: [IncallViewController stat_sys_signal_0]];
			}
		}
		else {
			[callquality setImage:nil];
		}
		
        if (enc != LinphoneMediaEncryptionNone) {
            cell.accessoryView = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 60, 28)] autorelease];
            UIButton* accessoryBtn = [UIButton buttonWithType:UIButtonTypeCustom];
            [accessoryBtn setFrame:CGRectMake(30, 0, 28, 28)];
            [accessoryBtn setImage:nil forState:UIControlStateNormal];
            [accessoryBtn setTag:SECURE_BUTTON_TAG];
            accessoryBtn.backgroundColor = [UIColor clearColor];
            accessoryBtn.userInteractionEnabled = YES;
			
            if (enc == LinphoneMediaEncryptionSRTP || linphone_call_get_authentication_token_verified(call)) {
                [accessoryBtn setImage: verified forState:UIControlStateNormal];
            } else {
                [accessoryBtn setImage: unverified forState:UIControlStateNormal];
            }
			[cell.accessoryView addSubview:accessoryBtn];
			
			if (((UIButton*)accessoryBtn).imageView.image != nil && linphone_call_params_get_media_encryption(linphone_call_get_current_params(call)) == LinphoneMediaEncryptionZRTP) {
				[((UIButton*)accessoryBtn) addTarget:self action:@selector(secureIconPressed:withEvent:) forControlEvents:UIControlEventTouchUpInside];
			}
        } 
		
		[cell.accessoryView addSubview:callquality];
        [callquality release];
    }
    
    cell.userInteractionEnabled = YES; 
    cell.selectionStyle = UITableViewCellSelectionStyleNone;
    return cell;
} 

-(void) secureIconPressed:(UIControl*) button withEvent: (UIEvent*) evt {
    NSSet* touches = [evt allTouches];
    UITouch* touch = [touches anyObject];
    CGPoint currentTouchPos = [touch locationInView:self.callTableView];
    NSIndexPath *path = [self.callTableView indexPathForRowAtPoint:currentTouchPos];
    if (path) {
        LinphoneCall* call = [IncallViewController retrieveCallAtIndex:path.row inConference:NO];
        // start action sheet to validate/unvalidate zrtp code
        CallDelegate* cd = [[CallDelegate alloc] init];
        cd.delegate = self;
        cd.call = call;
        UIView* container=(UIView*)[callTableView cellForRowAtIndexPath:path].accessoryView;
        UIButton *button=(UIButton*)[container viewWithTag:SECURE_BUTTON_TAG];
        [button setImage:nil forState:UIControlStateNormal];
            
		zrtpVerificationSheet = [[UIActionSheet alloc] initWithTitle:[NSString  stringWithFormat:NSLocalizedString(@" Mark auth token '%s' as:",nil),linphone_call_get_authentication_token(call)]
                                                    delegate:cd 
                                                    cancelButtonTitle:NSLocalizedString(@"Unverified",nil) 
                                                    destructiveButtonTitle:NSLocalizedString(@"Verified",nil) 
                                                    otherButtonTitles:nil];
        
		zrtpVerificationSheet.actionSheetStyle = UIActionSheetStyleDefault;
		[zrtpVerificationSheet showInView:self.view];
		[zrtpVerificationSheet release];
    }
}

-(void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex withUserDatas:(void *)datas {
    LinphoneCall* call = (LinphoneCall*)datas;
    // maybe we could verify call validity
    
    if (buttonIndex == 0)
        linphone_call_set_authentication_token_verified(call, YES);
    else if (buttonIndex == 1)
        linphone_call_set_authentication_token_verified(call, NO);
    zrtpVerificationSheet = nil;
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
- (NSString*) tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    return nil;
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

    bool inConf = (indexPath.row == 0 && linphone_core_get_conference_size(lc) > 0);
    
    LinphoneCall* selectedCall = [IncallViewController retrieveCallAtIndex:indexPath.row inConference:inConf];
    
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


@end
