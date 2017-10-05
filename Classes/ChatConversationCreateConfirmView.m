//
//  ChatConversationCreateConfirmView.m
//  linphone
//
//  Created by REIS Benjamin on 04/10/2017.
//

#import "ChatConversationCreateConfirmView.h"
#import "PhoneMainView.h"
#import "UIChatCreateConfirmCollectionViewCell.h"

@implementation ChatConversationCreateConfirmView

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
	_nameField.delegate = self;
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
								   initWithTarget:self
								   action:@selector(dismissKeyboards)];
	tap.delegate = self;
	[self.view addGestureRecognizer:tap];
}

- (void)dismissKeyboards {
	if ([_nameField isFirstResponder]) {
		[_nameField resignFirstResponder];
	}
}

- (IBAction)onBackClick:(id)sender {
	[PhoneMainView.instance popToView:ChatConversationCreateView.compositeViewDescription];
}

- (IBAction)onValidateClick:(id)sender {
}

#pragma mark - UITextFieldDelegate

- (void)textFieldDidEndEditing:(UITextField *)textField {
	_validateButton.enabled = (textField.text.length > 0 && textField.text != nil && ![textField.text isEqual:@""]);
}

#pragma mark - UICollectionViewDataSource

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
	return _contacts.count;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
	return 1;
}

- (UIChatCreateConfirmCollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
	return NULL;
}

@end
