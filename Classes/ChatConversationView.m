/* ChatRoomViewController.m
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import <Photos/PHAssetChangeRequest.h>

#import "ChatConversationView.h"
#import "PhoneMainView.h"
#import "Utils.h"
#import "FileTransferDelegate.h"
#import "UIChatBubbleTextCell.h"
#import "DevicesListView.h"

@implementation ChatConversationView
static NSString* groupName = @"group.belledonne-communications.linphone";

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		scrollOnGrowingEnabled = TRUE;
		_chatRoom = NULL;
		_chatRoomCbs = NULL;
        securityDialog = NULL;
		imageQualities = [[OrderedDictionary alloc]
			initWithObjectsAndKeys:[NSNumber numberWithFloat:0.9], NSLocalizedString(@"Maximum", nil),
								   [NSNumber numberWithFloat:0.5], NSLocalizedString(@"Average", nil),
								   [NSNumber numberWithFloat:0.0], NSLocalizedString(@"Minimum", nil), nil];
		composingVisible = false;
	}
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

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


+ (void)markAsRead:(LinphoneChatRoom *)chatRoom {
	if (!chatRoom)
		return;

	linphone_chat_room_mark_as_read(chatRoom);
	if (IPAD) {
		ChatsListView *listView = VIEW(ChatsListView);
		[listView.tableController markCellAsRead:chatRoom];
	}
	[PhoneMainView.instance updateApplicationBadgeNumber];
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];
    _markAsRead = TRUE;
	// if we use fragments, remove back button
	if (IPAD) {
		_backButton.hidden = YES;
		_backButton.alpha = 0;
	}
    
    refreshControl = [[UIRefreshControl alloc]init];
    [refreshControl addTarget:self action:@selector(refreshData) forControlEvents:UIControlEventValueChanged];
    _tableController.refreshControl = refreshControl;
    
	_messageField.minNumberOfLines = 1;
	_messageField.maxNumberOfLines = IPAD ? 10 : 3;
	_messageField.delegate = self;
	_messageField.font = [UIFont systemFontOfSize:18.0f];
	_messageField.contentInset = UIEdgeInsetsMake(-15, 0, 0, 0);
	//	_messageField.internalTextView.scrollIndicatorInsets = UIEdgeInsetsMake(0, 0, 0, 10);
	[_tableController setChatRoomDelegate:self];
    [_imagesCollectionView registerClass:[UIImageViewDeletable class] forCellWithReuseIdentifier:NSStringFromClass([UIImageViewDeletable class])];
    [_imagesCollectionView setDataSource:self];
}

- (void)refreshData {
    [_tableController refreshData];
    [refreshControl endRefreshing];
    [_tableController loadData];
    [_tableController.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:_tableController.currentIndex inSection:0]
                          atScrollPosition:UITableViewScrollPositionTop
                                  animated:false];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(applicationWillEnterForeground:)
											   name:UIApplicationDidBecomeActiveNotification
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(keyboardWillShow:)
											   name:UIKeyboardWillShowNotification
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(keyboardWillHide:)
											   name:UIKeyboardWillHideNotification
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(onMessageChange:)
											   name:UITextViewTextDidChangeNotification
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(callUpdateEvent:)
											   name:kLinphoneCallUpdate
											 object:nil];
    
    if ([_imagesArray count] > 0) {
        [UIView animateWithDuration:0
                              delay:0
                            options:UIViewAnimationOptionBeginFromCurrentState
                         animations:^{
                             // resizing imagesView
                             CGRect imagesFrame = [_imagesView frame];
                             imagesFrame.origin.y = [_messageView frame].origin.y - 100;
                             imagesFrame.size.height = 100;
                             [_imagesView setFrame:imagesFrame];
                             // resizing chatTable
                             CGRect tableViewFrame = [_tableController.tableView frame];
                             tableViewFrame.size.height -= 100;
                             [_tableController.tableView setFrame:tableViewFrame];
                         }
                         completion:nil];
    }
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	if (_chatRoom && _chatRoomCbs) {
		linphone_chat_room_remove_callbacks(_chatRoom, _chatRoomCbs);
		_chatRoomCbs = NULL;
	}

	[_messageField resignFirstResponder];

	[self setComposingVisible:false withDelay:0]; // will hide the "user is composing.." message

	[NSNotificationCenter.defaultCenter removeObserver:self];
	PhoneMainView.instance.currentRoom = NULL;
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	composingVisible = !composingVisible;
	[self setComposingVisible:!composingVisible withDelay:0];

	// force offset recomputing
	[_messageField refreshHeight];
	[self configureForRoom:true];
	_backButton.hidden = _tableController.isEditing;
	[_tableController scrollToBottom:true];
    [self refreshImageDrawer];
}

#pragma mark -

- (void)configureForRoom:(BOOL)editing {
	if (!_chatRoom) {
		_chatView.hidden = YES;
		return;
	}

	if (!_chatRoomCbs) {
		_chatRoomCbs = linphone_factory_create_chat_room_cbs(linphone_factory_get());
		linphone_chat_room_cbs_set_state_changed(_chatRoomCbs, on_chat_room_state_changed);
		linphone_chat_room_cbs_set_subject_changed(_chatRoomCbs, on_chat_room_subject_changed);
		linphone_chat_room_cbs_set_participant_added(_chatRoomCbs, on_chat_room_participant_added);
		linphone_chat_room_cbs_set_participant_removed(_chatRoomCbs, on_chat_room_participant_removed);
		linphone_chat_room_cbs_set_participant_admin_status_changed(_chatRoomCbs, on_chat_room_participant_admin_status_changed);
		linphone_chat_room_cbs_set_chat_message_received(_chatRoomCbs, on_chat_room_chat_message_received);
		linphone_chat_room_cbs_set_chat_message_sent(_chatRoomCbs, on_chat_room_chat_message_sent);
		linphone_chat_room_cbs_set_is_composing_received(_chatRoomCbs, on_chat_room_is_composing_received);
		linphone_chat_room_cbs_set_conference_joined(_chatRoomCbs, on_chat_room_conference_joined);
		linphone_chat_room_cbs_set_conference_left(_chatRoomCbs, on_chat_room_conference_left);
		linphone_chat_room_cbs_set_user_data(_chatRoomCbs, (__bridge void*)self);
		linphone_chat_room_add_callbacks(_chatRoom, _chatRoomCbs);
	}

	[self updateSuperposedButtons];

	if (_tableController.isEditing)
		[_tableController setEditing:editing];

	BOOL fileSharingEnabled = linphone_core_get_file_transfer_server(LC) != NULL;
	[_pictureButton setEnabled:fileSharingEnabled];

	[self callUpdateEvent:nil];
	PhoneMainView.instance.currentRoom = _chatRoom;
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(_chatRoom);
	if (capabilities & LinphoneChatRoomCapabilitiesOneToOne) {
		bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
		LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
		const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(_chatRoom);
		[ContactDisplay setDisplayNameLabel:_addressLabel forAddress:addr];
	} else
		_addressLabel.text = [NSString stringWithUTF8String:linphone_chat_room_get_subject(_chatRoom) ?: LINPHONE_DUMMY_SUBJECT];

	[self updateParticipantLabel];

	_messageField.editable = !linphone_chat_room_has_been_left(_chatRoom);
	_pictureButton.enabled = !linphone_chat_room_has_been_left(_chatRoom);
	_messageView.userInteractionEnabled = !linphone_chat_room_has_been_left(_chatRoom);
	[_tableController setChatRoom:_chatRoom];

	_chatView.hidden = NO;
    UIImage *image = [FastAddressBook imageForSecurityLevel:linphone_chat_room_get_security_level(_chatRoom)];
    [_encryptedButton setImage:image forState:UIControlStateNormal];
    _encryptedButton.hidden = image ? FALSE : TRUE;
	[self update];
    [self shareFile];
}

- (void)shareFile {
    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:groupName];
    NSDictionary *dict = [defaults valueForKey:@"photoData"];
    NSDictionary *dictFile = [defaults valueForKey:@"icloudData"];
    NSDictionary *dictUrl = [defaults valueForKey:@"url"];
    if (dict) {
        //file shared from photo lib
        NSString *fileName = dict[@"url"];
        [_messageField setText:dict[@"message"]];
        NSString *key = [[fileName componentsSeparatedByString:@"."] firstObject];
        NSMutableDictionary <NSString *, PHAsset *> * assetDict = [LinphoneUtils photoAssetsDictionary];
        if ([fileName hasSuffix:@"JPG"] || [fileName hasSuffix:@"PNG"]) {
            UIImage *image = [[UIImage alloc] initWithData:dict[@"nsData"]];
            [self chooseImageQuality:image assetId:[[assetDict objectForKey:key] localIdentifier]];
        } else if ([fileName hasSuffix:@"MOV"]) {
            [self confirmShare:dict[@"nsData"] url:nil fileName:nil assetId:[[assetDict objectForKey:key] localIdentifier]];
        } else {
            LOGE(@"Unable to parse file %@",fileName);
        }
        
        [defaults removeObjectForKey:@"photoData"];
    } else if (dictFile) {
        NSString *fileName = dictFile[@"url"];
        [_messageField setText:dictFile[@"message"]];
        [self confirmShare:dictFile[@"nsData"] url:nil fileName:fileName assetId:nil];
        
        [defaults removeObjectForKey:@"icloudData"];
    } else if (dictUrl) {
        NSString *url = dictUrl[@"url"];
        [_messageField setText:dictUrl[@"message"]];
        [self confirmShare:nil url:url fileName:nil assetId:nil];
        
        [defaults removeObjectForKey:@"url"];
    }
}

- (void)applicationWillEnterForeground:(NSNotification *)notif {
	if (_chatRoom && _markAsRead)
		[ChatConversationView markAsRead:_chatRoom];

    _markAsRead = TRUE;
}

- (void)callUpdateEvent:(NSNotification *)notif {
    [self updateSuperposedButtons];
}

- (void)update {
	if (_chatRoom == NULL) {
		LOGW(@"Cannot update chat room header: null contact");
		return;
	}

	const LinphoneAddress *addr = linphone_chat_room_get_peer_address(_chatRoom);
	if (addr == NULL) {
		[PhoneMainView.instance popCurrentView];
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Invalid SIP address", nil)
																		 message:NSLocalizedString(@"Either configure a SIP proxy server from settings prior to send a "
																								   @"message or use a valid SIP address (I.E sip:john@example.net)",
																								   nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
		return;
	}
}

- (BOOL)sendMessage:(NSString *)message withExterlBodyUrl:(NSURL *)externalUrl withInternalURL:(NSURL *)internalUrl {
	if (_chatRoom == NULL) {
		LOGW(@"Cannot send message: No chatroom");
		return FALSE;
	}

	LinphoneChatMessage *msg = linphone_chat_room_create_message(_chatRoom, [message UTF8String]);
	if (externalUrl) {
		linphone_chat_message_set_external_body_url(msg, [[externalUrl absoluteString] UTF8String]);
	}

	if (internalUrl) {
        // internal url is saved in the appdata for display and later save
        [LinphoneManager setValueInMessageAppData:[internalUrl absoluteString] forKey:@"localimage" inMessage:msg];
	}

	// we must ref & unref message because in case of error, it will be destroy otherwise
	linphone_chat_room_send_chat_message(_chatRoom, msg);

	if (linphone_core_lime_enabled(LC) == LinphoneLimeMandatory && !linphone_chat_room_lime_available(_chatRoom)) {
		[LinphoneManager.instance alertLIME:_chatRoom];
	}

	return TRUE;
}

- (void)saveAndSend:(UIImage *)image assetId:(NSString *)phAssetId withQuality:(float)quality{
    
    [_imagesArray addObject:image];
    [_assetIdsArray addObject:phAssetId];
    [_qualitySettingsArray addObject:@(quality)];
    [self refreshImageDrawer];
}

- (void)chooseImageQuality:(UIImage *)image assetId:(NSString *)phAssetId {
	DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Choose the image size", nil)];
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
	  for (NSString *key in [imageQualities allKeys]) {
		  NSNumber *quality = [imageQualities objectForKey:key];
		  NSData *data = UIImageJPEGRepresentation(image, [quality floatValue]);
		  NSNumber *size = [NSNumber numberWithInteger:[data length]];
		  NSString *text = [NSString stringWithFormat:@"%@ (%@)", key, [size toHumanReadableSize]];
		  [sheet addButtonWithTitle:text
							  block:^() {
                                  [self saveAndSend:image assetId:phAssetId withQuality:[quality floatValue]];
							  }];
	  }
	  [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
	  dispatch_async(dispatch_get_main_queue(), ^{
		[sheet showInView:PhoneMainView.instance.view];
	  });
	});
}

- (void)confirmShare:(NSData *)data url:(NSString *)url fileName:(NSString *)fileName assetId:(NSString *)phAssetId {
    DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"", nil)];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [sheet addButtonWithTitle:@"send to this friend"
                                block:^() {
                                    if (![[self.messageField text] isEqualToString:@""]) {
                                        [self sendMessageInMessageField];
                                    }
                                    if (url)
                                        [self sendMessage:url withExterlBodyUrl:nil withInternalURL:nil];
                                    else if (fileName)
                                        [self startFileUpload:data withName:fileName];
                                    else
                                        [self startFileUpload:data assetId:phAssetId];
                                }];
     
        [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
        dispatch_async(dispatch_get_main_queue(), ^{
            [sheet showInView:PhoneMainView.instance.view];
        });
    });
}

- (void)setComposingVisible:(BOOL)visible withDelay:(CGFloat)delay {
	Boolean shouldAnimate = composingVisible != visible;
	CGRect keyboardFrame = [_messageView frame];
	CGRect newComposingFrame = [_composeIndicatorView frame];
	CGRect newTableFrame = [_tableController.tableView frame];

	if (visible) {
		// pull up the composing frame and shrink the table view
		newTableFrame.size.height -= newComposingFrame.size.height;
		newComposingFrame.origin.y = keyboardFrame.origin.y - newComposingFrame.size.height;
		const bctbx_list_t *addresses = linphone_chat_room_get_composing_addresses(_chatRoom);
		NSString *composingAddresses = @"";
		if (bctbx_list_size(addresses) == 1) {
			composingAddresses = [FastAddressBook displayNameForAddress:(LinphoneAddress *)addresses->data];
			_composeLabel.text = [NSString stringWithFormat:NSLocalizedString(@"%@ is writing...", nil), composingAddresses];
		} else {
			while (addresses) {
				if (![composingAddresses isEqualToString:@""])
					composingAddresses = [composingAddresses stringByAppendingString:@", "];
				composingAddresses = [composingAddresses stringByAppendingString:[FastAddressBook displayNameForAddress:(LinphoneAddress *)addresses->data]];
				addresses = addresses->next;
			}
			_composeLabel.text = [NSString stringWithFormat:NSLocalizedString(@"%@ are writing...", nil), composingAddresses];
		}
	} else {
		// pull down the composing frame and widen the tableview
		newTableFrame.size.height += newComposingFrame.size.height;
		newComposingFrame.origin.y = keyboardFrame.origin.y;
	}
	composingVisible = visible;
	if (!shouldAnimate)
		return;

	[UIView animateWithDuration:delay
					 animations:^{
						 _tableController.tableView.frame = newTableFrame;
						 _composeIndicatorView.frame = newComposingFrame;
					 }
					 completion:^(BOOL finished) {
						 [_tableController scrollToBottom:TRUE];
						 _composeIndicatorView.hidden = !visible;
					 }];
}

- (void)updateSuperposedButtons {
	[_backToCallButton update];
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(_chatRoom);
	_infoButton.hidden = ((capabilities & LinphoneChatRoomCapabilitiesOneToOne)
						|| !_backToCallButton.hidden
						|| _tableController.tableView.isEditing);
	_callButton.hidden = !_backToCallButton.hidden || !_infoButton.hidden || _tableController.tableView.isEditing;
}

- (void)updateParticipantLabel {
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(_chatRoom);
	if (capabilities & LinphoneChatRoomCapabilitiesOneToOne) {
		_particpantsLabel.hidden = TRUE;
	} else {
		_particpantsLabel.hidden = FALSE;
		bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
		_particpantsLabel.text = @"";
		while (participants) {
			LinphoneParticipant *participant = (LinphoneParticipant *)participants->data;
			if (![_particpantsLabel.text isEqualToString:@""])
				_particpantsLabel.text = [_particpantsLabel.text stringByAppendingString:@", "];

			_particpantsLabel.text = [_particpantsLabel.text stringByAppendingString:
									  [FastAddressBook displayNameForAddress:linphone_participant_get_address(participant)]];
			participants = participants->next;
		}
	}
}

- (void)sendMessageInMessageField {
    if ([self sendMessage:[_messageField text] withExterlBodyUrl:nil withInternalURL:nil]) {
        scrollOnGrowingEnabled = FALSE;
        [_messageField setText:@""];
        scrollOnGrowingEnabled = TRUE;
        [self onMessageChange:nil];
    }
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)growingTextViewShouldBeginEditing:(HPGrowingTextView *)growingTextView {
	if (_tableController.isEditing) {
		[_tableController setEditing:NO];
	}
	[_listTapGestureRecognizer setEnabled:TRUE];
	return TRUE;
}

- (BOOL)growingTextViewShouldEndEditing:(HPGrowingTextView *)growingTextView {
	[_listTapGestureRecognizer setEnabled:FALSE];
	return TRUE;
}

- (void)growingTextChanged:(HPGrowingTextView *)growingTextView text:(NSString *)text {
	if ([text length] > 0 && _chatRoom)
		linphone_chat_room_compose(_chatRoom);
}

- (void)growingTextView:(HPGrowingTextView *)growingTextView willChangeHeight:(float)height {
	int diff = height - growingTextView.bounds.size.height;

	if (diff != 0) {
		CGRect messageRect = [_messageView frame];
		messageRect.origin.y -= diff;
		messageRect.size.height += diff;
		[_messageView setFrame:messageRect];

        if ([_imagesArray count] > 0) {
            CGRect _imagesRect = [_imagesView frame];
            _imagesRect.origin.y -= diff;
            [_imagesView setFrame:_imagesRect];
        }
        
		// Always stay at bottom
		if (scrollOnGrowingEnabled) {
			CGRect tableFrame = [_tableController.view frame];
			CGPoint contentPt = [_tableController.tableView contentOffset];
			contentPt.y += diff;
			if (contentPt.y + tableFrame.size.height > _tableController.tableView.contentSize.height)
				contentPt.y += diff;
			[_tableController.tableView setContentOffset:contentPt animated:FALSE];
		}

		CGRect tableRect = [_tableController.view frame];
		tableRect.size.height -= diff;
		[_tableController.view setFrame:tableRect];

		// if we're showing the compose message, update it position
		if (![_composeLabel isHidden]) {
			CGRect frame = [_composeLabel frame];
			frame.origin.y -= diff;
			[_composeLabel setFrame:frame];
		}
	}
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
	[_tableController setChatRoom:NULL];
	[PhoneMainView.instance popToView:ChatsListView.compositeViewDescription];
}

- (IBAction)onEditClick:(id)event {
	[_tableController setEditing:![_tableController isEditing] animated:TRUE];
	[_messageField resignFirstResponder];
	[self updateSuperposedButtons];
}

- (IBAction)onSendClick:(id)event {
    if ([_imagesArray count] > 0) {
        int i = 0;
        for (i = 0; i < [_imagesArray count] - 1; ++i) {
            [self startImageUpload:[_imagesArray objectAtIndex:i] assetId:[_assetIdsArray objectAtIndex:i] withQuality:[_qualitySettingsArray objectAtIndex:i].floatValue];
        }
        BOOL isOneToOneChat = linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesOneToOne;
        if (isOneToOneChat) {
            [self startImageUpload:[_imagesArray objectAtIndex:i] assetId:[_assetIdsArray objectAtIndex:i] withQuality:[_qualitySettingsArray objectAtIndex:i].floatValue];
            if (![[self.messageField text] isEqualToString:@""]) {
                [self sendMessage:[_messageField text] withExterlBodyUrl:nil withInternalURL:nil];
            }
        } else {
            [self startImageUpload:[_imagesArray objectAtIndex:i] assetId:[_assetIdsArray objectAtIndex:i] withQuality:[_qualitySettingsArray objectAtIndex:i].floatValue andMessage:[self.messageField text]];
        }
        
        [self clearMessageView];
        return;
    }
    [self sendMessageInMessageField];
}

- (IBAction)onListTap:(id)sender {
	[_messageField resignFirstResponder];
}

- (IBAction)onDeleteClick:(id)sender {
	LOGI(@"onDeleteClick");
	NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete the selected messages?", nil)];
	[UIConfirmationDialog ShowWithMessage:msg
		cancelMessage:nil
		confirmMessage:nil
		onCancelClick:^() {
		  [self onEditionChangeClick:nil];
		}
		onConfirmationClick:^() {
		  [_tableController removeSelectionUsing:nil];
		  [_tableController loadData];
		  [self onEditionChangeClick:nil];
		}];
}

- (IBAction)onEditionChangeClick:(id)sender {
	_backButton.hidden = _tableController.isEditing;
	[self updateSuperposedButtons];
}

- (IBAction)onEncryptedDevicesClick:(id)sender {
    BOOL isOneToOne =  linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesOneToOne;
    NSString *message = NSLocalizedString(@"Instant messages are end-to-end encrypted in secured conversations. It is possible to upgrade the security level of a conversation by authenticating participants. To do so, call the contact and follow the authentification process.",nil);
    BOOL notAskAgain = [LinphoneManager.instance lpConfigBoolForKey:@"confirmation_dialog_before_sas_call_not_ask_again"];
    if (isOneToOne) {
        bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
        
        LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
        const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(_chatRoom);
        if (bctbx_list_size(linphone_participant_get_devices(firstParticipant)) == 1) {
            if (notAskAgain) {
                [LinphoneManager.instance doCallWithSas:addr isSas:TRUE];
            } else {
                securityDialog = [UIConfirmationDialog ShowWithMessage:message cancelMessage:NSLocalizedString(@"CANCEL", nil) confirmMessage:NSLocalizedString(@"CALL", nil) onCancelClick:^() {
                } onConfirmationClick:^() {
                    [LinphoneManager.instance doCallWithSas:addr isSas:TRUE];
                }];
                securityDialog.authView.hidden = FALSE;
            }
            return;
        }
    }

    if (notAskAgain) {
        [self goToDeviceListView];
    } else {
        securityDialog = [UIConfirmationDialog ShowWithMessage:message cancelMessage:NSLocalizedString(@"CANCEL", nil) confirmMessage:NSLocalizedString(@"OK", nil) onCancelClick:^() {
        } onConfirmationClick:^() {
            [self goToDeviceListView];
        }];
        securityDialog.authView.hidden = FALSE;
    }
}

- (IBAction)onCallClick:(id)sender {
	bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
	LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
	const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(_chatRoom);
	[LinphoneManager.instance call:addr];
}

- (IBAction)onListSwipe:(id)sender {
	[self onBackClick:sender];
}

- (IBAction)onMessageChange:(id)sender {
	if ([[_messageField text] length] > 0) {
		[_sendButton setEnabled:TRUE];
	} else {
		[_sendButton setEnabled:FALSE];
	}
}

- (IBAction)onPictureClick:(id)event {
	[_messageField resignFirstResponder];
	[ImagePickerView SelectImageFromDevice:self atPosition:_pictureButton inView:self.view];
}

- (IBAction)onInfoClick:(id)sender {
	NSMutableArray *contactsArray = [[NSMutableArray alloc] init];
	NSMutableArray *admins = [[NSMutableArray alloc] init];
	bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
	while (participants) {
		LinphoneParticipant *participant = (LinphoneParticipant *)participants->data;
		NSString *uri = [NSString stringWithUTF8String:linphone_address_as_string_uri_only(linphone_participant_get_address(participant))];
		[contactsArray addObject:uri];

		if(linphone_participant_is_admin(participant))
		   [admins addObject:uri];
		participants = participants->next;
	}
	ChatConversationInfoView *view = VIEW(ChatConversationInfoView);
	view.create = FALSE;
	view.contacts = [contactsArray mutableCopy];
	view.oldContacts = [contactsArray mutableCopy];
	view.admins = [admins mutableCopy];
	view.oldAdmins = [admins mutableCopy];
	view.oldSubject = [NSString stringWithUTF8String:linphone_chat_room_get_subject(_chatRoom) ?: LINPHONE_DUMMY_SUBJECT];
	view.room = _chatRoom;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

#pragma mark ChatRoomDelegate

- (BOOL)startImageUpload:(UIImage *)image assetId:(NSString *)phAssetId withQuality:(float)quality {
	FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
	[fileTransfer upload:image withassetId:phAssetId forChatRoom:_chatRoom withQuality:quality];
	[_tableController scrollToBottom:true];
	return TRUE;
}

- (BOOL)startImageUpload:(UIImage *)image assetId:(NSString *)phAssetId withQuality:(float)quality andMessage:(NSString *)message {
    FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
    [fileTransfer setText:message];
    [fileTransfer upload:image withassetId:phAssetId forChatRoom:_chatRoom withQuality:quality];
    [_tableController scrollToBottom:true];
    return TRUE;
}

- (BOOL)startFileUpload:(NSData *)data assetId:(NSString *)phAssetId {
    FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
    [fileTransfer uploadVideo:data withassetId:phAssetId forChatRoom:_chatRoom];
    [_tableController scrollToBottom:true];
    return TRUE;
}

- (BOOL)startFileUpload:(NSData *)data withName:(NSString *)name  {
    FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
    [fileTransfer uploadFile:data forChatRoom:_chatRoom withName:name];
    [_tableController scrollToBottom:true];
    return TRUE;
}

- (void)resendChat:(NSString *)message withExternalUrl:(NSString *)url {
	[self sendMessage:message withExterlBodyUrl:[NSURL URLWithString:url] withInternalURL:nil];
}

#pragma mark ImagePickerDelegate

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSString *)phAssetId {
	// When getting image from the camera, it may be 90° rotated due to orientation
	// (image.imageOrientation = UIImageOrientationRight). Just rotate it to be face up.
	if (image.imageOrientation != UIImageOrientationUp) {
		UIGraphicsBeginImageContextWithOptions(image.size, false, image.scale);
		[image drawInRect:CGRectMake(0, 0, image.size.width, image.size.height)];
		image = UIGraphicsGetImageFromCurrentImageContext();
		UIGraphicsEndImageContext();
	}

	// Dismiss popover on iPad
	if (IPAD) {
		[VIEW(ImagePickerView).popoverController dismissPopoverAnimated:TRUE];
	}
    [self chooseImageQuality:image assetId:phAssetId];
}

- (void)tableViewIsScrolling {
	// if user is scrolling in table view, we should hide the keyboard
	if ([_messageField isFirstResponder]) {
		[_messageField resignFirstResponder];
	}
}

#pragma mark - Keyboard Event Functions

- (void)keyboardWillHide:(NSNotification *)notif {
	NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    
    int heightDiff = UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]) ? 55 : 105;
    
	[UIView animateWithDuration:duration
		delay:0
		options:UIViewAnimationOptionBeginFromCurrentState
		animations:^{
		  CGFloat composeIndicatorCompensation = composingVisible ? _composeIndicatorView.frame.size.height : 0.0f;

		  //						  Show TabBar and status bar and also top bar

		  // somehow, it breaks rotation if we put that in the block above when rotating portrait -> landscape
		  //						 if (!UIInterfaceOrientationIsPortrait(self.interfaceOrientation)) {
		  [PhoneMainView.instance hideTabBar:NO];
		  //						 }
		  [PhoneMainView.instance hideStatusBar:NO];
		  [PhoneMainView.instance fullScreen:NO];
		  _topBar.alpha = 1.0;

		  // Resize chat view
		  {
			  CGRect chatFrame = [_chatView frame];
			  chatFrame.origin.y = _topBar.frame.origin.y + _topBar.frame.size.height;
			  chatFrame.size.height = [[self view] frame].size.height - chatFrame.origin.y;
			  [_chatView setFrame:chatFrame];
		  }

		  // Resize & Move table view
		  {
			  CGRect tableFrame = [_tableController.view frame];
			  tableFrame.size.height =
				  [_messageView frame].origin.y - tableFrame.origin.y - composeIndicatorCompensation;
			  [_tableController.view setFrame:tableFrame];

			  // Scroll to bottom
			  NSInteger lastSection = [_tableController.tableView numberOfSections] - 1;
			  if (lastSection >= 0) {
				  NSInteger lastRow = [_tableController.tableView numberOfRowsInSection:lastSection] - 1;
				  if (lastRow >= 0) {
					  [_tableController.tableView
						  scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:lastRow inSection:lastSection]
								atScrollPosition:UITableViewScrollPositionBottom
										animated:FALSE];
				  }
			  }
		  }
            
            
            if ([_imagesArray count] > 0){
                // resizing imagesView
                CGRect imagesFrame = [_imagesView frame];
                imagesFrame.origin.y = [_messageView frame].origin.y - heightDiff;
                imagesFrame.size.height = heightDiff;
                [_imagesView setFrame:imagesFrame];
                // resizing chatTable
                CGRect tableViewFrame = [_tableController.tableView frame];
                tableViewFrame.size.height = imagesFrame.origin.y - tableViewFrame.origin.y;
                [_tableController.tableView setFrame:tableViewFrame];
            }
		}
		completion:^(BOOL finished){

		}];
}

- (void)keyboardWillShow:(NSNotification *)notif {
	NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    
    int heightDiff = UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]) ? 55 : 105;
    
	[UIView animateWithDuration:duration
		delay:0
		options:UIViewAnimationOptionBeginFromCurrentState
		animations:^{
		  CGFloat composeIndicatorCompensation = composingVisible ? _composeIndicatorView.frame.size.height : 0.0f;

		  CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];

		  if (([[UIDevice currentDevice].systemVersion floatValue] < 8) &&
			  UIInterfaceOrientationIsLandscape([UIApplication sharedApplication].statusBarOrientation)) {
			  int width = endFrame.size.height;
			  endFrame.size.height = endFrame.size.width;
			  endFrame.size.width = width;
		  }

		  // Hide TabBar and status bar and also top bar
		  [PhoneMainView.instance hideTabBar:YES];
		  [PhoneMainView.instance hideStatusBar:YES];
		  [PhoneMainView.instance fullScreen:YES];
		  _topBar.alpha = 0.0;

		  // Resize chat view
		  {
			  CGRect viewFrame = [[self view] frame];
			  CGRect rect = PhoneMainView.instance.view.bounds;
			  CGPoint pos = {viewFrame.size.width, viewFrame.size.height};
			  CGPoint gPos =
				  [self.view convertPoint:pos
								   toView:[UIApplication sharedApplication]
											  .keyWindow.rootViewController.view]; // Bypass IOS bug on landscape mode
			  float diff = (rect.size.height - gPos.y - endFrame.size.height);
			  if (diff > 0)
				  diff = 0;
			  CGRect chatFrame = [_chatView frame];
			  chatFrame.origin.y = 0;
			  chatFrame.size.height = viewFrame.size.height - chatFrame.origin.y + diff;
			  [_chatView setFrame:chatFrame];
		  }

		  // Resize & Move table view
		  {
			  CGRect tableFrame = _tableController.view.frame;
			  tableFrame.size.height =
				  [_messageView frame].origin.y - tableFrame.origin.y - composeIndicatorCompensation;
			  [_tableController.view setFrame:tableFrame];
		  }
            
            if ([_imagesArray count] > 0){
                // resizing imagesView
                CGRect imagesFrame = [_imagesView frame];
                imagesFrame.origin.y = [_messageView frame].origin.y - heightDiff;
                imagesFrame.size.height = heightDiff;
                [_imagesView setFrame:imagesFrame];
                // resizing chatTable
                CGRect tableViewFrame = [_tableController.tableView frame];
                tableViewFrame.size.height = imagesFrame.origin.y - tableViewFrame.origin.y;
                [_tableController.tableView setFrame:tableViewFrame];
            }

		  // Scroll
		  NSInteger lastSection = [_tableController.tableView numberOfSections] - 1;
		  if (lastSection >= 0) {
			  NSInteger lastRow = [_tableController.tableView numberOfRowsInSection:lastSection] - 1;
			  if (lastRow >= 0) {
				  [_tableController.tableView
					  scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:lastRow inSection:lastSection]
							atScrollPosition:UITableViewScrollPositionBottom
									animated:FALSE];
			  }
		  }
            
		}
		completion:^(BOOL finished){
            
		}];
}

#pragma mark - chat room callbacks

void on_chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	view.messageField.editable = !linphone_chat_room_has_been_left(cr);
	view.pictureButton.enabled = !linphone_chat_room_has_been_left(cr);
	view.messageView.userInteractionEnabled = !linphone_chat_room_has_been_left(cr);
}

void on_chat_room_subject_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	const char *subject = linphone_chat_room_get_subject(cr) ?: linphone_event_log_get_subject(event_log);
	if (subject) {
		view.addressLabel.text = [NSString stringWithUTF8String:subject];
		[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
		[view.tableController scrollToBottom:true];
	}
}

void on_chat_room_participant_added(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[view updateParticipantLabel];
	[view.tableController scrollToBottom:true];
}

void on_chat_room_participant_removed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[view updateParticipantLabel];
	[view.tableController scrollToBottom:true];
}

void on_chat_room_participant_admin_status_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[view.tableController scrollToBottom:true];
}

void on_chat_room_chat_message_received(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));

	LinphoneChatMessage *chat = linphone_event_log_get_chat_message(event_log);
	if (!chat)
		return;

    BOOL hasFile = FALSE;
    // if auto_download is available and file is downloaded
    if ((linphone_core_get_max_size_for_auto_download_incoming_files(LC) > -1) && linphone_chat_message_get_file_transfer_information(chat))
        hasFile = TRUE;

	if (!linphone_chat_message_is_file_transfer(chat) && !linphone_chat_message_is_text(chat) && !hasFile) /*probably an imdn*/
		return;
		
	const LinphoneAddress *from = linphone_chat_message_get_from_address(chat);
	if (!from)
		return;
  
    if (hasFile) {
        [view autoDownload:chat view:view inChat:TRUE];
        [view.tableController addEventEntry:(LinphoneEventLog *)event_log];
        return;
    }

	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
	[view.tableController scrollToLastUnread:TRUE];
}



void on_chat_room_chat_message_sent(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[view.tableController scrollToBottom:true];
    [ChatsListTableView saveDataToUserDefaults];

	if (IPAD)
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
}

void on_chat_room_is_composing_received(LinphoneChatRoom *cr, const LinphoneAddress *remoteAddr, bool_t isComposing) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	BOOL composing = linphone_chat_room_is_remote_composing(cr) || bctbx_list_size(linphone_chat_room_get_composing_addresses(cr)) > 0;
	[view setComposingVisible:composing withDelay:0.3];
}

void on_chat_room_conference_joined(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view configureForRoom:false];
	[view.tableController scrollToBottom:true];
}

void on_chat_room_conference_left(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[view.tableController scrollToBottom:true];
}

- (void)goToDeviceListView {
    DevicesListView *view = VIEW(DevicesListView);
    view.room = _chatRoom;
    [PhoneMainView.instance popToView:view.compositeViewDescription];
}

- (void)openFile:(NSString *) filePath
{
    // Open the controller.
    _documentInteractionController = [UIDocumentInteractionController interactionControllerWithURL:[NSURL fileURLWithPath:filePath]];
    _documentInteractionController.delegate = self;
    
    BOOL canOpen =  [_documentInteractionController presentOpenInMenuFromRect:CGRectZero inView:self.view animated:YES];
    //NO app can open the file
    if (canOpen == NO) {
        [[[UIAlertView alloc] initWithTitle:@"Info" message:@"There is no app found to open it" delegate:nil cancelButtonTitle:@"cancel" otherButtonTitles:nil, nil] show];
        
    }
}

- (void)deleteImageWithAssetId:(NSString *)assetId {
    NSUInteger key = [_assetIdsArray indexOfObject:assetId];
    [_imagesArray removeObjectAtIndex:key];
    [_assetIdsArray removeObjectAtIndex:key];
    [self refreshImageDrawer];
}

- (void)clearMessageView {
    [_messageField setText:@""];
    _imagesArray = [NSMutableArray array];
    _assetIdsArray = [NSMutableArray array];
    
    [self refreshImageDrawer];
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return [_imagesArray count];
}

- (__kindof UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
    UIImageViewDeletable *imgView = [collectionView dequeueReusableCellWithReuseIdentifier:NSStringFromClass([UIImageViewDeletable class]) forIndexPath:indexPath];
    CGRect imgFrame = imgView.frame;
    imgFrame.origin.y = 5;
    if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation])) {
        imgFrame.size.height = 50;
    } else {
        imgFrame.size.height = 100;
    }
    [imgView.image setImage:[UIImage resizeImage:[_imagesArray objectAtIndex:[indexPath item]] withMaxWidth:imgFrame.size.width andMaxHeight:imgFrame.size.height]];
    [imgView setAssetId:[_assetIdsArray objectAtIndex:[indexPath item]]];
    [imgView setDeleteDelegate:self];
    [imgView setFrame:imgFrame];
    [_sendButton setEnabled:TRUE];
    return imgView;
}

- (void)refreshImageDrawer {
    int heightDiff = UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]) ? 55 : 105;
    
    if ([_imagesArray count] == 0) {
        [UIView animateWithDuration:0
                              delay:0
                            options:UIViewAnimationOptionBeginFromCurrentState
                         animations:^{
                             // resizing imagesView
                             CGRect imagesFrame = [_imagesView frame];
                             imagesFrame.origin.y = [_messageView frame].origin.y;
                             imagesFrame.size.height = 0;
                             [_imagesView setFrame:imagesFrame];
                             // resizing chatTable
                             CGRect tableViewFrame = [_tableController.tableView frame];
                             tableViewFrame.size.height = imagesFrame.origin.y - tableViewFrame.origin.y;
                             [_tableController.tableView setFrame:tableViewFrame];
                         }
                         completion:nil];
        if ([_messageField.text isEqualToString:@""])
            [_sendButton setEnabled:FALSE];
    } else {
        [UIView animateWithDuration:0
                              delay:0
                            options:UIViewAnimationOptionBeginFromCurrentState
                         animations:^{
                             // resizing imagesView
                             CGRect imagesFrame = [_imagesView frame];
                             imagesFrame.origin.y = [_messageView frame].origin.y - heightDiff;
                             imagesFrame.size.height = heightDiff;
                             [_imagesView setFrame:imagesFrame];
                             // resizing chatTable
                             CGRect tableViewFrame = [_tableController.tableView frame];
                             tableViewFrame.size.height = imagesFrame.origin.y - tableViewFrame.origin.y;
                             [_tableController.tableView setFrame:tableViewFrame];
                         }
                         completion:^(BOOL result){[_imagesCollectionView reloadData];}];
    }
}

- (void)autoDownload:(LinphoneChatMessage *)message view:(ChatConversationView *)view inChat:(BOOL)inChat{
    //TODO: migrate with  "linphone_iphone_file_transfer_recv"
    LinphoneContent *content = linphone_chat_message_get_file_transfer_information(message);
    NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
    // get download path
    NSString *filePath = [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:name];
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ([fileManager fileExistsAtPath:filePath]) {
        NSData* data = [NSData dataWithContentsOfFile:filePath];
        NSString *fileType = [NSString stringWithUTF8String:linphone_content_get_type(content)];
        if ([fileType isEqualToString:@"image"]) {
            // we're finished, save the image and update the message
            UIImage *image = [UIImage imageWithData:data];
            if (!image) {
                UIAlertController *errView = [UIAlertController
                                              alertControllerWithTitle:NSLocalizedString(@"File download error", nil)
                                              message:NSLocalizedString(@"Error while downloading the file.\n"
                                                                        @"The file is probably encrypted.\n"
                                                                        @"Please retry to download this file after activating LIME.",
                                                                        nil)
                                              preferredStyle:UIAlertControllerStyleAlert];
                
                UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
                                                                        style:UIAlertActionStyleDefault
                                                                      handler:^(UIAlertAction *action){
                                                                      }];
                
                [errView addAction:defaultAction];
                [PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
                return;
            }
            
            // until image is properly saved, keep a reminder on it so that the
            // chat bubble is aware of the fact that image is being saved to device
            //[LinphoneManager setValueInMessageAppData:@"saving..." forKey:@"localimage" inMessage:self.message];
            
            __block PHObjectPlaceholder *placeHolder;
            [[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
                PHAssetCreationRequest *request = [PHAssetCreationRequest creationRequestForAssetFromImage:image];
                placeHolder = [request placeholderForCreatedAsset];
            } completionHandler:^(BOOL success, NSError *error) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (error) {
                        LOGE(@"Cannot save image data downloaded [%@]", [error localizedDescription]);
                        [LinphoneManager setValueInMessageAppData:nil forKey:@"localimage" inMessage:message];
                        UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                                                         message:NSLocalizedString(@"Cannot write image to photo library",
                                                                                                                   nil)
                                                                                  preferredStyle:UIAlertControllerStyleAlert];
                        
                        UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
                                                                                style:UIAlertActionStyleDefault
                                                                              handler:^(UIAlertAction * action) {}];
                        
                        [errView addAction:defaultAction];
                        [PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
                    } else {
                        LOGI(@"Image saved to [%@]", [placeHolder localIdentifier]);
                        [LinphoneManager setValueInMessageAppData:[placeHolder localIdentifier]
                                                           forKey:@"localimage"
                                                        inMessage:message];
                    }
                    if(inChat) {
                        [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
                        [view.tableController scrollToLastUnread:TRUE];
                    }
                });
            }];
        }  else if([fileType isEqualToString:@"video"]) {
            // until image is properly saved, keep a reminder on it so that the
            // chat bubble is aware of the fact that image is being saved to device
            //[LinphoneManager setValueInMessageAppData:@"saving..." forKey:@"localvideo" inMessage:self.message];
            
            __block PHObjectPlaceholder *placeHolder;
            [[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
                PHAssetCreationRequest *request = [PHAssetCreationRequest creationRequestForAssetFromVideoAtFileURL:[NSURL fileURLWithPath:filePath]];
                placeHolder = [request placeholderForCreatedAsset];
            } completionHandler:^(BOOL success, NSError * _Nullable error) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    if (error) {
                        LOGE(@"Cannot save video data downloaded [%@]", [error localizedDescription]);
                        [LinphoneManager setValueInMessageAppData:nil forKey:@"localvideo" inMessage:message];
                        UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                                                         message:NSLocalizedString(@"Cannot write video to photo library",
                                                                                                                   nil)
                                                                                  preferredStyle:UIAlertControllerStyleAlert];
                        
                        UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
                                                                                style:UIAlertActionStyleDefault
                                                                              handler:^(UIAlertAction * action) {}];
                        
                        [errView addAction:defaultAction];
                        [PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
                    } else {
                        LOGI(@"video saved to [%@]", [placeHolder localIdentifier]);
                        [LinphoneManager setValueInMessageAppData:[placeHolder localIdentifier]
                                                           forKey:@"localvideo"
                                                        inMessage:message];
                    }
                    if(inChat) {
                        [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
                        [view.tableController scrollToLastUnread:TRUE];
                    }
                });
            }];
            
        } else {
            NSString *key =  @"localfile";
            //write file to path
            dispatch_async(dispatch_get_main_queue(), ^{
                [LinphoneManager setValueInMessageAppData:name forKey:key inMessage:message];
                [LinphoneManager setValueInMessageAppData:filePath forKey:@"cachedfile" inMessage:message];
                if(inChat) {
                    [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
                    [view.tableController scrollToLastUnread:TRUE];
                }
            });
        }
    }
}

@end
