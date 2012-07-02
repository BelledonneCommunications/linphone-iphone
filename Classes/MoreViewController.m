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
#include "lpconfig.h"

@implementation MoreViewController

@synthesize web;
@synthesize credit;
@synthesize console;
@synthesize creditText;
@synthesize weburi;


#pragma mark - Lifecycle Functions

- (void)viewDidLoad {
    [super viewDidLoad];
	[creditText setText: [NSString stringWithFormat:creditText.text,[[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]]];
	consoleViewController = [[ConsoleViewController alloc] initWithNibName:@"ConsoleViewController" bundle:[NSBundle mainBundle]];
	[[LinphoneManager instance] registerLogView:consoleViewController];
	isDebug =  lp_config_get_int(linphone_core_get_config([LinphoneManager getLc]),"app","debugenable_preference",0);  

}

- (void)dealloc {
    [credit release];
	[creditText release];
    
	[web release];
	[weburi release];
	[console release];
    
	[consoleViewController release];
    [super dealloc];
}


#pragma mark - 

-(void) enableLogView {
	isLogViewEnabled = true;
}


#pragma mark - UITableViewDelegate Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}


#pragma mark - UITableViewDataSource Functions

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	if (indexPath.section == 0) {
		return 230;
	} else {
		return 44;
	}
}

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
