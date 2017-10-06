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
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_nextButton.enabled = FALSE;
	_tableController.tableView.frame = CGRectMake(_tableController.tableView.frame.origin.x,
												  _tableController.searchBar.frame.origin.y + _tableController.searchBar.frame.size.height,
												  _tableController.tableView.frame.size.width,
												  _tableController.tableView.frame.size.height + _collectionView.frame.size.height);
	[_collectionView reloadData];
	[self changeView:ContactsAll];
}

#pragma mark - searchBar delegate

- (IBAction)onBackClick:(id)sender {
	[_tableController.contactsDict removeAllObjects];
	[_tableController.contactsGroup removeAllObjects];
	[PhoneMainView.instance popToView:ChatsListView.compositeViewDescription];
}

- (IBAction)onNextClick:(id)sender {
	ChatConversationCreateConfirmView *view = VIEW(ChatConversationCreateConfirmView);
	view.contacts = _tableController.contactsDict;
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
