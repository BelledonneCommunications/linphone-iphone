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
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[_collectionView reloadData];
	[self changeView:ContactsAll];
}

#pragma mark - searchBar delegate

- (IBAction)onBackClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}

- (IBAction)onNextClick:(id)sender {
	/*NSString *uri;
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:[_contacts.allKeys objectAtIndex:indexPath.row]];
	if (addr) {
		uri = [NSString stringWithUTF8String:linphone_address_as_string_uri_only(addr)];
	} else {
		uri = [_contacts.allKeys objectAtIndex:indexPath.row];
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
	}*/
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
	NSString *name = _tableController.contactsGroup[indexPath.item];
	UIChatCreateCollectionViewCell *cell = (UIChatCreateCollectionViewCell *)[_collectionView dequeueReusableCellWithReuseIdentifier:name forIndexPath:indexPath];
	cell.controller = self;
	cell.uri = name;
	cell = [cell initWithName:_tableController.contactsDict[name]];
	return cell;
}

@end
