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

#import "ChatConversationCreateView.h"
#import "PhoneMainView.h"
#import "UIChatCreateCollectionViewCell.h"

@implementation ChatConversationCreateView

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:ChatsListView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
								   initWithTarget:self
								   action:@selector(dismissKeyboards)];
	tap.delegate = self;
	[self.view addGestureRecognizer:tap];
	UICollectionViewFlowLayout *layout = [[UICollectionViewFlowLayout alloc] init];
	layout.scrollDirection = UICollectionViewScrollDirectionHorizontal;
	layout.itemSize = CGSizeMake(100.0 , 50.0);
	_collectionController.collectionView = _collectionView;
	_collectionController = (ChatConversationCreateCollectionViewController *)[[UICollectionViewController alloc] initWithCollectionViewLayout:layout];
	_collectionView.dataSource = self;
	[_collectionView setCollectionViewLayout:layout];
	_tableController.collectionView = _collectionView;
	_tableController.controllerNextButton = _nextButton;
	_isForEditing = FALSE; 
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
    [self viewUpdateEvent:nil];
    
    if (IPAD)
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector(viewUpdateEvent:)
                                                   name:kLinphoneChatCreateViewChange
                                                 object:nil];
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
	_chiffreOptionView.hidden = !(cfg && linphone_proxy_config_get_conference_factory_uri(cfg));
	if ([LinphoneManager.instance lpConfigBoolForKey:@"hide_linphone_contacts" inSection:@"app"]) {
		self.linphoneButton.hidden = TRUE;
		self.selectedButtonImage.hidden = TRUE;
		CGRect frame = _allButton.frame;
		frame.origin.x = _linphoneButton.frame.origin.x;
		_allButton.frame = frame;
	}
}

- (void)viewUpdateEvent:(NSNotification *)notif {
    CGRect frame = _chiffreOptionView.frame;
    if (_isGroupChat) {
        _nextButton.hidden = FALSE;
        _switchView.hidden = TRUE;
        frame.origin.x = (self.view.frame.size.width -  _chiffreOptionView.frame.size.width)/2;
    } else {
        _nextButton.hidden = TRUE;
        _switchView.hidden = FALSE;
        frame.origin.x = self.view.frame.size.width * 0.192;
    }
    _chiffreOptionView.frame = frame;
    _isEncrypted = FALSE;
    CGRect buttonFrame = _chiffreButton.frame;
    _tableController.isEncrypted = _isEncrypted;

    // no encrypted by default
    buttonFrame.origin.x = 2;
    [_chiffreImage setImage:[UIImage imageNamed:@"security_toogle_background_grey.png"]];
    _chiffreButton.frame = buttonFrame;

	_waitView.hidden = YES;
	_backButton.hidden = IPAD;
	if(_tableController.contactsGroup.count == 0) {
		if (!_isForEditing)
			_nextButton.enabled = FALSE;

		_tableController.tableView.frame = CGRectMake(_tableController.tableView.frame.origin.x,
													  _tableController.searchBar.frame.origin.y + _tableController.searchBar.frame.size.height,
													  _tableController.tableView.frame.size.width,
													  _tableController.tableView.frame.size.height + _collectionView.frame.size.height);
	} else {
		_tableController.tableView.frame = CGRectMake(_tableController.tableView.frame.origin.x,
													  _collectionView.frame.origin.y + _collectionView.frame.size.height,
													  _tableController.tableView.frame.size.width,
													  _tableController.tableView.frame.size.height);
	}
	[_collectionView reloadData];
	_tableController.isForEditing = _isForEditing;
    _tableController.isGroupChat = _isGroupChat;
    _tableController.isEncrypted = _isEncrypted;
    [self changeView:ContactsLinphone];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if (IPAD)
        [NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Chat room functions

- (void)createChatRoom {
	NSString *addr = _tableController.contactsGroup[0];
	LinphoneAddress *remoteAddress = linphone_address_new(addr.UTF8String);
	[PhoneMainView.instance getOrCreateOneToOneChatRoom:remoteAddress waitView:_waitView isEncrypted:_isEncrypted];
	linphone_address_unref(remoteAddress);
}

#pragma mark - Buttons signals

- (IBAction)onBackClick:(id)sender {
	[_tableController.contactsGroup removeAllObjects];
	if (_tableController.isForEditing)
		[PhoneMainView.instance popToView:ChatConversationInfoView.compositeViewDescription];
	else
		[PhoneMainView.instance popToView:ChatsListView.compositeViewDescription];
}

- (IBAction)onNextClick:(id)sender {
	ChatConversationInfoView *view = VIEW(ChatConversationInfoView);
	view.contacts = _tableController.contactsGroup;
	view.create = !_isForEditing;
    view.encrypted = _isEncrypted;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onChiffreClick:(id)sender {
    CGRect frame = _chiffreButton.frame;
    _isEncrypted = !_isEncrypted;
    _tableController.isEncrypted = _isEncrypted;
    if (_isEncrypted) {
        // encrypted
        frame.origin.x = 20;
        [_chiffreImage setImage:[UIImage imageNamed:@"security_toogle_background_green.png"]];
    } else {
        // no encrypted
        frame.origin.x = 2;
        [_chiffreImage setImage:[UIImage imageNamed:@"security_toogle_background_grey.png"]];
    }
    _chiffreButton.frame = frame;
    [_tableController.tableView reloadData];
}

- (void)dismissKeyboards {
	if ([self.tableController.searchBar isFirstResponder])
		[self.tableController.searchBar resignFirstResponder];
}

#pragma mark - Contacts filter

typedef enum { ContactsAll, ContactsLinphone, ContactsMAX } ContactsCategory;

- (void)changeView:(ContactsCategory)view {
	CGRect frame = _selectedButtonImage.frame;
	if (_tableController.magicSearch)
		linphone_magic_search_reset_search_cache(_tableController.magicSearch);
	
	if (view == ContactsAll && !_allButton.selected) {
		frame.origin.x = _allButton.frame.origin.x;
		_allButton.selected = TRUE;
		_linphoneButton.selected = FALSE;
		_tableController.allFilter = TRUE;
		[_tableController loadData];
	} else if (view == ContactsLinphone && !_linphoneButton.selected) {
		frame.origin.x = _linphoneButton.frame.origin.x;
		_linphoneButton.selected = TRUE;
		_allButton.selected = FALSE;
		_tableController.allFilter = FALSE;
		[_tableController loadData];
	}
	_selectedButtonImage.frame = frame;
	if ([LinphoneManager.instance lpConfigBoolForKey:@"hide_linphone_contacts" inSection:@"app"]) {
		_allButton.selected = FALSE;
	}
}

- (IBAction)onAllClick:(id)event {
	[self changeView:ContactsAll];
}

- (IBAction)onLinphoneClick:(id)event {
	[self changeView:ContactsLinphone];
}

#pragma mark - GestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
	return NO;
}

#pragma mark - UICollectionViewDataSource
- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
	return _tableController.contactsGroup.count;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
	return 1;
}

- (UIChatCreateCollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
	NSString *uri = _tableController.contactsGroup[indexPath.item];
	UIChatCreateCollectionViewCell *cell = (UIChatCreateCollectionViewCell *)[_collectionView dequeueReusableCellWithReuseIdentifier:uri forIndexPath:indexPath];
	cell.controller = self;
	cell.uri = uri;
	LinphoneAddress *addr = NULL;
	LinphoneAccount *account = linphone_core_get_default_account(LC);
	if (account && linphone_account_is_phone_number(account, uri.UTF8String)) {
		char *phone = linphone_account_normalize_phone_number(account, uri.UTF8String);
		addr = linphone_account_normalize_sip_uri(account, phone);
		ms_free(phone);
	} else
		addr = linphone_address_new(uri.UTF8String);
	cell = [cell initWithName:[FastAddressBook displayNameForAddress:addr]];
	linphone_address_unref(addr);
	return cell;
}

@end
