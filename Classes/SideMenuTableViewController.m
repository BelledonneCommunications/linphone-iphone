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
		addObject:[[SideMenuEntry alloc] initWithTitle:NSLocalizedString(@"Assistant", nil)
											  tapBlock:^() {
												[PhoneMainView.instance
													changeCurrentView:AssistantViewController.compositeViewDescription
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
	return ms_list_size(linphone_core_get_proxy_config_list([LinphoneManager getLc])) > 1 ? 2 : 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section == 0) {
		// default account is shown in the header already
		return ms_list_size(linphone_core_get_proxy_config_list([LinphoneManager getLc])) - 1;
	} else {
		return [_sideMenuEntries count];
	}
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [[UITableViewCell alloc] init];
	if (indexPath.section == 0) {
		LinphoneProxyConfig *proxy =
			ms_list_nth_data(linphone_core_get_proxy_config_list([LinphoneManager getLc]), (int)indexPath.row);
		cell.textLabel.text = [NSString
			stringWithUTF8String:linphone_address_get_username(linphone_proxy_config_get_identity_address(proxy))];
		cell.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_F"]];
	} else {
		SideMenuEntry *entry = [_sideMenuEntries objectAtIndex:indexPath.row];
		cell.textLabel.text = entry->title;
	}
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
