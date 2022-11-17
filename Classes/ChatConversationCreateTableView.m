/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "linphoneapp-Swift.h"
#import "ChatConversationCreateTableView.h"
#import "UIChatCreateCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "UIChatCreateCollectionViewCell.h"

@interface ChatConversationCreateTableView ()

@property(nonatomic, strong) NSMutableArray *addresses;
@property(nonatomic, strong) NSMutableArray *phoneOrAddr;
@property(nonatomic, strong) NSMutableArray *addressesCached;
@property(readonly, nonatomic) NSMutableDictionary *ldapAndProvisioningContactAddressBookMap;

@end

@implementation ChatConversationCreateTableView

- (void)viewWillAppear:(BOOL)animated {
	if (!_ldapAndProvisioningContactAddressBookMap) {
		_ldapAndProvisioningContactAddressBookMap = [NSMutableDictionary dictionary];
	}
	[super viewWillAppear:animated];

	int y = _contactsGroup.count > 0
		? _collectionView.frame.origin.y + _collectionView.frame.size.height
		: _searchBar.frame.origin.y + _searchBar.frame.size.height;
	[UIView animateWithDuration:0
						  delay:0
						options:UIViewAnimationOptionCurveEaseOut
					 animations:^{
						 [self.tableView setFrame:CGRectMake(self.tableView.frame.origin.x,
															 y,
															 self.tableView.frame.size.width,
															 _waitView.frame.size.height - _waitView.frame.origin.y - y)];
						 }
					 completion:nil];

	_addresses = [[NSMutableArray alloc] initWithCapacity:LinphoneManager.instance.fastAddressBook.addressBookMap.allKeys.count];
	_phoneOrAddr = [[NSMutableArray alloc] initWithCapacity:LinphoneManager.instance.fastAddressBook.addressBookMap.allKeys.count];
	_addressesCached = [[NSMutableArray alloc] initWithCapacity:LinphoneManager.instance.fastAddressBook.addressBookMap.allKeys.count];
	
	[[NSNotificationCenter defaultCenter]
	 addObserver:self
	 selector:@selector(onChatMagicSearchStarted:)
	 name:kLinphoneMagicSearchStarted
	 object:nil];
	[[NSNotificationCenter defaultCenter]
	 addObserver:self
	 selector:@selector(onChatMagicSearchFinished:)
	 name:kLinphoneMagicSearchFinished
	 object:nil];
	
	if(_notFirstTime) {
		for(NSString *addr in _contactsGroup) {
			[_collectionView registerClass:UIChatCreateCollectionViewCell.class forCellWithReuseIdentifier:addr];
		}
		[self searchBar:_searchBar textDidChange:_searchBar.text];
		return;
	}
	_contactsGroup = [[NSMutableArray alloc] init];
	[_searchBar setText:@""];
	[self searchBar:_searchBar textDidChange:_searchBar.text];
	self.tableView.accessibilityIdentifier = @"Suggested addresses";
	
}

- (void) viewWillDisappear:(BOOL)animated {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	_notFirstTime = FALSE;
}

- (void)onChatMagicSearchStarted:(NSNotification *)k {
	_loadingView.hidden = FALSE;
}
- (void)onChatMagicSearchFinished:(NSNotification *)k {
	[self buildChatContactTable];
	_loadingView.hidden = TRUE;
}

- (void) buildChatContactTable {
	
	bctbx_list_t *results = [MagicSearchSingleton.instance getLastSearchResults];
	while (results) {
		
		LinphoneSearchResult *result = results->data;
		const LinphoneAddress *addr = linphone_search_result_get_address(result);
		const LinphoneFriend* friend = linphone_search_result_get_friend(result);
		const char *phoneNumber = linphone_search_result_get_phone_number(result);
		
		Contact *contact = nil;
		char *uri = nil;
		NSString *address = nil;
		if (addr) {
			uri = linphone_address_as_string_uri_only(addr);
			address = [NSString stringWithUTF8String:uri];
			contact = [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:[FastAddressBook normalizeSipURI:address use_prefix:[CallManager.instance applyInternationalPrefix]]];
			
			if (!contact && friend) {
				contact = [[Contact alloc] initWithFriend:friend];
				[contact setCreatedFromLdapOrProvisioning:TRUE];
				[_ldapAndProvisioningContactAddressBookMap setObject:contact forKey:address];
			}
		} else if (friend){
			if (!phoneNumber) {
				results = results->next;
				continue;
			}
			LinphoneAccount *account = linphone_core_get_default_account(LC);
			if (account) {
				const char *normalizedPhoneNumber = linphone_account_normalize_phone_number(account, phoneNumber);
				if (!normalizedPhoneNumber) {
					// get invalid phone number, continue
					results = results->next;
					continue;
				}
				addr = linphone_account_normalize_sip_uri(account, normalizedPhoneNumber);
				uri = linphone_address_as_string_uri_only(addr);
				address = [NSString stringWithUTF8String:uri];
				
				contact = [[Contact alloc] initWithFriend:friend];
				[contact setCreatedFromLdapOrProvisioning:TRUE];
				[_ldapAndProvisioningContactAddressBookMap setObject:contact forKey:address];
			}
		}
		
		if (!addr) {
			results = results->next;
			continue;
		}
		
		ms_free(uri);
		
		[_addresses addObject:address];
		[_phoneOrAddr addObject:phoneNumber ? [NSString stringWithUTF8String:phoneNumber] : address];
		[_addressesCached addObject:[NSString stringWithFormat:@"%d",linphone_search_result_get_capabilities(result)]];
		
		results = results->next;
	}
	[self.tableView reloadData];
	_reloadMagicSearch = FALSE;
}


- (void) loadData {
	[self reloadDataWithFilter:_searchBar.text];
}

- (void)reloadDataWithFilter:(NSString *)filter {
	[_addresses removeAllObjects];
	[_phoneOrAddr removeAllObjects];
	[_addressesCached removeAllObjects];
	[_ldapAndProvisioningContactAddressBookMap removeAllObjects];
	[self.tableView reloadData];
	
	_reloadMagicSearch = _reloadMagicSearch || [filter length]==0 || ![[MagicSearchSingleton.instance currentFilter] isEqualToString:filter];
	[MagicSearchSingleton.instance setCurrentFilter:filter];
	
	if (_reloadMagicSearch) {
		[MagicSearchSingleton.instance searchForContactsWithDomain:_allFilter ?  @"" : @"*" sourceFlags:LinphoneMagicSearchSourceAll clearCache:FALSE];
	} else {
		[self buildChatContactTable];
	}
}

#pragma mark - TableView methods

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return _addresses.count;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
	return 60.0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIChatCreateCell.class);
	UIChatCreateCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil)
		cell = [[UIChatCreateCell alloc] initWithIdentifier:kCellId];

	NSString *key = [_addresses objectAtIndex:indexPath.row];
	NSString *phoneOrAddr = [_phoneOrAddr objectAtIndex:indexPath.row];
	Contact *contact = [LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:[FastAddressBook normalizeSipURI:key use_prefix:[CallManager.instance applyInternationalPrefix]]];
	if (!contact) {
		contact = [_ldapAndProvisioningContactAddressBookMap objectForKey:key];
	}
	
	const LinphonePresenceModel *model = contact.friend ? linphone_friend_get_presence_model(contact.friend) : NULL;
	Boolean linphoneContact = [FastAddressBook contactHasValidSipDomain:contact]
		|| (model && linphone_presence_model_get_basic_status(model) == LinphonePresenceBasicStatusOpen);
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:key];
	if (!addr)
		return cell;
	
	cell.linphoneImage.hidden = [LinphoneManager.instance lpConfigBoolForKey:@"hide_linphone_contacts" inSection:@"app"] || !linphoneContact;
	cell.securityImage.hidden = !(model && linphone_presence_model_has_capability(model, LinphoneFriendCapabilityLimeX3dh));
	int capabilities = [[_addressesCached objectAtIndex:indexPath.row] intValue];
	BOOL greyCellForEncryptedChat = _isEncrypted ? capabilities > 1 : TRUE;
	BOOL greyCellForGroupChat = _isGroupChat ? capabilities > 0 : TRUE;
	cell.userInteractionEnabled =  cell.greyView.hidden = greyCellForEncryptedChat && greyCellForGroupChat;
	cell.displayNameLabel.text = [contact createdFromLdapOrProvisioning] ? [contact displayName] : [FastAddressBook displayNameForAddress:addr];
	char *str = linphone_address_as_string(addr);
	cell.addressLabel.text = linphoneContact ? [NSString stringWithUTF8String:str] : phoneOrAddr;
	ms_free(str);
	cell.selectedImage.hidden = ![_contactsGroup containsObject:cell.addressLabel.text];
	[cell.avatarImage setImage:[FastAddressBook imageForAddress:addr] bordered:NO withRoundedRadius:YES];
	cell.contentView.userInteractionEnabled = false;
	cell.contentView.backgroundColor  = UIColor.clearColor;
	cell.backgroundColor = UIColor.clearColor;
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	UIChatCreateCell *cell = [tableView cellForRowAtIndexPath:indexPath];
	if (!cell.userInteractionEnabled)
		return;
	
	LinphoneAccount *defaultAccount = linphone_core_get_default_account(LC);
	if (!(defaultAccount && linphone_account_params_get_conference_factory_uri(linphone_account_get_params(defaultAccount))) || !_isGroupChat) {
		LinphoneAddress *addr = linphone_address_new(cell.addressLabel.text.UTF8String);
		[PhoneMainView.instance getOrCreateOneToOneChatRoom:addr waitView:_waitView isEncrypted:_isEncrypted];
		if (!addr) {
			LOGE(@"Chat room could not be created on server, because null address.");
			[ChatConversationInfoView displayCreationError];
		} else {
			linphone_address_unref(addr);
		}
		return;
	}

	[tableView deselectRowAtIndexPath:indexPath animated:YES];
	NSInteger index = 0;
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
	if ([searchText isEqualToString:@""]) {
		[_searchBar resignFirstResponder];
	}
}

- (BOOL)searchBar:(UISearchBar *)searchBar shouldChangeTextInRange:(NSRange)range replacementText:(nonnull NSString *)text {
	return TRUE;
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
