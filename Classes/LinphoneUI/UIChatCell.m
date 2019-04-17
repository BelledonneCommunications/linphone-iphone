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
	[_imdmIcon setHidden:TRUE];
	return self;
}

#pragma mark - Property Funcitons

- (void)setChatRoom:(LinphoneChatRoom *)achat {
	chatRoom = achat;
	[self update];
}

#pragma mark -

- (NSString *)accessibilityValue {
	if (_chatContentLabel.text) {
		return [NSString stringWithFormat:@"%@, %@ (%li)", _addressLabel.text, _chatContentLabel.text,
										  (long)[_unreadCountLabel.text integerValue]];
	} else {
		return [NSString stringWithFormat:@"%@ (%li)", _addressLabel.text, (long)[_unreadCountLabel.text integerValue]];
	}
}

- (void)update {
	if (chatRoom == nil) {
		LOGW(@"Cannot update chat cell: null chat");
		return;
	}

	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(chatRoom);
	if (capabilities & LinphoneChatRoomCapabilitiesOneToOne) {
		bctbx_list_t *participants = linphone_chat_room_get_participants(chatRoom);
		LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
		const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(chatRoom);
		if (addr) {
			[ContactDisplay setDisplayNameLabel:_addressLabel forAddress:addr];
			[_avatarImage setImage:[FastAddressBook imageForAddress:addr] bordered:NO withRoundedRadius:YES];
		} else {
			_addressLabel.text = [NSString stringWithUTF8String:LINPHONE_DUMMY_SUBJECT];
		}
	} else {
		const char *subject = linphone_chat_room_get_subject(chatRoom);
		_addressLabel.text = [NSString stringWithUTF8String:subject ?: LINPHONE_DUMMY_SUBJECT];
		[_avatarImage setImage:[UIImage imageNamed:@"chat_group_avatar.png"] bordered:NO withRoundedRadius:YES];
	}
    // TODO update security image when security level changed
    [_securityImage setImage:[FastAddressBook imageForSecurityLevel:linphone_chat_room_get_security_level(chatRoom)]];

	_chatLatestTimeLabel.text = [LinphoneUtils timeToString:linphone_chat_room_get_last_update_time(chatRoom) withFormat:LinphoneDateChatList];

	LinphoneChatMessage *last_msg = linphone_chat_room_get_last_message_in_history(chatRoom);
	if (last_msg) {
        BOOL imdnInSnap = FALSE;
        if (imdnInSnap) {
            BOOL outgoing = linphone_chat_message_is_outgoing(last_msg);
            NSString *text = [UIChatBubbleTextCell TextMessageForChat:last_msg];
            if (outgoing) {
                // shorten long messages
                /*if ([text length] > 50)
                    text = [[text substringToIndex:50] stringByAppendingString:@"[...]"];*/
                _chatContentLabel.attributedText = nil;
                _chatContentLabel.text = text;
            } else {
                NSString *name = [FastAddressBook displayNameForAddress:linphone_chat_message_get_from_address(last_msg)];
                if ([name length] > 25) {
                    name = [[name substringToIndex:25] stringByAppendingString:@"[...]"];
                }
                CGFloat fontSize = _chatContentLabel.font.pointSize;
                UIFont *boldFont = [UIFont boldSystemFontOfSize:fontSize];
                NSMutableAttributedString *boldText = [[NSMutableAttributedString alloc] initWithString:name attributes:@{ NSFontAttributeName : boldFont }];
                text = [@" : " stringByAppendingString:text];
                //NSString *fullText = [name stringByAppendingString:text];
                /*if ([fullText length] > 50) {
                    text = [[text substringToIndex: (50 - [name length])] stringByAppendingString:@"[...]"];
                }*/
                [boldText appendAttributedString:[[NSAttributedString alloc] initWithString:text]];
                _chatContentLabel.text = nil;
                _chatContentLabel.attributedText = boldText;
            }

            
            LinphoneChatMessageState state = linphone_chat_message_get_state(last_msg);
            if (outgoing && (state == LinphoneChatMessageStateDeliveredToUser || state == LinphoneChatMessageStateDisplayed || state == LinphoneChatMessageStateNotDelivered || state == LinphoneChatMessageStateFileTransferError)) {
                [self displayImdmStatus:state];
                CGRect newFrame = _chatContentLabel.frame;
                newFrame.origin.x = 89;
                _chatContentLabel.frame = newFrame;
            } else {
                // We displace the message 20 pixels to the left
                [_imdmIcon setHidden:TRUE];
                CGRect newFrame = _chatContentLabel.frame;
                newFrame.origin.x = 69;
                _chatContentLabel.frame = newFrame;
            }
        } else {
            NSString *text = [[FastAddressBook displayNameForAddress:linphone_chat_message_get_from_address(last_msg)]
                              stringByAppendingFormat:@" : %@", [UIChatBubbleTextCell TextMessageForChat:last_msg]];
            // shorten long messages
            /*if ([text length] > 50)
                text = [[text substringToIndex:50] stringByAppendingString:@"[...]"];*/
            [_imdmIcon setHidden:TRUE];
            CGRect newFrame = _chatContentLabel.frame;
            newFrame.origin.x = 69;
            _chatContentLabel.frame = newFrame;
            _chatContentLabel.text = text;
        }
        
		linphone_chat_message_unref(last_msg);
	} else
		_chatContentLabel.text = nil;
    
	[self updateUnreadBadge];
}

- (void)updateUnreadBadge {
	int count = linphone_chat_room_get_unread_messages_count(chatRoom);
	_unreadCountLabel.text = [NSString stringWithFormat:@"%i", count];
	if (count > 0) {
		[_unreadCountView startAnimating:YES];
	} else {
		[_unreadCountView stopAnimating:YES];
	}
	UIFont *addressFont = (count <= 0) ? [UIFont systemFontOfSize:21] : [UIFont boldSystemFontOfSize:21];
	_addressLabel.font = addressFont;
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
		[_unreadCountView stopAnimating:animated];
	} else if (linphone_chat_room_get_unread_messages_count(chatRoom) > 0) {
		[_unreadCountView startAnimating:animated];
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


- (void)displayImdmStatus:(LinphoneChatMessageState)state {
    if (state == LinphoneChatMessageStateDeliveredToUser) {
        [_imdmIcon setImage:[UIImage imageNamed:@"chat_delivered"]];
        [_imdmIcon setHidden:FALSE];
    } else if (state == LinphoneChatMessageStateDisplayed) {
        [_imdmIcon setImage:[UIImage imageNamed:@"chat_read"]];
        [_imdmIcon setHidden:FALSE];
    } else if (state == LinphoneChatMessageStateNotDelivered || state == LinphoneChatMessageStateFileTransferError) {
        [_imdmIcon setImage:[UIImage imageNamed:@"chat_error"]];
        [_imdmIcon setHidden:FALSE];
    } else {
        [_imdmIcon setHidden:TRUE];
    }
}

@end
