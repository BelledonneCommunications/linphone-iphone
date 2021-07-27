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
	_senderNameWithTextContent.font = FT_FONT_RBT_BOLD_30;
	_senderNameWithTextContent.textColor = rgb(59,59,59);
	_senderNameWithoutTextContent.font = FT_FONT_RBT_BOLD_30;
	_senderNameWithoutTextContent.textColor = rgb(59,59,59);
	_textContent.font = FT_FONT_RBT_REG_30;
}


-(void) configureForMessage:(LinphoneChatMessage *)message withDimissBlock:(void (^)(void))dismissBlock hideDismiss:(BOOL)hideDismiss{
	if (!message) {
		_senderNameWithoutTextContent.text = NSLocalizedString(@"Original message was removed", nil);
		_senderNameWithoutTextContent.hidden = NO;
		_dismissButton.hidden = true;
		_senderNameWithTextContent.hidden = true;
		_dataContent.hidden = true;
		_textContent.hidden = true;
		return;
	}
	self.message = message;
	NSString *sender = linphone_chat_message_is_outgoing(message) ? [[FTContactsManager mySelf] displayname] : [FastAddressBook displayNameForAddress:linphone_chat_message_get_from_address(message)];
	_senderNameWithTextContent.text = sender;
	_senderNameWithoutTextContent.text = sender;
	const char * text = linphone_chat_message_get_utf8_text(message);
	if (text && strlen(text) == 0)
		text = nil;
	_senderNameWithoutTextContent.hidden = text != nil;
	_senderNameWithTextContent.hidden = _textContent.hidden = text == nil;
	_textContent.text = text ? [NSString stringWithUTF8String:text] : @"";
	_dismissButton.hidden = hideDismiss;
	_dismissAction = dismissBlock;
	[_dismissButton addTarget:self action:@selector(dismissClick) forControlEvents:UIControlEventTouchUpInside];
	self.view.backgroundColor = hideDismiss ? UIColor.whiteColor : linphone_chat_message_is_outgoing(message) ? OUTGOING_BG_COLOR  : INCOMING_BG_COLOR;
	_leftBar.backgroundColor = REPLY_LEFT_BAR_COLOR;
	_rightBar.backgroundColor = self.view.backgroundColor;
	CGRect r = self.view.frame ;
	r.size.width = self.view.superview.frame.size.width;
	self.view.frame = r;
	self.dataContent.image = [self firstDataContent];
	if (self.dataContent.image == nil) {
		r = _textContent.frame;
		r.size.width = self.view.frame.size.width - (hideDismiss ? 20 : 30);
	} else {
		r.size.width = self.view.frame.size.width -100;
	}
	_textContent.frame = r;
	
	if (self.dataContent.image == nil) {
		r = _senderNameWithTextContent.frame;
		r.size.width = self.view.frame.size.width - (hideDismiss ? 20 : 30);
	} else {
		r.size.width = self.view.frame.size.width -100;
	}
	_senderNameWithTextContent.frame = r;
}


-(UIImage *) firstDataContent {
	const bctbx_list_t *contents = linphone_chat_message_get_contents(_message);
	const char * text = linphone_chat_message_get_utf8_text(_message);
	if (text && bctbx_list_size(contents) == 1)
		return nil;

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
		return [UIChatBubbleTextCell getImageFromContent:content filePath:filePath];
	}
	return nil;
}

-(void) dismissClick {
	_dismissAction();
}

@end
