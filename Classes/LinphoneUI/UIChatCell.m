/* UIChatCell.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UIChatCell.h"
#import "PhoneMainView.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation UIChatCell

@synthesize avatarImage;
@synthesize addressLabel;
@synthesize chatContentLabel;
@synthesize unreadCountButton;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];

		// resize cell to match .nib size. It is needed when resized the cell to
		// correctly adapt its height too
		UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:0]);
		[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
		[self addSubview:sub];
	}
	return self;
}

#pragma mark - Property Funcitons

- (void)setChatRoom:(LinphoneChatRoom *)achat {
	chatRoom = achat;
	[self update];
}

#pragma mark -

- (NSString *)accessibilityValue {
	if (chatContentLabel.text) {
		return [NSString stringWithFormat:@"%@ - %@ (%li)", addressLabel.text, chatContentLabel.text,
										  (long)[unreadCountButton.titleLabel.text integerValue]];
	} else {
		return [NSString
			stringWithFormat:@"%@ (%li)", addressLabel.text, (long)[unreadCountButton.titleLabel.text integerValue]];
	}
}

- (void)update {
	if (chatRoom == nil) {
		LOGW(@"Cannot update chat cell: null chat");
		return;
	}
	const LinphoneAddress *addr = linphone_chat_room_get_peer_address(chatRoom);
	[ContactDisplay setDisplayNameLabel:addressLabel forAddress:addr];
	avatarImage.image =
		[FastAddressBook getContactImage:[FastAddressBook getContactWithLinphoneAddress:addr] thumbnail:NO];

	LinphoneChatMessage *last_message = linphone_chat_room_get_user_data(chatRoom);
	if (last_message) {
		const char *text = linphone_chat_message_get_text(last_message);
		const char *url = linphone_chat_message_get_external_body_url(last_message);
		const LinphoneContent *last_content = linphone_chat_message_get_file_transfer_information(last_message);
		// Last message was a file transfer (image) so display a picture...
		if (url || last_content) {
			chatContentLabel.text = @"🗻";
			// otherwise show beginning of the text message
		} else if (text) {
			NSString *message = [NSString stringWithUTF8String:text];
			// shorten long messages
			if ([message length] > 50)
				message = [[message substringToIndex:50] stringByAppendingString:@"[...]"];

			chatContentLabel.text = message;
		}

		_chatLatestTimeLabel.text = [LinphoneUtils timeToString:linphone_chat_message_get_time(last_message)
													  withStyle:NSDateFormatterShortStyle];
		_chatLatestTimeLabel.hidden = NO;
	} else {
		chatContentLabel.text = nil;
		_chatLatestTimeLabel.text = NSLocalizedString(@"Now", nil);
	}

	int count = linphone_chat_room_get_unread_messages_count(chatRoom);
	[unreadCountButton setTitle:[NSString stringWithFormat:@"%i", count] forState:UIControlStateNormal];
	unreadCountButton.hidden = (count <= 0);
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	if (animated) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
	}
	if (editing) {
		[unreadCountButton setAlpha:0.0f];
	} else {
		[unreadCountButton setAlpha:1.0f];
	}
	if (animated) {
		[UIView commitAnimations];
	}
}

#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)event {
	if (chatRoom != NULL) {
		UITableView *tableView = VIEW(ChatsListView).tableController.tableView;
		NSIndexPath *indexPath = [tableView indexPathForCell:self];
		[[tableView dataSource] tableView:tableView
					   commitEditingStyle:UITableViewCellEditingStyleDelete
						forRowAtIndexPath:indexPath];
	}
}

@end
