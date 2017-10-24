//
//  ChatConversationInfoView.m
//  linphone
//
//  Created by REIS Benjamin on 23/10/2017.
//

#import <Foundation/Foundation.h>

#import "ChatConversationInfoView.h"
#import "PhoneMainView.h"
#import "UIChatConversationInfoTableViewCell.h"

@implementation ChatConversationInfoView

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
	_nameLabel.delegate = self;
	_tableView.dataSource = self;
	_tableView.delegate	= self;
	_admins = [[NSMutableArray alloc] init];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_nextButton.enabled = _nameLabel.text.length > 0 && _contacts.count > 0;
	[_tableView reloadData];
	_quitButton.hidden = _create;
}

#pragma mark - Buttons responders

- (IBAction)onNextClick:(id)sender {
}

- (IBAction)onBackClick:(id)sender {
	if(_create) {
		ChatConversationCreateView *view = VIEW(ChatConversationCreateView);
		view.tableController.contactsDict = _contacts;
		view.tableController.contactsGroup = [[_contacts allKeys] mutableCopy];
		view.tableController.notFirstTime = TRUE;
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	}
}

- (IBAction)onQuitClick:(id)sender {
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return _contacts.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIChatConversationInfoTableViewCell.class);
	UIChatConversationInfoTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatConversationInfoTableViewCell alloc] initWithIdentifier:kCellId];
	}
	cell.uri = _contacts.allKeys[indexPath.row];
	cell.nameLabel.text = [_contacts objectForKey:cell.uri];
	cell.controllerView = self;
	if(![_admins containsObject:cell.uri]) {
		cell.adminLabel.enabled	= FALSE;
		cell.adminImage.image = [UIImage imageNamed:@"check_unselected.png"];
	}
	return cell;
}

#pragma mark - searchBar delegate

- (void)dismissKeyboards {
	if ([_nameLabel isFirstResponder]) {
		[_nameLabel resignFirstResponder];
	}
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
	_nextButton.enabled = (!((string.length == 0 || string == nil || [string isEqual:@""]) && (textField.text.length == 1))
						   && _contacts.count > 0);
	return TRUE;
}

@end
