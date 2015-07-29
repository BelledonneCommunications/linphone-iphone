//
//  SideMenuTableViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import "SideMenuTableViewController.h"
#import "Utils.h"

#import "PhoneMainView.h"
#import "UIStateBar.h"

@implementation SideMenuEntry

- (id)initWithTitle:(NSString *)atitle tapBlock:(SideMenuEntryBlock)tapBlock {
	if ((self = [super init])) {
		self->title = atitle;
		self->onTapBlock = tapBlock;
	}
	return self;
}

@end

@implementation SideMenuTableViewController

- (void)viewDidLoad {
	_sideMenuEntries = [[NSMutableArray alloc] init];

	[_sideMenuEntries
		addObject:[[SideMenuEntry alloc] initWithTitle:NSLocalizedString(@"Settings", nil)
											  tapBlock:^() {
												[PhoneMainView.instance
													changeCurrentView:SettingsViewController.compositeViewDescription
																 push:NO
															 animated:NO];
											  }]];
	[_sideMenuEntries
		addObject:[[SideMenuEntry alloc] initWithTitle:NSLocalizedString(@"About", nil)
											  tapBlock:^() {
												[PhoneMainView.instance
													changeCurrentView:AboutViewController.compositeViewDescription
																 push:NO
															 animated:NO];

											  }]];
}

#pragma mark - Table View Controller
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [_sideMenuEntries count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	SideMenuEntry *entry = [_sideMenuEntries objectAtIndex:indexPath.row];
	UITableViewCell *cell = [[UITableViewCell alloc] init];
	cell.textLabel.text = entry->title;
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];

	SideMenuEntry *entry = [_sideMenuEntries objectAtIndex:indexPath.row];
	LOGI(@"Entry %@ has been tapped", entry->title);
	if (entry->onTapBlock == nil) {
		LOGF(@"Entry %@ has no onTapBlock!", entry->title);
	}
	entry->onTapBlock();
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

@end
