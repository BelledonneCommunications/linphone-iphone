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

#import "UIChatReplyBubbleView.h"
#import "linphoneapp-Swift.h"
#import "Utils.h"

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
	_contentCollection.dataSource = self;
	[_icsIcon setImageNamed:@"voip_meeting_schedule" tintColor:VoipTheme.voip_dark_gray];
	[_contentCollection registerClass:UICollectionViewCell.class forCellWithReuseIdentifier:@"dataContent"];
}


-(void) configureForMessage:(LinphoneChatMessage *)message withDimissBlock:(void (^)(void))dismissBlock hideDismiss:(BOOL)hideDismiss withClickBlock:(void (^)(void))clickBlock{
	if (!message) {
		_textContent.hidden = true;
		_dismissButton.hidden = true;
		_contentCollection.hidden = true;
		_senderName.hidden = true;
		_originalMessageGone.hidden = false;
		_icsIcon.hidden = true;
		return;
	}
	if (hideDismiss) {
		self.view.layer.cornerRadius = 10;
		self.view.layer.masksToBounds = true;
	}
	_originalMessageGone.hidden = true;
	self.message = message;
	self.dataContent = [self loadDataContent];
	BOOL isIcal = [ICSBubbleView isConferenceInvitationMessageWithCmessage:message];
	_icsIcon.hidden = !isIcal;

	NSString *sender =  [FastAddressBook displayNameForAddress:linphone_chat_message_get_from_address(message)];
	_senderName.text = sender;
	const char * text = isIcal ? [ICSBubbleView getSubjectFromContentWithCmessage:message].UTF8String : linphone_chat_message_get_text_content(message);
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

	
	self.view.backgroundColor = hideDismiss ? UIColor.whiteColor :(linphone_chat_message_is_outgoing(message) ? [[UIColor color:@"A"] colorWithAlphaComponent:0.2]  : [[UIColor color:@"D"] colorWithAlphaComponent:0.2]);
	_leftBar.backgroundColor = linphone_chat_message_is_outgoing(message) ? [UIColor color:@"A"]  : [UIColor color:@"D"];
	_leftBar.hidden = !hideDismiss;
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
		r.origin.y = 30;
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
		[result addObject:[UIChatBubbleTextCell getImageFromContent:content filePath:filePath forReplyBubble:true]];
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
	UIImageView *img = [[UIImageView alloc] initWithFrame:CGRectMake(0, 0, 60, 60)];
	img.image = [self.dataContent objectAtIndex:indexPath.row];
	[cell.contentView addSubview:img];
	return cell;
}




@end
