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

#import "UIChatBubbleTextCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>

@implementation UIChatBubbleTextCell

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		if ([identifier isEqualToString:NSStringFromClass(self.class)]) {
			NSArray *arrayOfViews =
				[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
			// resize cell to match .nib size. It is needed when resized the cell to
			// correctly adapt its height too
			UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:arrayOfViews.count - 1]);
			[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
			[self addSubview:sub];
		}
	}

	UITapGestureRecognizer *limeRecognizer =
	[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onLime)];
	limeRecognizer.numberOfTapsRequired = 1;
	//[_LIMEKO addGestureRecognizer:limeRecognizer];
	//_LIMEKO.userInteractionEnabled = YES;
	UITapGestureRecognizer *resendRecognizer =
	[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onResend)];
	resendRecognizer.numberOfTapsRequired = 1;
	[_bubbleView addGestureRecognizer:resendRecognizer];
	_imdmIcon.userInteractionEnabled = YES;
	UITapGestureRecognizer *resendRecognizer2 =
	[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onResend)];
	resendRecognizer2.numberOfTapsRequired = 1;
	//[_imdmLabel addGestureRecognizer:resendRecognizer2];
	//_imdmLabel.userInteractionEnabled = YES;

	self.contentView.userInteractionEnabled = NO;
	return self;
}

- (void)dealloc {
	[self setEvent:NULL vfsEnabled:_vfsEnabled];
	[self setChatMessageForCbs:NULL];
}

#pragma mark -

- (void)clearEncryptedFiles {
	if (_vfsEnabled) {
		NSMutableDictionary<NSString *, NSString *> *encrptedFilePaths = [LinphoneManager getMessageAppDataForKey:@"encryptedfiles" inMessage:_message];
		if ([encrptedFilePaths count] > 0) {
			for(NSString *path in [encrptedFilePaths allValues]) {
				[[NSFileManager defaultManager] removeItemAtPath:path error:NULL];
			}
			[LinphoneManager setValueInMessageAppData:NULL forKey:@"encryptedfiles" inMessage:_message];
			return;
		}

		NSString *filePath = [LinphoneManager getMessageAppDataForKey:@"encryptedfile" inMessage:_message];
		if (filePath) {
			if (![filePath isEqualToString:@""])
				[[NSFileManager defaultManager] removeItemAtPath:filePath error:NULL];
			[LinphoneManager setValueInMessageAppData:NULL forKey:@"encryptedfile" inMessage:_message];
		}
	}
}

- (void)setEvent:(LinphoneEventLog *)event vfsEnabled:(BOOL)enabled {
	if(!event)
		return;

	_event = event;
	_vfsEnabled = enabled;
	if (!(linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage)) {
		LOGE(@"Impossible to create a ChatBubbleText whit a non message event");
		return;
	}
	[self setChatMessageForCbs:linphone_event_log_get_chat_message(event)];
}

- (void)setChatMessageForCbs:(LinphoneChatMessage *)amessage {
	if (!amessage || amessage == _message) {
		return;
	}

	_message = amessage;
	linphone_chat_message_set_user_data(_message, (void *)CFBridgingRetain(self));
	LinphoneChatMessageCbs *cbs = linphone_chat_message_get_callbacks(_message);
	linphone_chat_message_cbs_set_msg_state_changed(cbs, message_status);
    linphone_chat_message_cbs_set_participant_imdn_state_changed(cbs, participant_imdn_status);
	linphone_chat_message_cbs_set_user_data(cbs, (void *)_event);
}

+ (NSString *)TextMessageForChat:(LinphoneChatMessage *)message {
	const char *url = linphone_chat_message_get_external_body_url(message);
	const LinphoneContent *last_content = linphone_chat_message_get_file_transfer_information(message);
	// Last message was a file transfer (image) so display a picture...
	if (url || last_content) {
        if (linphone_chat_message_get_text_content(message))
            return [NSString stringWithUTF8String:linphone_chat_message_get_text_content(message)];
		return @"🗻";
	} else {
        const char *text = linphone_chat_message_get_text_content(message) ?: "";
		return [NSString stringWithUTF8String:text] ?: [NSString stringWithCString:text encoding:NSASCIIStringEncoding]
														   ?: NSLocalizedString(@"(invalid string)", nil);
	}
}

+ (NSString *)ContactDateForChat:(LinphoneChatMessage *)message {
	const LinphoneAddress *address =
		linphone_chat_message_get_from_address(message)
			? linphone_chat_message_get_from_address(message)
			: linphone_chat_room_get_peer_address(linphone_chat_message_get_chat_room(message));
	return [NSString stringWithFormat:@"%@ - %@", [LinphoneUtils timeToString:linphone_chat_message_get_time(message)
																   withFormat:LinphoneDateChatBubble],
									  [FastAddressBook displayNameForAddress:address]];
}

- (NSString *)textMessage {
	return [self.class TextMessageForChat:_message];
}

- (void)update {
	if (_message == nil) {
		LOGW(@"Cannot update message room cell: null message");
		return;
	}

	if (_messageText && ![LinphoneManager getMessageAppDataForKey:@"localvideo" inMessage:_message]) {
		[_messageText setHidden:FALSE];
		/* We need to use an attributed string here so that data detector don't mess
		 * with the text style. See http://stackoverflow.com/a/20669356 */
		UIColor *color = [UIColor darkGrayColor];
		if (@available(iOS 13,*)) {
			color = [UIColor secondaryLabelColor];
		}

		NSAttributedString *attr_text =
			[[NSAttributedString alloc] initWithString:self.textMessage
											attributes:@{
												NSFontAttributeName : _messageText.font,
												NSForegroundColorAttributeName : color
											}];
		_messageText.attributedText = attr_text;
	}

	LinphoneChatMessageState state = linphone_chat_message_get_state(_message);
	BOOL outgoing = linphone_chat_message_is_outgoing(_message);
   
    
    _contactDateLabel.hidden = !_isFirst;
    if (outgoing) {
        _contactDateLabel.text = [LinphoneUtils timeToString:linphone_chat_message_get_time(_message)
                                                  withFormat:LinphoneDateChatBubble];
        _contactDateLabel.textAlignment = NSTextAlignmentRight;
        _avatarImage.hidden = TRUE;
        
    } else {
        [_avatarImage setImage:[FastAddressBook imageForAddress:linphone_chat_message_get_from_address(_message)]
                      bordered:NO
             withRoundedRadius:YES];
        _contactDateLabel.text = [self.class ContactDateForChat:_message];
        _contactDateLabel.textAlignment = NSTextAlignmentLeft;
        _avatarImage.hidden = !_isFirst;
    }
	
	// Not use [UIImage imageNamed], it takes too much time
	NSString *imageName = outgoing ? @"color_A.png" : @"color_D.png";
    _backgroundColorImage.image =
		[UIImage imageWithContentsOfFile:[NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] bundlePath],imageName]];
    
    // set maskedCorners
    if (@available(iOS 11.0, *)) {
        _backgroundColorImage.layer.cornerRadius = 10;
        if (outgoing) {
            _backgroundColorImage.layer.maskedCorners = kCALayerMinXMaxYCorner | kCALayerMinXMinYCorner;
            if (_isFirst)
                _backgroundColorImage.layer.maskedCorners = _backgroundColorImage.layer.maskedCorners | kCALayerMaxXMinYCorner;
            if (_isLast)
                _backgroundColorImage.layer.maskedCorners = _backgroundColorImage.layer.maskedCorners | kCALayerMaxXMaxYCorner;
        } else {
            _backgroundColorImage.layer.maskedCorners = kCALayerMaxXMinYCorner | kCALayerMaxXMaxYCorner;
            if (_isFirst)
                _backgroundColorImage.layer.maskedCorners = _backgroundColorImage.layer.maskedCorners | kCALayerMinXMinYCorner;
            if (_isLast)
                _backgroundColorImage.layer.maskedCorners = _backgroundColorImage.layer.maskedCorners | kCALayerMinXMaxYCorner;
        }
        _backgroundColorImage.layer.masksToBounds = YES;
    } else {
        // TODO it doesn't work for ios < 11.0
        UIRectCorner corner;
        if (outgoing) {
            corner = UIRectCornerTopLeft | UIRectCornerBottomLeft;
            if (_isFirst)
                corner = corner | UIRectCornerTopRight;
            if (_isLast)
                corner = corner | UIRectCornerBottomRight;
        } else {
            corner = UIRectCornerTopRight | UIRectCornerBottomRight;
            if (_isFirst)
                corner = corner | UIRectCornerTopLeft;
            if (_isLast)
                corner = corner | UIRectCornerBottomLeft;
        }
        UIBezierPath *maskPath = [UIBezierPath bezierPathWithRoundedRect:_backgroundColorImage.frame byRoundingCorners:corner cornerRadii:CGSizeMake(10,10)];
        CAShapeLayer *maskLayer = [[CAShapeLayer alloc] init];
        maskLayer.frame = _backgroundColorImage.frame;
        maskLayer.path = maskPath.CGPath;
        _backgroundColorImage.layer.mask = maskLayer;
    }
    
    // need space for dateLabel
    CGRect frame = _innerView.frame;
    frame.origin.y = _isFirst ? 20 : 0;
    _innerView.frame = frame;
	
	[_messageText setAccessibilityLabel:outgoing ? @"Outgoing message" : @"Incoming message"];
	if (outgoing &&
		(state == LinphoneChatMessageStateDeliveredToUser || state == LinphoneChatMessageStateDisplayed ||
		 state == LinphoneChatMessageStateNotDelivered || state == LinphoneChatMessageStateFileTransferError)) {
		[self displayImdmStatus:state];
	} else
		[self displayImdmStatus:LinphoneChatMessageStateInProgress];
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	_messageText.userInteractionEnabled = !editing;
	_resendRecognizer.enabled = !editing;
}

- (void)displayLIMEWarning {
	UIAlertController *errView =
		[UIAlertController alertControllerWithTitle:NSLocalizedString(@"LIME warning", nil)
											message:NSLocalizedString(@"This message is not encrypted.", nil)
									 preferredStyle:UIAlertControllerStyleAlert];

	UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction *action){
														  }];

	[errView addAction:defaultAction];
	[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
}

#pragma mark - Action Functions

- (void)onDelete {
	if (_message != NULL) {
		UITableView *tableView = VIEW(ChatConversationView).tableController.tableView;
		NSIndexPath *indexPath = [tableView indexPathForCell:self];
		[tableView.dataSource tableView:tableView
					 commitEditingStyle:UITableViewCellEditingStyleDelete
					  forRowAtIndexPath:indexPath];
	}
}

- (void)onLime {
	/*if (!_LIMEKO.hidden)
		[self displayLIMEWarning];*/
}

- (void)onResend {
	if (_message == nil || !linphone_chat_message_is_outgoing(_message))
		return;

	LinphoneChatMessageState state = linphone_chat_message_get_state(_message);
	if (state != LinphoneChatMessageStateNotDelivered && state != LinphoneChatMessageStateFileTransferError)
		return;

	const bctbx_list_t *contents = linphone_chat_message_get_contents(_message);
	BOOL multiParts = ((linphone_chat_message_get_text_content(self.message) != NULL) ? bctbx_list_size(contents) > 2 : bctbx_list_size(contents) > 1);
	if (multiParts) {
		FileContext *newfileContext = [[FileContext alloc] init];
		[newfileContext clear];
		NSMutableDictionary<NSString *, NSString *> *encrptedFilePaths = encrptedFilePaths = [LinphoneManager getMessageAppDataForKey:@"encryptedfiles" inMessage:_message];
		int i;
		const bctbx_list_t *it;
		for (it = contents, i=0; it != NULL; it=bctbx_list_next(it)){
			LinphoneContent *content = (LinphoneContent *)it->data;
			if (linphone_content_is_file_transfer(content) || linphone_content_is_file(content)){
				NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
				NSString *filePath = [encrptedFilePaths valueForKey:name];
				if (filePath == NULL) {
					filePath = [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:name];
				}
				[newfileContext addObject:[NSData dataWithContentsOfFile:filePath] name:name type:[NSString stringWithUTF8String:linphone_content_get_type(content)]];
			}
		}
		[self onDelete];
		dispatch_async(dispatch_get_main_queue(), ^ {
			const char *text = linphone_chat_message_get_text_content(_message);
			[_chatRoomDelegate resendMultiFiles:newfileContext message: text? [NSString stringWithUTF8String:text]: NULL];
		});
		return;
	}
	if (linphone_chat_message_get_file_transfer_information(_message) != NULL) {
		NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:_message];
		NSString *localVideo = [LinphoneManager getMessageAppDataForKey:@"localvideo" inMessage:_message];
		NSString *localFile = [LinphoneManager getMessageAppDataForKey:@"localfile" inMessage:_message];
		NSString *filePath = [LinphoneManager getMessageAppDataForKey:@"encryptedfile" inMessage:self.message];

		[self onDelete];
		dispatch_async(dispatch_get_main_queue(), ^ {
			NSData *data = NULL;
			if (filePath) {
				data = [NSData dataWithContentsOfFile:filePath];
			}
			const char *text = linphone_chat_message_get_text_content(_message);
			NSString *str = text ? [NSString stringWithUTF8String:text] : NULL;
			if (localImage) {
				[_chatRoomDelegate resendFile: (data?:[ChatConversationView getCacheFileData:localImage]) withName:localImage type:@"image" key:@"localimage" message:str];
			} else if (localVideo) {
				[_chatRoomDelegate resendFile:(data?:[ChatConversationView getCacheFileData:localVideo]) withName:localVideo type:@"video" key:@"localvideo" message:str];
			} else {
				[_chatRoomDelegate resendFile:(data?:[ChatConversationView getCacheFileData:localFile]) withName:localFile type:@"image" key:@"localfile" message:str];
			}
		});
	} else {
		[self onDelete];
		double delayInSeconds = 0.4;
		dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
		dispatch_after(popTime, dispatch_get_main_queue(), ^(void) {
			[_chatRoomDelegate resendChat:self.textMessage withExternalUrl:nil];
		});
	}
}
#pragma mark - State changed handling
static void message_status(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	LOGI(@"State for message [%p] changed to %s", msg, linphone_chat_message_state_to_string(state));
	LinphoneEventLog *event = (LinphoneEventLog *)linphone_chat_message_cbs_get_user_data(linphone_chat_message_get_callbacks(msg));
	ChatConversationView *view = VIEW(ChatConversationView);
	[view.tableController updateEventEntry:event];
	[view.tableController scrollToBottom:true];
}

static void participant_imdn_status(LinphoneChatMessage* msg, const LinphoneParticipantImdnState *state) {
    ChatConversationImdnView *imdnView = VIEW(ChatConversationImdnView);
    [imdnView updateImdnList];
}

- (void)displayImdmStatus:(LinphoneChatMessageState)state {
	NSString *imageName = nil;
	_notDelivered = FALSE;
	if (state == LinphoneChatMessageStateDeliveredToUser) {
		imageName = @"chat_delivered.png";
		[_imdmIcon setHidden:FALSE];
	} else if (state == LinphoneChatMessageStateDisplayed) {
		imageName = @"chat_read";
		[_imdmIcon setHidden:FALSE];
	} else if (state == LinphoneChatMessageStateNotDelivered || state == LinphoneChatMessageStateFileTransferError) {
		imageName = @"chat_error";
		[_imdmIcon setHidden:FALSE];
		_notDelivered = TRUE;
	} else {
		[_imdmIcon setHidden:TRUE];
	}
	[_imdmIcon setImage:[UIImage imageWithContentsOfFile:[NSString stringWithFormat:@"%@/%@",[[NSBundle mainBundle] bundlePath],imageName]]];
}

#pragma mark - Bubble size computing

+ (CGSize)computeBoundingBox:(NSString *)text size:(CGSize)size font:(UIFont *)font {
	if (!text || text.length == 0)
		return CGSizeMake(0, 0);

	return [text boundingRectWithSize:size
							  options:(NSStringDrawingUsesLineFragmentOrigin |
									   NSStringDrawingTruncatesLastVisibleLine | NSStringDrawingUsesFontLeading)
						   attributes:@{
							   NSFontAttributeName : font
						   }
							  context:nil].size;
}

static const CGFloat CELL_MIN_HEIGHT = 65.0f;
static const CGFloat CELL_MIN_WIDTH = 190.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 68 + 10.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 44;

+ (CGSize)ViewHeightForMessage:(LinphoneChatMessage *)chat withWidth:(int)width {
    return [self ViewHeightForMessageText:chat withWidth:width textForImdn:nil];
}

+ (CGSize)ViewHeightForFile:(int)width {
	CGSize fileSize = CGSizeMake(230, 50);
	CGSize size = [self getMediaMessageSizefromOriginalSize:fileSize withWidth:width];
	size.width = MAX(size.width + CELL_MESSAGE_X_MARGIN, CELL_MIN_WIDTH);
	size.height = MAX(size.height + CELL_MESSAGE_Y_MARGIN, CELL_MIN_HEIGHT);
	return size;
}

+ (UIImage *)getImageFromContent:(LinphoneContent *)content filePath:(NSString *)filePath; {
	NSString *type = [NSString stringWithUTF8String:linphone_content_get_type(content)];
	NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
	if (!filePath) {
		filePath = [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:name];
	}
	
	UIImage *image = nil;
	if ([type isEqualToString:@"video"]) {
		image = [UIChatBubbleTextCell getImageFromVideoUrl:[NSURL fileURLWithPath:filePath]];
	} else if ([type isEqualToString:@"image"]) {
		NSData* data = [NSData dataWithContentsOfFile:filePath];
		image = [[UIImage alloc] initWithData:data];
	}
	if (image) return image;
	UIImage *basicImage = [ChatConversationView getBasicImage];
	image = [ChatConversationView drawText:[NSString stringWithFormat:@"📎 %@",name] image:basicImage textSize:25];
	return image;
}


+ (CGSize)ViewHeightForMessageText:(LinphoneChatMessage *)chat withWidth:(int)width textForImdn:(NSString *)imdnText {
    NSString *messageText = [UIChatBubbleTextCell TextMessageForChat:chat];
    static UIFont *messageFont = nil;

	if (!messageFont) {
		UIChatBubbleTextCell *cell =
			[[UIChatBubbleTextCell alloc] initWithIdentifier:NSStringFromClass(UIChatBubbleTextCell.class)];
		messageFont = cell.messageText.font;
	}
    width -= CELL_IMAGE_X_MARGIN;
	CGSize size;
	const char *url = linphone_chat_message_get_external_body_url(chat);
    
    if (imdnText) {
        size = [self computeBoundingBox:imdnText
                                   size:CGSizeMake(width - 4, CGFLOAT_MAX)
                                   font:messageFont];
        size.width = MAX(size.width + CELL_MESSAGE_X_MARGIN, CELL_MIN_WIDTH);
        size.height = MAX(size.height + CELL_MESSAGE_Y_MARGIN + 50, CELL_MIN_HEIGHT);
        return size;
    }

	CGFloat imagesw=0;
	CGFloat imagesh=0;
	CGFloat max_imagesw=0;
	CGFloat max_imagesh=0;
	const bctbx_list_t *contents = linphone_chat_message_get_contents(chat);
	BOOL multiParts = ((linphone_chat_message_get_text_content(chat) != NULL) ? bctbx_list_size(contents) > 2 : bctbx_list_size(contents) > 1);
	if (multiParts) {
		const bctbx_list_t *it = contents;
		NSMutableDictionary<NSString *, NSString *> *encrptedFilePaths = [LinphoneManager getMessageAppDataForKey:@"encryptedfiles" inMessage:chat];

		for (it = contents; it != NULL; it=bctbx_list_next(it)){
			LinphoneContent *content = (LinphoneContent *)it->data;
			UIImage *image;
			if(!linphone_chat_message_is_outgoing(chat) && linphone_content_is_file_transfer(content)) {
				// not yet downloaded
				UIImage *basicImage = [ChatConversationView getBasicImage];
				NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)] ;
				image = [ChatConversationView drawText:name image:basicImage textSize:25];
			} else if (linphone_content_is_file_transfer(content) || linphone_content_is_file(content)) {
				NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
				NSString *filePath=[encrptedFilePaths valueForKey:name];
				if (filePath == NULL) {
					filePath = [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:name];
				}

				image = [UIChatBubbleTextCell getImageFromContent:content filePath:filePath];
			}
			if (image) {
				CGSize sSize = [self getMediaMessageSizefromOriginalSize:image.size withWidth:IMAGE_DEFAULT_WIDTH];
				imagesw += sSize.width;
				if (imagesw > width) {
					imagesw = sSize.width;
					max_imagesw = MAX(max_imagesw, imagesw);
					max_imagesh = imagesh;
					imagesh = sSize.height;
				} else {
					max_imagesw = MAX(max_imagesw, imagesw);
					imagesh = MAX(imagesh, sSize.height);
				}
			}
		}
		
		max_imagesh += imagesh;
		size = CGSizeMake(max_imagesw, max_imagesh);
		CGSize textSize = CGSizeMake(0, 0);
		if (![messageText isEqualToString:@"🗻"]) {
			textSize = [self computeBoundingBox:messageText
										   size:CGSizeMake(width - CELL_MESSAGE_X_MARGIN - 4, CGFLOAT_MAX)
										   font:messageFont];
			size.height += textSize.height;
		}
		
		// add size for message text
		size.height += textSize.height;
		size.width = MAX(textSize.width, size.width);
		size.width = MAX(size.width + CELL_MESSAGE_X_MARGIN, CELL_MIN_WIDTH);
		size.height = MAX(size.height + CELL_MESSAGE_Y_MARGIN, CELL_MIN_HEIGHT);
		return size;
	}


    LinphoneContent *fileContent = linphone_chat_message_get_file_transfer_information(chat);
    if (url == nil && fileContent == NULL) {
        size = [self computeBoundingBox:messageText
                                    size:CGSizeMake(width - CELL_MESSAGE_X_MARGIN - 4, CGFLOAT_MAX)
                                    font:messageFont];
    } else {
        NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:chat];
        NSString *localFile = [LinphoneManager getMessageAppDataForKey:@"localfile" inMessage:chat];
        NSString *localVideo = [LinphoneManager getMessageAppDataForKey:@"localvideo" inMessage:chat];
		NSString *filePath = [LinphoneManager getMessageAppDataForKey:@"encryptedfile" inMessage:chat];
		NSString *fileName = [NSString stringWithUTF8String:linphone_content_get_name(fileContent)];
        
        CGSize textSize = CGSizeMake(0, 0);
        if (![messageText isEqualToString:@"🗻"]) {
            textSize = [self computeBoundingBox:messageText
                                           size:CGSizeMake(width - CELL_MESSAGE_X_MARGIN - 4, CGFLOAT_MAX)
                                           font:messageFont];
            size.height += textSize.height;
        }

		CGSize originalImageSize = CGSizeMake(230, 50);
		if (!filePath) {
			filePath = [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:fileName];
		}
		if (localFile) {
			UIImage *image = nil;
			NSString *type = [NSString stringWithUTF8String:linphone_content_get_type(fileContent)];
			
			if ([type isEqualToString:@"video"]) {
				if ([[NSFileManager defaultManager] fileExistsAtPath: filePath]) {
					image = [self getImageFromVideoUrl:[NSURL fileURLWithPath:filePath]];
				} else {
					image = [self getImageFromVideoUrl:[VIEW(ChatConversationView) getICloudFileUrl:localFile]];
				}
			} else if ([localFile hasSuffix:@"JPG"] || [localFile hasSuffix:@"PNG"] || [localFile hasSuffix:@"jpg"] || [localFile hasSuffix:@"png"]) {
				if ([[NSFileManager defaultManager] fileExistsAtPath: filePath]) {
					NSData *data = [NSData dataWithContentsOfFile:filePath];
					image = [[UIImage alloc] initWithData:data];
				} else {
					NSData *data = [NSData dataWithContentsOfURL:[VIEW(ChatConversationView) getICloudFileUrl:localFile]];
					image = [[UIImage alloc] initWithData:data];
				}
			} else {
				return [self ViewHeightForFile:width];
			}

			originalImageSize = image.size;
		} else {
			if (!localImage && !localVideo) {
				//We are loading the image
				return CGSizeMake(CELL_MIN_WIDTH + CELL_MESSAGE_X_MARGIN, CELL_MIN_HEIGHT + CELL_MESSAGE_Y_MARGIN + textSize.height + 20);
			}

			if (localImage && [[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
				NSData* data = [NSData dataWithContentsOfFile:filePath];
				UIImage *image = [[UIImage alloc] initWithData:data];
				if (!image) {
					return [self ViewHeightForFile:width];
				}
				originalImageSize = image.size;
			} else if (localVideo && [[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
				UIImage *image = [UIChatBubbleTextCell getImageFromVideoUrl:[NSURL URLWithString:filePath]];
				if (!image) {
					return [self ViewHeightForFile:width];
				}
				originalImageSize = image.size;
			} else {
				// support previous versions
				PHFetchResult<PHAsset *> *assets;
				if(localImage)
					assets = [LinphoneManager getPHAssets:localImage];
				else
					assets = [PHAsset fetchAssetsWithLocalIdentifiers:[NSArray arrayWithObject:localVideo] options:nil];

				if (![assets firstObject]) {
					return CGSizeMake(CELL_MIN_WIDTH, CELL_MIN_WIDTH + CELL_MESSAGE_Y_MARGIN + textSize.height);
				} else {
					PHAsset *asset = [assets firstObject];
					originalImageSize = CGSizeMake([asset pixelWidth], [asset pixelHeight]);
				}
			}
		}
		size = [self getMediaMessageSizefromOriginalSize:originalImageSize withWidth:width];
		// add size for message text
		size.height += textSize.height;
		size.width = MAX(textSize.width, size.width);
	}

	size.width = MAX(size.width + CELL_MESSAGE_X_MARGIN, CELL_MIN_WIDTH);
	size.height = MAX(size.height + CELL_MESSAGE_Y_MARGIN, CELL_MIN_HEIGHT);
	return size;
}

+ (CGSize)ViewSizeForMessage:(LinphoneChatMessage *)chat withWidth:(int)width {
	static UIFont *dateFont = nil;
	static CGSize dateViewSize;

	if (!dateFont) {
		UIChatBubbleTextCell *cell =
			[[UIChatBubbleTextCell alloc] initWithIdentifier:NSStringFromClass(UIChatBubbleTextCell.class)];
		dateFont = cell.contactDateLabel.font;
		dateViewSize = cell.contactDateLabel.frame.size;
		dateViewSize.width = CGFLOAT_MAX;
	}

	CGSize messageSize = [self ViewHeightForMessage:chat withWidth:width];
	CGSize dateSize = [self computeBoundingBox:[self ContactDateForChat:chat] size:dateViewSize font:dateFont];
	messageSize.width = MAX(MAX(messageSize.width, MIN(dateSize.width + CELL_MESSAGE_X_MARGIN, width)), CELL_MIN_WIDTH);
    messageSize.width = MAX(MAX(messageSize.width, MIN(CELL_MESSAGE_X_MARGIN, width)), CELL_MIN_WIDTH);

	return messageSize;
}

+ (UIImage *)getImageFromVideoUrl:(NSURL *)url {
    AVURLAsset* asset = [AVURLAsset URLAssetWithURL:url options:nil];
    AVAssetImageGenerator* generator = [AVAssetImageGenerator assetImageGeneratorWithAsset:asset];
    generator.appliesPreferredTrackTransform = YES;
    return [UIImage imageWithCGImage:[generator copyCGImageAtTime:CMTimeMake(0, 1) actualTime:nil error:nil]];
}

- (void)layoutSubviews {
	[super layoutSubviews];
	if (_message != nil) {
		UITableView *tableView = VIEW(ChatConversationView).tableController.tableView;
		BOOL is_outgoing = linphone_chat_message_is_outgoing(_message);
		CGRect bubbleFrame = _bubbleView.frame;
		int available_width = self.frame.size.width;
		int origin_x;

		bubbleFrame.size = [self.class ViewSizeForMessage:_message withWidth:available_width];

		if (tableView.isEditing) {
			origin_x = 0;
		} else {
			origin_x = (is_outgoing ? self.frame.size.width - bubbleFrame.size.width : 0);
		}

		bubbleFrame.origin.x = origin_x;
		_bubbleView.frame = bubbleFrame;
	}
}


+ (CGSize)getMediaMessageSizefromOriginalSize:(CGSize)originalSize withWidth:(int)width {
    CGSize mediaSize = CGSizeMake(0, 0);
    int availableWidth = width;
    if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]) || IPAD) {
        availableWidth = availableWidth /1.7;
    }
    
    int newHeight = originalSize.height;
    float originalAspectRatio = originalSize.width / originalSize.height;
    // We resize in width and crop in height
    if (originalSize.width > availableWidth) {
        newHeight = availableWidth / originalAspectRatio;
    }
    
    if (newHeight > availableWidth) {
        newHeight = availableWidth;
        availableWidth = newHeight * originalAspectRatio;
    }
    mediaSize.height = newHeight;
    mediaSize.width = MIN(availableWidth, originalSize.width);
    return mediaSize;
}

@end
