/*ConsoleViewController.h
 *
 * Copyright (C) 2010  Belledonne Comunications, Grenoble, France
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


#import "ConsoleViewController.h"


@implementation ConsoleViewController
NSMutableString* MoreViewController_logs;

@synthesize logs;
@synthesize logsView;

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
	UIBarButtonItem* clear = [[[UIBarButtonItem alloc] 
							  initWithBarButtonSystemItem:UIBarButtonSystemItemTrash 
							  target:self 
							  action:@selector(doAction)] autorelease]; 
		[self.navigationItem setRightBarButtonItem:clear];
}


/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

-(void) viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
	[logs setText:MoreViewController_logs];
}

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}


-(void) addLog:(NSString*) log {
	@synchronized(self) {
		if (!MoreViewController_logs) {
			MoreViewController_logs = [[NSMutableString alloc] init];
		}
		[MoreViewController_logs appendString:log];
		if (MoreViewController_logs.length > 50000) {
			NSRange range = {0,log.length};
			[MoreViewController_logs deleteCharactersInRange:range];
		}
	}
}
-(void) doAction{
	@synchronized(self) {
		NSMutableString* oldString = MoreViewController_logs;
		MoreViewController_logs =  [[NSMutableString alloc] init];
		[oldString release];
		[logs setText:@""];
	}
}



- (void)dealloc {
    [super dealloc];
}


@end
