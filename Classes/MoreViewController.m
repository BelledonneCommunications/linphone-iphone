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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */          

#import "MoreViewController.h"
#import "AboutViewController.h"
#include "ConsoleViewController.h"



@implementation MoreViewController
@synthesize web;
@synthesize help;
@synthesize reset;
@synthesize credit;
@synthesize console;
@synthesize linphoneDelegate;




 // The designated initializer.  Override if you create the controller programmatically and want to perform customization that is not appropriate for viewDidLoad.
- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    if (self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil]) {
        // Custom initialization
		isLogViewEnabled = false;
    }
    return self;
}



//Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad {
    [super viewDidLoad];
	aboutViewController = [[AboutViewController alloc] initWithNibName:@"AboutViewController" bundle:[NSBundle mainBundle]];
	consoleViewController = [[ConsoleViewController alloc] initWithNibName:@"ConsoleViewController" bundle:[NSBundle mainBundle]];
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
    return 1;
}


// Customize the number of rows in the table view.
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (isLogViewEnabled) {
		return 5;
	} else {
		return 4;
	}
}



- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    switch (indexPath.row) {
		case 0: return web;
		case 1: return help;
		case 2: return reset;
		case 3: return credit;
		case 4: return console;
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
			NSString *stringURL = @"http://dialer.axtellabs.net";
			NSURL *url = [NSURL URLWithString:stringURL];
			[[UIApplication sharedApplication] openURL:url];
			break;
		};
		case 1:{
			NSString *stringURL = @"http://help.dialer.axtellabs.net/help.html?version=1.1";
			NSURL *url = [NSURL URLWithString:stringURL];
			[[UIApplication sharedApplication] openURL:url];
			break;
		};
		case 2: {
			UIAlertView* ckecking = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Alert",nil)
																message:NSLocalizedString(@"Esto borrara sus configuraciones actuales, seguro que desea continuar?",nil) 
																delegate:self 
																cancelButtonTitle:NSLocalizedString(@"Si",nil) 
																otherButtonTitles:nil];
			[ckecking addButtonWithTitle:NSLocalizedString(@"No",nil)];
			[ckecking show];
			break;
		}
		case 3:  {
			[self.navigationController pushViewController:aboutViewController animated:true];
			break;
		}
		case 4:  {
			[self.navigationController pushViewController:consoleViewController animated:true];
			break;
		}
    }
	
}

- (void)alertView:(UIAlertView *)alertView didDismissWithButtonIndex:(NSInteger)buttonIndex {
	if (buttonIndex == 0) {
		[linphoneDelegate resetConfig];
	}
}
@end
