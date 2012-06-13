/* ConferenceCallDetailView.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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
@synthesize addCall;

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
    
    /*[mute initWithOnImage:[UIImage imageNamed:@"micro_inverse.png"]  offImage:[UIImage imageNamed:@"micro.png"] debugName:"MUTE button"];
    [speaker initWithOnImage:[UIImage imageNamed:@"HP_inverse.png"]  offImage:[UIImage imageNamed:@"HP.png"] debugName:"SPEAKER button"];
     */
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
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    [super viewWillAppear:animated];
}

-(void) viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
	callQualityRefresher = [NSTimer scheduledTimerWithTimeInterval:1
							target:self 
							selector:@selector(updateCallQuality) 
							userInfo:nil 
							repeats:YES];
}

-(void) viewDidDisappear:(BOOL)animated {
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
    
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
	LinphoneCall* call = [InCallViewController retrieveCallAtIndex:indexPath.row inConference:YES];
    [InCallViewController updateCellImageView:image Label:label DetailLabel:nil AndAccessoryView:nil withCall:call];
    
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
	UIImageView* callquality = (UIImageView*) [cell viewWithTag:3];
    [InCallViewController updateIndicator:callquality withCallQuality:linphone_call_get_average_quality(call)];
    tableView.rowHeight = 80;
    
    return cell;
}

-(NSInteger) tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    LinphoneCore* lc = [LinphoneManager getLc];
    int result = linphone_core_get_conference_size(lc) - (int)linphone_core_is_in_conference(lc);
    return result;
}

@end
