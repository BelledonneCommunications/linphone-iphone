//
//  ChatConversationCreateViewViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/10/15.
//
//

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
	// if we use fragments, remove back button
	if (IPAD) {
		_backButton.hidden = YES;
	}
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
	if(_tableController.contactsGroup.count == 0) {
		if (!_isForEditing)
			_nextButton.enabled = FALSE;

		_tableController.tableView.frame = CGRectMake(_tableController.tableView.frame.origin.x,
													  _tableController.searchBar.frame.origin.y + _tableController.searchBar.frame.size.height,
													  _tableController.tableView.frame.size.width,
													  _tableController.tableView.frame.size.height + _collectionView.frame.size.height);
	}
	[_collectionView reloadData];
	[self changeView:ContactsAll];
	[_tableController loadData];
	_tableController.isForEditing = _isForEditing;
}

#pragma mark - searchBar delegate

- (IBAction)onBackClick:(id)sender {
	[_tableController.contactsDict removeAllObjects];
	[_tableController.contactsGroup removeAllObjects];
	[PhoneMainView.instance popToView:ChatsListView.compositeViewDescription];
}

#pragma mark - Chat room functions

void create_chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	switch (newState) {
		case LinphoneChatRoomStateCreated:
			LOGI(@"Chat room [%p] created on server.", cr);
			[(__bridge ChatConversationCreateView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_callbacks(cr)) onChatRoomCreated:cr];
			break;
		case LinphoneChatRoomStateCreationFailed:
			LOGE(@"Chat room [%p] could not be created on server.", cr);
			break;
		default:
			break;
	}
}

- (void)onChatRoomCreated:(LinphoneChatRoom *)cr {
	ChatConversationView *view = VIEW(ChatConversationView);
	view.chatRoom = cr;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (void)createChatRoom {
	LinphoneChatRoom *room = linphone_core_create_client_group_chat_room(LC, "dummy subject");
	NSString *addr = _tableController.contactsDict.allKeys[0];
	LinphoneAddress *linphoneAddress = linphone_address_new(addr.UTF8String);
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(room);
	linphone_chat_room_cbs_set_state_changed(cbs, create_chat_room_state_changed);
	linphone_chat_room_cbs_set_user_data(cbs, (__bridge void*)self);
	bctbx_list_t *addresses = bctbx_list_new((void *)linphoneAddress);
	linphone_chat_room_add_participants(room, addresses);
	bctbx_list_free_with_data(addresses, (void (*)(void *))linphone_address_unref);
}

#pragma mark - Buttons signals

- (IBAction)onNextClick:(id)sender {
	if (_tableController.contactsGroup.count == 1) {
		[self createChatRoom];
		return;
	}
	ChatConversationInfoView *view = VIEW(ChatConversationInfoView);
	if (!_isForEditing)
		view.contacts = _tableController.contactsDict;
	else {
		for (NSString *uri in _tableController.contactsDict) {
			[view.contacts setObject:[_tableController.contactsDict objectForKey:uri] forKey:uri];
		}
	}

	view.create = !_isForEditing;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (void)dismissKeyboards {
	if ([self.tableController.searchBar isFirstResponder]) {
		[self.tableController.searchBar resignFirstResponder];
	}
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
		[_tableController loadData];
	} else if (view == ContactsLinphone && !_linphoneButton.selected) {
		frame.origin.x = _linphoneButton.frame.origin.x;
		_linphoneButton.selected = TRUE;
		_allButton.selected = FALSE;
		_tableController.allFilter = FALSE;
		[_tableController loadData];
	}
	_selectedButtonImage.frame = frame;
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
	cell = [cell initWithName:_tableController.contactsDict[uri]];
	return cell;
}

@end