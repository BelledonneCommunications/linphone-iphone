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

@property(nonatomic, strong) NSMutableArray *addresses;
@property(nonatomic, strong) NSDictionary *allContacts;
@property(nonatomic, strong) NSMutableArray *contactsAddresses;
@property(nonatomic, strong) NSArray *sortedAddresses;
@end

@implementation ChatConversationCreateTableView

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	self.allContacts = [[NSMutableDictionary alloc] initWithDictionary:LinphoneManager.instance.fastAddressBook.addressBookMap];
	self.sortedAddresses = [[LinphoneManager.instance.fastAddressBook.addressBookMap allKeys] sortedArrayUsingComparator:^NSComparisonResult(id a, id b) {
		Contact* first =  [_allContacts objectForKey:a];
		Contact* second =  [_allContacts objectForKey:b];
		if([[first.firstName lowercaseString] compare:[second.firstName lowercaseString]] == NSOrderedSame)
			return [[first.lastName lowercaseString] compare:[second.lastName lowercaseString]];
		else
			return [[first.firstName lowercaseString] compare:[second.firstName lowercaseString]];
	}];

	self.contactsAddresses = [NSMutableArray array];
	_addresses = [[NSMutableArray alloc] initWithCapacity:_sortedAddresses.count];
	[_searchBar setText:@""];
	[self searchBar:_searchBar textDidChange:_searchBar.text];
	self.tableView.accessibilityIdentifier = @"Suggested addresses";
}

- (void)reloadDataWithFilter:(NSString *)filter {
	[_addresses removeAllObjects];
	[_contactsAddresses removeAllObjects];
	for (NSString* key in _sortedAddresses){
		NSString *address = (NSString *)key;
		NSString *name = [FastAddressBook displayNameForContact:[_allContacts objectForKey:key]];
		if ((filter.length == 0) || ([name.lowercaseString containsSubstring:filter.lowercaseString]) ||
			([address.lowercaseString containsSubstring:filter.lowercaseString])) {
			[_addresses addObject:key];
		}
	}
	// also add current entry, if not listed
	NSString *nsuri = filter.lowercaseString;
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:nsuri];
	if (addr) {
		char *uri = linphone_address_as_string(addr);
		nsuri = [NSString stringWithUTF8String:uri];
		ms_free(uri);
		linphone_address_destroy(addr);
	}

	if (nsuri.length > 0 && ![_addresses containsObject:nsuri])
		[_addresses addObject:nsuri];

	[self.tableView reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return _addresses.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIChatCreateCell.class);
	UIChatCreateCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatCreateCell alloc] initWithIdentifier:kCellId];

	NSString *key = [_addresses objectAtIndex:indexPath.row];
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:key];
	cell.displayNameLabel.text = [FastAddressBook displayNameForAddress:addr];
	cell.addressLabel.text = [NSString stringWithUTF8String:linphone_address_as_string(addr)];
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	NSString *uri;
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:[_contactsAddresses objectAtIndex:indexPath.row]];
	if (addr) {
		uri = [NSString stringWithUTF8String:linphone_address_as_string(addr)];
	} else {
		uri = [_addresses objectAtIndex:indexPath.row];
	}
	LinphoneChatRoom *room = linphone_core_get_chat_room_from_uri(LC, uri.UTF8String);
	if (!room) {
		[PhoneMainView.instance popCurrentView];
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Invalid address", nil)
																		 message:NSLocalizedString(@"Please specify the entire SIP address for the chat",
																									   nil)
																  preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
																style:UIAlertActionStyleDefault
																handler:^(UIAlertAction * action) {}];
		defaultAction.accessibilityLabel = @"OK";
		[errView addAction:defaultAction];
		[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[view setChatRoom:room];
		[PhoneMainView.instance popCurrentView];
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
		// refresh list of chatrooms if we are using fragment
		if (IPAD) {
			ChatsListView *listView = VIEW(ChatsListView);
			[listView.tableController loadData];
		}
	}
}

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
	searchBar.showsCancelButton = (searchText.length > 0);
	[self reloadDataWithFilter:searchText];
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar {
	[searchBar setShowsCancelButton:FALSE animated:TRUE];
}

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar {
	[searchBar setShowsCancelButton:(searchBar.text.length > 0) animated:TRUE];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
	[searchBar resignFirstResponder];
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
	[searchBar resignFirstResponder];
}
@end
