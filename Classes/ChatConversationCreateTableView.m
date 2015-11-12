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
	_searchBar.text = @"";
	[_searchBar becomeFirstResponder];

	self.tableView.accessibilityIdentifier = @"Suggested addresses";
}

- (void)reloadDataWithFilter:(NSString *)filter {
	[_contacts removeAllObjects];
	if (filter.length == 0) {
		_contacts = [[NSMutableArray alloc] initWithArray:_allContacts];
	} else {
		for (NSString *contact in _allContacts) {
			if ([contact.lowercaseString containsString:filter.lowercaseString]) {
				[_contacts addObject:contact];
			}
		}
		// also add current entry, if not listed
		LinphoneAddress *addr = linphone_core_interpret_url([LinphoneManager getLc], filter.UTF8String);
		NSString *nsuri = filter;
		if (addr) {
			char *uri = linphone_address_as_string(addr);
			nsuri = [NSString stringWithUTF8String:uri];
			ms_free(uri);
			linphone_address_destroy(addr);
		}
		if (![_contacts containsObject:nsuri]) {
			[_contacts insertObject:nsuri atIndex:0];
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

	const LinphoneAddress *addr =
		linphone_core_interpret_url([LinphoneManager getLc], ((NSString *)_contacts[indexPath.row]).UTF8String);
	if (addr) {
		char *uri = linphone_address_as_string(addr);
		cell.addressLabel.text = [NSString stringWithUTF8String:uri];
		ms_free(uri);
		[ContactDisplay setDisplayNameLabel:cell.displayNameLabel forAddress:addr];
	} else {
		cell.displayNameLabel.text = _contacts[indexPath.row];
		cell.addressLabel.text = NSLocalizedString(@"Invalid SIP address", nil);
	}
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri([LinphoneManager getLc],
																  ((NSString *)_contacts[indexPath.row]).UTF8String);
	if (!room) {
		[PhoneMainView.instance popCurrentView];
		UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Invalid address", nil)
														message:@"Please specify the entire SIP address for the chat"
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"OK", nil)
											  otherButtonTitles:nil];
		[alert show];
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[view setChatRoom:room];
		[PhoneMainView.instance popCurrentView];
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription push:TRUE];
	}
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
