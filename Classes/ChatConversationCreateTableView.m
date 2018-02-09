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
#import "UIChatCreateCollectionViewCell.h"

@interface ChatConversationCreateTableView ()

@property(nonatomic, strong) NSMutableDictionary *contacts;
@property(nonatomic, strong) NSDictionary *allContacts;
@property(nonatomic, strong) NSArray *sortedAddresses;
@end

@implementation ChatConversationCreateTableView

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_allContacts = [[NSMutableDictionary alloc] initWithDictionary:LinphoneManager.instance.fastAddressBook.addressBookMap];
	_sortedAddresses = [[LinphoneManager.instance.fastAddressBook.addressBookMap allKeys] sortedArrayUsingComparator:^NSComparisonResult(id a, id b) {
		Contact* first =  [_allContacts objectForKey:a];
		Contact* second =  [_allContacts objectForKey:b];
		if([[first.firstName lowercaseString] compare:[second.firstName lowercaseString]] == NSOrderedSame)
			return [[first.lastName lowercaseString] compare:[second.lastName lowercaseString]];
		else
			return [[first.firstName lowercaseString] compare:[second.firstName lowercaseString]];
	}];

	if(_notFirstTime) {
		for(NSString *addr in _contactsGroup) {
			[_collectionView registerClass:UIChatCreateCollectionViewCell.class forCellWithReuseIdentifier:addr];
		}
		return;
	}
	_contacts = [[NSMutableDictionary alloc] initWithCapacity:_sortedAddresses.count];
	_contactsGroup = [[NSMutableArray alloc] init];
	_allFilter = TRUE;

	[_searchBar setText:@""];
	[self searchBar:_searchBar textDidChange:_searchBar.text];
	self.tableView.accessibilityIdentifier = @"Suggested addresses";
	if (_contactsGroup.count > 0) {
		[UIView animateWithDuration:0
							  delay:0
							options:UIViewAnimationOptionCurveEaseOut
						 animations:^{
							 [self.tableView setFrame:CGRectMake(self.tableView.frame.origin.x,
															_collectionView.frame.origin.y + _collectionView.frame.size.height,
															self.tableView.frame.size.width,
															self.tableView.frame.size.height - _collectionView.frame.size.height)];

						 }
						 completion:nil];
	}
}

- (void) viewWillDisappear:(BOOL)animated {
	_notFirstTime = FALSE;
}

- (void) loadData {
	[self reloadDataWithFilter:_searchBar.text];
}

- (void)reloadDataWithFilter:(NSString *)filter {
	[_contacts removeAllObjects];

	for (NSString* key in _sortedAddresses) {
		Contact *contact = [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:key];
		NSString *name = [FastAddressBook displayNameForContact:contact];
		Boolean linphoneContact = [FastAddressBook contactHasValidSipDomain:contact]
			|| (contact.friend && linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(contact.friend)) == LinphonePresenceBasicStatusOpen);
		BOOL add = _allFilter || linphoneContact;

		if (((filter.length == 0)
				 || ([name.lowercaseString containsSubstring:filter.lowercaseString])
				 || ([key.lowercaseString containsSubstring:filter.lowercaseString]))
			&& add)
			[_contacts setObject:name forKey:key];
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

	if (nsuri.length > 0 && [_contacts valueForKey:nsuri] == nil) {
		[_contacts setObject:filter forKey:nsuri];
	}

	[self.tableView reloadData];
}

#pragma mark - TableView methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return _contacts.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIChatCreateCell.class);
	UIChatCreateCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil)
		cell = [[UIChatCreateCell alloc] initWithIdentifier:kCellId];

	NSString *key = [_contacts.allKeys objectAtIndex:indexPath.row];
	Contact *contact = [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:key];
	Boolean linphoneContact = [FastAddressBook contactHasValidSipDomain:contact]
		|| (contact.friend && linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(contact.friend)) == LinphonePresenceBasicStatusOpen);
	cell.linphoneImage.hidden = !linphoneContact;
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:key];
	cell.displayNameLabel.text = [FastAddressBook displayNameForAddress:addr];
	cell.addressLabel.text = [NSString stringWithUTF8String:linphone_address_as_string(addr)];

	cell.selectedImage.hidden = ![_contactsGroup containsObject:cell.addressLabel.text];
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	UIChatCreateCell *cell = [tableView cellForRowAtIndexPath:indexPath];
	NSInteger index = 0;
	_searchBar.text = @"";
	[self searchBar:_searchBar textDidChange:@""];
	if(cell.selectedImage.hidden) {
		if(![_contactsGroup containsObject:cell.addressLabel.text]) {
			[_contactsGroup addObject:cell.addressLabel.text];
			[_collectionView registerClass:UIChatCreateCollectionViewCell.class forCellWithReuseIdentifier:cell.addressLabel.text];
		}
	} else if([_contactsGroup containsObject:cell.addressLabel.text]) {
		index = (NSInteger)[_contactsGroup indexOfObject:cell.addressLabel.text];
		[_contactsGroup removeObject:cell.addressLabel.text];
		if(index == _contactsGroup.count)
			index = index-1;
	}
	cell.selectedImage.hidden = !cell.selectedImage.hidden;
	_controllerNextButton.enabled = (_contactsGroup.count > 0) || _isForEditing;
	if (_contactsGroup.count > 1 || (_contactsGroup.count == 1 && cell.selectedImage.hidden)) {
		[UIView animateWithDuration:0.2
							  delay:0
							options:UIViewAnimationOptionCurveEaseOut
						 animations:^{
							 [tableView setFrame:CGRectMake(tableView.frame.origin.x,
															_collectionView.frame.origin.y + _collectionView.frame.size.height,
															tableView.frame.size.width,
															tableView.frame.size.height)];

						 }
						 completion:nil];
	} else if (_contactsGroup.count == 1 && !cell.selectedImage.hidden) {
		[UIView animateWithDuration:0.2
							  delay:0
							options:UIViewAnimationOptionCurveEaseOut
						 animations:^{
							 [tableView setFrame:CGRectMake(tableView.frame.origin.x,
															_collectionView.frame.origin.y + _collectionView.frame.size.height,
															tableView.frame.size.width,
															tableView.frame.size.height - _collectionView.frame.size.height)];

						 }
						 completion:nil];
	} else {
		[UIView animateWithDuration:0.2
							  delay:0
							options:UIViewAnimationOptionCurveEaseOut
						 animations:^{
							 [tableView setFrame:CGRectMake(tableView.frame.origin.x,
															_searchBar.frame.origin.y + _searchBar.frame.size.height,
															tableView.frame.size.width,
															tableView.frame.size.height + _collectionView.frame.size.height)];
						 }
						 completion:nil];
	}
	[_collectionView reloadData];
	if (!cell.selectedImage.hidden) {
		index = _contactsGroup.count - 1;
	}

	dispatch_async(dispatch_get_main_queue(), ^{
		if(index > 0) {
			NSIndexPath *path = [NSIndexPath indexPathForItem:index inSection:0];
			[_collectionView scrollToItemAtIndexPath:path
									atScrollPosition:(UICollectionViewScrollPositionCenteredHorizontally | UICollectionViewScrollPositionCenteredVertically)
											animated:YES];
		}
	});
}

#pragma mark - Searchbar delegates

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
	searchBar.showsCancelButton = (searchText.length > 0);
	[self reloadDataWithFilter:searchText];
	if ([searchText isEqualToString:@""])
		[_searchBar resignFirstResponder];
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
