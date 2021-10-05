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

#import <Foundation/Foundation.h>
#import "UIChatContentView.h"
#import "ChatConversationView.h"
#import "PhoneMainView.h"

@implementation UIChatContentView

- (void)setContent:(LinphoneContent *)content message:(LinphoneChatMessage *)message {
	_content = content;
	_message = message;
	self.userInteractionEnabled = YES;

	if(!linphone_chat_message_is_outgoing(_message) && linphone_content_is_file_transfer(_content)) {
		// has not yet downloaded
		NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)] ;
		UIImage *image = [UIChatBubbleTextCell getImageFromFileName:name forReplyBubble:false];
		[self setImage:image];
		_downloadButton = [UIButton buttonWithType:UIButtonTypeCustom];
		[_downloadButton addTarget:self
				   action:@selector(onDownloadClick:)
		 forControlEvents:UIControlEventTouchUpInside];
		UIFont *boldFont = [UIFont systemFontOfSize:12];
		NSMutableParagraphStyle * paragraphStyle = [[NSMutableParagraphStyle alloc] init];
		paragraphStyle.alignment = NSTextAlignmentCenter;
		
		NSMutableAttributedString *boldText = [[NSMutableAttributedString alloc] initWithString:@"Download" attributes:@{ NSFontAttributeName : boldFont, NSParagraphStyleAttributeName:paragraphStyle,NSUnderlineStyleAttributeName: @(NSUnderlineStyleSingle) }];
		[_downloadButton setAttributedTitle:boldText forState:UIControlStateNormal];
		_downloadButton.frame = CGRectMake(0, 90, 120, 30);
		[self addSubview:_downloadButton];
	} else {
		if (_filePath == NULL) {
			NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
			_filePath = [LinphoneManager validFilePath:name];
		}
		UIImage *image = [UIChatBubbleTextCell getImageFromContent:content filePath:_filePath forReplyBubble:false];
		[self setImage:image];
		UITapGestureRecognizer *tapGestureRecognizer =	[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onMultiPartClick:)];
		tapGestureRecognizer.numberOfTapsRequired = 1;
		tapGestureRecognizer.enabled = YES;
		[self addGestureRecognizer:tapGestureRecognizer];
		self.userInteractionEnabled = true;
	}
}


-(IBAction)onMultiPartClick:(id)sender {
	ChatConversationView *view = VIEW(ChatConversationView);
	[view openFileWithURLs:_fileUrls index:_position];
}

-(IBAction)onDownloadClick:(id)sender {
	_downloadButton.enabled = NO;
	linphone_content_set_file_path(_content, [[LinphoneManager imagesDirectory] stringByAppendingPathComponent:[NSString stringWithUTF8String:linphone_content_get_name(_content)]].UTF8String);
	linphone_chat_message_download_content(_message, _content);
}

@end

