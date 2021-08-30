//
//  UIChatReplyBubbleView.h
//
//
//  Created by CD on 26/07/2021.
//


#import "UIChatReplyBubbleView.h"
#import "frogtrustapp-Swift.h"

@interface UIChatReplyBubbleView ()

@end

@implementation UIChatReplyBubbleView


- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	return [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
	self = [super initWithCoder:coder];
	return self;
}

-(void) viewDidLoad {
	_senderName.font = FT_FONT_RBT_BOLD_30;
	_senderName.textColor = rgb(59,59,59);
	_textContent.font = FT_FONT_RBT_REG_30;
	_contentCollection.dataSource = self;
	[_contentCollection registerClass:UICollectionViewCell.class forCellWithReuseIdentifier:@"dataContent"];

}


-(void) configureForMessage:(LinphoneChatMessage *)message withDimissBlock:(void (^)(void))dismissBlock hideDismiss:(BOOL)hideDismiss withClickBlock:(void (^)(void))clickBlock{
	if (!message) {
		_senderName.text = NSLocalizedString(@"Original message was removed", nil);
		_dismissButton.hidden = true;
		_contentCollection.hidden = true;
		_textContent.hidden = true;
		return;
	}
	self.message = message;
	self.dataContent = [self loadDataContent];
	NSString *sender = linphone_chat_message_is_outgoing(message) ? [[FTContactsManager mySelf] displayname] : [FastAddressBook displayNameForAddress:linphone_chat_message_get_from_address(message)];
	_senderName.text = sender;
	const char * text = linphone_chat_message_get_text_content(message);
	if (text && strlen(text) == 0)
		text = nil;
	_textContent.text = text ? [NSString stringWithUTF8String:text] : @"";
	_dismissButton.hidden = hideDismiss;
	_dismissAction = dismissBlock;
	_clickAction = clickBlock;
	if (hideDismiss) {
		UITapGestureRecognizer *singleFingerTap =
		  [[UITapGestureRecognizer alloc] initWithTarget:self
												  action:@selector(onClick)];
		[self.view addGestureRecognizer:singleFingerTap];
	}
	else
		[_dismissButton addTarget:self action:@selector(dismissClick) forControlEvents:UIControlEventTouchUpInside];

	self.view.backgroundColor = hideDismiss ? UIColor.whiteColor : linphone_chat_message_is_outgoing(message) ? OUTGOING_BG_COLOR  : INCOMING_BG_COLOR;
	_leftBar.backgroundColor = REPLY_LEFT_BAR_COLOR;
	_rightBar.backgroundColor = self.view.backgroundColor;
	
	
	// Resize frame -> text or content only = 100, 145 otherwise
	_contentCollection.hidden = self.dataContent.count == 0;
	
	CGRect r = self.view.frame ;
	r.size.width = self.view.superview.frame.size.width;
	self.view.frame = r;

	if (self.dataContent.count == 0) {
		CGRect r = _textContent.frame;
		r.origin.y = _contentCollection.frame.origin.y;
		r.size.height = 87;
		_textContent.frame = r;
	}
	
	if (text == nil) {
		CGRect r = _contentCollection.frame;
		r.origin.y = 40;
		_contentCollection.frame = r;
	}
}


-(NSArray *) loadDataContent {
	NSMutableArray *result = [[NSMutableArray alloc] init];
	const bctbx_list_t *contents = linphone_chat_message_get_contents(_message);
	const char * text = linphone_chat_message_get_utf8_text(_message);
	if (text && bctbx_list_size(contents) == 1)
		return result;

	for (const bctbx_list_t * it = contents; it != NULL; it=bctbx_list_next(it)){
		LinphoneContent *content = (LinphoneContent *)it->data;
		if (linphone_content_is_text(content))
			continue;
		NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
		NSMutableDictionary<NSString *, NSString *> *encrptedFilePaths = encrptedFilePaths = [LinphoneManager getMessageAppDataForKey:@"encryptedfiles" inMessage:_message];
		NSString *filePath = encrptedFilePaths ? [encrptedFilePaths valueForKey:name] : nil;
		if (filePath == NULL) {
			filePath = [LinphoneManager validFilePath:name];
		}
		[result addObject:[UIChatBubbleTextCell getImageFromContent:content filePath:filePath]];
	}
	return result;
}

-(void) dismissClick {
	_dismissAction();
}

-(void) onClick {
	_clickAction();
}

-(NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView {
	return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
	return self.dataContent.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
	UICollectionViewCell *cell =  [collectionView dequeueReusableCellWithReuseIdentifier:@"dataContent" forIndexPath:indexPath];
	UIImageView *img = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 60, 80)];
	img.image = [self.dataContent objectAtIndex:indexPath.row];
	[cell.contentView addSubview:img];
	return cell;
}




@end
