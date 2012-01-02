//
//  ConferenceCallDetailView.m
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 25/11/11.
//  Copyright (c) 2011 Belledonne Communications. All rights reserved.
//

#import "ConferenceCallDetailView.h"
#import "linphonecore.h"
#import "LinphoneManager.h"
#import "IncallViewController.h"

@implementation ConferenceCallDetailView

@synthesize mute;
@synthesize speaker;
@synthesize back;
@synthesize hangup;
@synthesize table;

@synthesize conferenceDetailCell;

NSTimer *callQualityRefresher;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [back addTarget:self action:@selector(backButtonPressed) forControlEvents:UIControlEventTouchUpInside];
    
    table.rowHeight = 80;
    
    [mute initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] ];
    [speaker initWithOnImage:[UIImage imageNamed:@"HP_inverse.png"]  offImage:[UIImage imageNamed:@"HP.png"] ];
}

-(void) backButtonPressed {
    [self dismissModalViewControllerAnimated:YES];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

-(void) viewWillAppear:(BOOL)animated {
    [table reloadData];
    [mute reset];
    [speaker reset];
}

-(void) viewDidAppear:(BOOL)animated {
	callQualityRefresher = [NSTimer scheduledTimerWithTimeInterval:1
							target:self 
							selector:@selector(updateCallQuality) 
							userInfo:nil 
							repeats:YES];
}

-(void) viewDidDisappear:(BOOL)animated {
	if (callQualityRefresher != nil) {
        [callQualityRefresher invalidate];
        callQualityRefresher=nil;
	}
}

-(void) updateCallQuality {
	[table reloadData];
	[table setNeedsDisplay];
}

#pragma mark - UITableView delegates
-(void) tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.row % 2)
        cell.backgroundColor = [UIColor lightGrayColor];
    else
        cell.backgroundColor = [UIColor darkGrayColor];
}

-(UITableViewCell*) tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString* identifier = @"ConferenceDetailCellIdentifier";
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:identifier];
    if (cell == nil) {
        [[NSBundle mainBundle] loadNibNamed:@"ConferenceCallDetailCell" owner:self options:nil];
        cell = conferenceDetailCell;
        self.conferenceDetailCell = nil;
    }
    
    /* retrieve cell's fields using tags */
    UIImageView* image = (UIImageView*) [cell viewWithTag:1];
    UILabel* label = (UILabel*) [cell viewWithTag:2];
    
    /* update cell content */
	LinphoneCall* call = [IncallViewController retrieveCallAtIndex:indexPath.row inConference:YES];
    [IncallViewController updateCellImageView:image Label:label DetailLabel:nil AndAccessoryView:nil withCall:call];
    
	cell.accessoryType = UITableViewCellAccessoryNone;
	if (cell.accessoryView == nil) {
		UIView *containerView = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 28, 28)] autorelease];
		cell.accessoryView = containerView;
	}
	else {
		for (UIView *view in cell.accessoryView.subviews) {
			[view removeFromSuperview];
		}
	}
	UIImageView* callquality = [UIImageView new];
	[callquality setFrame:CGRectMake(0, 0, 28, 28)];
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
	[cell.accessoryView addSubview:callquality];
	
    tableView.rowHeight = 80;//cell.bounds.size.height;
    
    return cell;
}

-(NSInteger) tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    LinphoneCore* lc = [LinphoneManager getLc];
    int result = linphone_core_get_conference_size(lc) - (int)linphone_core_is_in_conference(lc);
    return result;
}

@end
