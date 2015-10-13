//
//  MyTableViewController.m
//  UISearchDisplayController
//
//  Created by Phillip Harris on 4/19/14.
//  Copyright (c) 2014 Phillip Harris. All rights reserved.
//

#import "ChatConversationCreateTableView.h"
#import "UIChatCreateCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@interface ChatConversationCreateTableView ()

@property(nonatomic, strong) NSMutableArray *contacts;
@property(nonatomic, strong) NSMutableArray *allContacts;
@end

@implementation ChatConversationCreateTableView

- (void)viewWillAppear:(BOOL)animated {
	self.contacts = [[NSMutableArray alloc] init];
	self.allContacts = [[NSMutableArray alloc] init];
	for (NSString *ref in LinphoneManager.instance.fastAddressBook.addressBookMap.allKeys) {
		[self.contacts addObject:ref];
		[self.allContacts addObject:ref];
	}
}

- (void)reloadDataWithFilter:(NSString *)filter {
	[_contacts removeAllObjects];
	if (filter.length == 0) {
		_contacts = [[NSMutableArray alloc] initWithArray:_allContacts];
	} else {
		for (NSString *contact in _allContacts) {
			if ([contact containsString:filter]) {
				[_contacts addObject:contact];
			}
		}
		// also add current entry, if not listed
		if (![_contacts containsObject:filter]) {
			[_contacts insertObject:filter atIndex:0];
		}
	}

	[self.tableView reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {

	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [self.contacts count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIChatCreateCell.class);
	UIChatCreateCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatCreateCell alloc] initWithIdentifier:kCellId];
	}

	cell.addressLabel.text = _contacts[indexPath.row];
	const LinphoneAddress *addr =
		linphone_core_interpret_url([LinphoneManager getLc], cell.addressLabel.text.UTF8String);
	[ContactDisplay setDisplayNameLabel:cell.displayNameLabel forAddress:addr];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri([LinphoneManager getLc],
																  ((NSString *)_contacts[indexPath.row]).UTF8String);
	ChatConversationView *view = VIEW(ChatConversationView);
	[view setChatRoom:room];
	[PhoneMainView.instance popCurrentView];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription push:TRUE];
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
	// display searchtext in UPPERCASE
	searchBar.showsCancelButton = (searchText.length > 0);
	[self reloadDataWithFilter:searchText];
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar {
	[searchBar setShowsCancelButton:FALSE animated:TRUE];
}

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar {
	[searchBar setShowsCancelButton:TRUE animated:TRUE];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
	[searchBar resignFirstResponder];
}

@end
