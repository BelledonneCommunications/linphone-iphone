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
#import "linphoneapp-Swift.h"

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

-(void) unfragmentCompositeDescription {
	if (!IPAD)
		return;
	compositeDescription.isLeftFragment = true;
	compositeDescription.otherFragment = nil;
}

-(void) fragmentCompositeDescription {
	if (!IPAD)
		return;
	compositeDescription.otherFragment = IPAD ? NSStringFromClass(ChatsListView.class) : nil;
	compositeDescription.isLeftFragment = false;
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
	layout.estimatedItemSize =  UICollectionViewFlowLayoutAutomaticSize;
	_collectionController.collectionView = _collectionView;
	_collectionController = (ChatConversationCreateCollectionViewController *)[[UICollectionViewController alloc] initWithCollectionViewLayout:layout];
	_collectionView.dataSource = self;
	[_collectionView setCollectionViewLayout:layout];
	_tableController.collectionView = _collectionView;
	_tableController.controllerNextButton = _nextButton;
	_isForEditing = FALSE;
	_voipTitle.text = VoipTexts.call_action_participants_list;

}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
    [self viewUpdateEvent:nil];
    
    if (IPAD)
        [NSNotificationCenter.defaultCenter addObserver:self
                                               selector:@selector(viewUpdateEvent:)
                                                   name:kLinphoneChatCreateViewChange
                                                 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
																				 selector:@selector(displayModeChanged)
																						 name:kDisplayModeChanged
																					 object:nil];
	LinphoneAccount *defaultAccount = linphone_core_get_default_account(LC);
	_chiffreOptionView.hidden = !(defaultAccount && linphone_account_params_get_conference_factory_uri(linphone_account_get_params(defaultAccount)));
	if ([LinphoneManager.instance lpConfigBoolForKey:@"hide_linphone_contacts" inSection:@"app"]) {
		self.linphoneButton.hidden = TRUE;
		self.selectedButtonImage.hidden = TRUE;
		CGRect frame = _allButton.frame;
		frame.origin.x = _linphoneButton.frame.origin.x;
		_allButton.frame = frame;

	}
	
	if ([LinphoneManager.instance lpConfigBoolForKey:@"force_lime_chat_rooms"]) {
		_chiffreOptionView.hidden = true;
		_isEncrypted = true;
		_tableController.isEncrypted = true;
		_allButton.hidden = true;
	}
	
	if (_isForVoipConference) {
		_switchView.hidden = true;
		_chiffreOptionView.hidden = true;
		_voipTitle.hidden = false;
		if (_isForOngoingVoipConference) {
			[_nextButton setImage:[UIImage imageNamed:@"valid_default"] forState:UIControlStateNormal];
		} else {
			[_nextButton setImage:[UIImage imageNamed:@"next_default"] forState:UIControlStateNormal];
		}
	} else {
		_voipTitle.hidden = true;
		[_nextButton setImage:[UIImage imageNamed:@"next_default"] forState:UIControlStateNormal];
	}
	[self displayModeChanged];
}

- (void)displayModeChanged{
	[self.tableController.tableView reloadData];
	if (_isForVoipConference) {
		_topBar.backgroundColor = [VoipTheme.voipToolbarBackgroundColor get];
		self.view.backgroundColor = [VoipTheme.voipBackgroundBWColor get];
		_tableController.tableView.backgroundColor = [VoipTheme.voipBackgroundBWColor get];
		_tableController.searchBar.backgroundColor = [VoipTheme.voipBackgroundBWColor get];
		_tableController.collectionView.backgroundColor = [VoipTheme.voipBackgroundBWColor get];
	} else {
		_topBar.backgroundColor = UIColor.secondarySystemBackgroundColor;
		self.view.backgroundColor = [VoipTheme.backgroundWhiteBlack get];
		_tableController.tableView.backgroundColor = [VoipTheme.backgroundWhiteBlack get];
		_tableController.searchBar.backgroundColor = [VoipTheme.backgroundWhiteBlack get];
		_tableController.collectionView.backgroundColor = [VoipTheme.backgroundWhiteBlack get];
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
	_isEncrypted = [LinphoneManager.instance lpConfigBoolForKey:@"force_lime_chat_rooms"]; // false by default
    CGRect buttonFrame = _chiffreButton.frame;

	if (!_isEncrypted) {
		buttonFrame.origin.x = 2;
		[_chiffreImage setImage:[UIImage imageNamed:@"security_toogle_background_grey.png"]];
		_chiffreButton.frame = buttonFrame;
	}

	_waitView.hidden = YES;
	_backButton.hidden = IPAD && !(_isForVoipConference||_isForOngoingVoipConference);
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
	if (_isForVoipConference) {
		if (_isForOngoingVoipConference) {
			[PhoneMainView.instance popToView:VIEW(ConferenceCallView).compositeViewDescription];
			[ControlsViewModelBridge showParticipants];
		} else {
			[PhoneMainView.instance popToView:ConferenceSchedulingView.compositeViewDescription];
		}
	} else {
		if (_tableController.isForEditing)
			[PhoneMainView.instance popToView:ChatConversationInfoView.compositeViewDescription];
		else
			[PhoneMainView.instance popToView:ChatsListView.compositeViewDescription];
	}
}

- (IBAction)onNextClick:(id)sender {
	if (_isForVoipConference) {
		if (_isForOngoingVoipConference) {
			[PhoneMainView.instance popToView:VIEW(ConferenceCallView).compositeViewDescription];
			[ConferenceViewModelBridge updateParticipantsListWithAddresses:_tableController.contactsGroup];
		} else {
			[PhoneMainView.instance changeCurrentView:VIEW(ConferenceSchedulingSummaryView).compositeViewDescription];
			[VIEW(ConferenceSchedulingSummaryView) setParticipantsWithAddresses:_tableController.contactsGroup];
		}
	} else {
		ChatConversationInfoView *view = VIEW(ChatConversationInfoView);
		view.contacts = _tableController.contactsGroup;
		view.create = !_isForEditing;
    	view.encrypted = _isEncrypted;
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	}
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
	
	if (view == ContactsAll && !_allButton.selected) {
		frame.origin.x = _allButton.frame.origin.x;
		_allButton.selected = TRUE;
		_linphoneButton.selected = FALSE;
		_tableController.allFilter = TRUE;
		_tableController.reloadMagicSearch = TRUE;
		[_tableController loadData];
	} else if (view == ContactsLinphone && !_linphoneButton.selected) {
		frame.origin.x = _linphoneButton.frame.origin.x;
		_linphoneButton.selected = TRUE;
		_allButton.selected = FALSE;
		_tableController.allFilter = FALSE;
		_tableController.reloadMagicSearch = TRUE;
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

#pragma mark - UICollectionViewDataSource & Delegate
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
	[cell.nameLabel setText:[FastAddressBook displayNameForAddress:addr]];
	linphone_address_unref(addr);
	return cell;
}


@end
