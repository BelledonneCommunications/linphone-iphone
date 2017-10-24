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
@end

@implementation ChatConversationCreateTableView

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_allContacts =
		[[NSDictionary alloc] initWithDictionary:LinphoneManager.instance.fastAddressBook.addressBookMap];
	if(_notFirstTime) {
		_notFirstTime = FALSE;
		for(NSString *addr in _contactsGroup) {
			[_collectionView registerClass:UIChatCreateCollectionViewCell.class forCellWithReuseIdentifier:addr];
		}
		return;
	}
	_contacts = [[NSMutableDictionary alloc] initWithCapacity:_allContacts.count];
	_contactsGroup = [[NSMutableArray alloc] init];
	_contactsDict = [[NSMutableDictionary alloc] init];
	_allFilter = TRUE;
	[_searchBar setText:@""];
	[self searchBar:_searchBar textDidChange:_searchBar.text];
	self.tableView.accessibilityIdentifier = @"Suggested addresses";
}

- (void) viewWillDisappear:(BOOL)animated {
	_notFirstTime = FALSE;
}

- (void) loadData {
	[self reloadDataWithFilter:_searchBar.text];
}

- (void)reloadDataWithFilter:(NSString *)filter {
	[_contacts removeAllObjects];

	[_allContacts enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL *stop) {
		NSString *address = (NSString *)key;
		NSString *name = [FastAddressBook displayNameForContact:value];
		Contact *contact = [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:address];
		Boolean linphoneContact = [FastAddressBook contactHasValidSipDomain:contact]
			|| (contact.friend && linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(contact.friend)) == LinphonePresenceBasicStatusOpen);
		BOOL add = _allFilter || linphoneContact;

		if (((filter.length == 0)
				 || ([name.lowercaseString containsSubstring:filter.lowercaseString])
				 || ([address.lowercaseString containsSubstring:filter.lowercaseString]))
			&& add) {
			_contacts[address] = name;
		}
	}];
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
		_contacts[nsuri] = filter;
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
	cell.displayNameLabel.text = [_contacts.allValues objectAtIndex:indexPath.row];
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:[_contacts.allKeys objectAtIndex:indexPath.row]];
	Contact *contact = [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:[_contacts.allKeys objectAtIndex:indexPath.row]];
	Boolean linphoneContact = [FastAddressBook contactHasValidSipDomain:contact]
		|| (contact.friend && linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(contact.friend)) == LinphonePresenceBasicStatusOpen);
	cell.linphoneImage.hidden = !linphoneContact;
	if (addr) {
		cell.addressLabel.text = [NSString stringWithUTF8String:linphone_address_as_string_uri_only(addr)];
	} else {
		cell.addressLabel.text = [_contacts.allKeys objectAtIndex:indexPath.row];
	}
	cell.selectedImage.hidden = ![_contactsGroup containsObject:cell.addressLabel.text];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	UIChatCreateCell *cell = [tableView cellForRowAtIndexPath:indexPath];
	NSInteger index = 0;
	if(cell.selectedImage.hidden) {
		if(![_contactsGroup containsObject:cell.addressLabel.text]) {
			[_contactsGroup addObject:cell.addressLabel.text];
			_contactsDict[cell.addressLabel.text] = cell.displayNameLabel.text;
			[_collectionView registerClass:UIChatCreateCollectionViewCell.class forCellWithReuseIdentifier:cell.addressLabel.text];
		}
	} else if([_contactsGroup containsObject:cell.addressLabel.text]) {
		index = (NSInteger)[_contactsGroup indexOfObject:cell.addressLabel.text];
		[_contactsGroup removeObject:cell.addressLabel.text];
		if(index == _contactsGroup.count) index = index-1;
		[_contactsDict removeObjectForKey:cell.addressLabel.text];
	}
	cell.selectedImage.hidden = !cell.selectedImage.hidden;
	_controllerNextButton.enabled = (_contactsGroup.count > 0);
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
	if(!cell.selectedImage.hidden) {
		index = _contactsGroup.count-1;
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
