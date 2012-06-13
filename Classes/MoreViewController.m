/* MoreViewController.m
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

#import "MoreViewController.h"
#include "ConsoleViewController.h"
#import "LinphoneManager.h"

@implementation MoreViewController
@synthesize web;
@synthesize credit;
@synthesize console;
@synthesize creditText;
@synthesize weburi;

//Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	[creditText setText: [NSString stringWithFormat:creditText.text,[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]]];
	consoleViewController = [[ConsoleViewController alloc] initWithNibName:@"ConsoleViewController" bundle:[NSBundle mainBundle]];
	[[LinphoneManager instance] registerLogView:consoleViewController];
	isDebug =  [[NSUserDefaults standardUserDefaults] boolForKey:@"debugenable_preference"];  

}

/*
// Override to allow orientations other than the default portrait orientation.
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}
*/

- (void)didReceiveMemoryWarning {
	// Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload {
	// Release any retained subviews of the main view.
	// e.g. self.myOutlet = nil;
}

- (void)dealloc {
    [super dealloc];
}

-(void) enableLogView {
	isLogViewEnabled = true;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	if (indexPath.section == 0) {
		return 230;
	} else {
		return 44;
	}
}

// Customize the number of rows in the table view.
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section == 0) {
		return 1;
	} else {
		if (isDebug) {
			return 2;
		} else {
			return 1;
		}
	}
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 0) {
		return credit;
	} else {
		switch (indexPath.row) {
			case 0: return web;
			case 1: return console;
		}
	}
	return nil;
}

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath {
	
	[self tableView:tableView didSelectRowAtIndexPath:indexPath];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {


	[tableView deselectRowAtIndexPath:indexPath animated:NO];
	
    switch (indexPath.row) {
		case 0:  {
			NSURL *url = [NSURL URLWithString:weburi.text];
			[[UIApplication sharedApplication] openURL:url];
			break;
		};
		case 1:  {
			[self.navigationController pushViewController:consoleViewController animated:true];
			break;
		}
    }
	
}

@end
