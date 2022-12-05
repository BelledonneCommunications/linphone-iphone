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

#import <Photos/PHAssetChangeRequest.h>

#import "linphoneapp-Swift.h"
#import "ChatConversationView.h"
#import "PhoneMainView.h"
#import "Utils.h"
#import "FileTransferDelegate.h"
#import "UIChatBubbleTextCell.h"
#import "DevicesListView.h"
#import "SVProgressHUD.h"
#import "EphemeralSettingsView.h"
#import "Utils.h"

@implementation FileContext

- (void)addObject:(UIImage *)image withQuality:(float)quality {
	NSString *name = [NSString stringWithFormat:@"%li-%f.jpg", (long)image.hash, [NSDate timeIntervalSinceReferenceDate]];
	NSData *data = UIImageJPEGRepresentation(image, quality);

	[self addObject:data name:name type:@"image" image:image];
}

- (void)addObject:(NSData *)data name:(NSString *)name type:(NSString *)type image:(UIImage *)image {
	[_previewsArray addObject:image];
	[_uuidsArray addObject:[NSUUID UUID]];
	[self addObject:data name:name type:type];
}

- (void)addObject:(NSData *)data name:(NSString *)name type:(NSString *)type {
	[_namesArray addObject:name];
	[_typesArray addObject:type];
	[_datasArray addObject:data];
}

- (void)deleteContentWithUuid:(NSUUID *)uuid {
	NSUInteger key = [_uuidsArray indexOfObject:uuid];
	[_previewsArray removeObjectAtIndex:key];
	[_uuidsArray removeObjectAtIndex:key];
	[_namesArray removeObjectAtIndex:key];
	[_typesArray removeObjectAtIndex:key];
	[_datasArray removeObjectAtIndex:key];
}

- (void)clear {
	_previewsArray = [NSMutableArray array];
	_uuidsArray = [NSMutableArray array];
	_namesArray = [NSMutableArray array];
	_typesArray = [NSMutableArray array];
	_datasArray = [NSMutableArray array];
}

- (NSUInteger)count {
	return [_datasArray count];
}

@end


@implementation PreviewItem
- (instancetype)initPreviewURL:(NSURL *)docURL
                     WithTitle:(NSString *)title {
    self = [super init];
    if (self) {
        _previewItemURL = [docURL copy];
        _previewItemTitle = [title copy];
    }
    return self;
}
@end

@implementation FileDataSource
- (instancetype)initWithFiles:(NSMutableArray<NSURL*>*)files {
    self = [super init];
    if (self) {
		_files = files;
    }
    return self;
}
- (NSInteger)numberOfPreviewItemsInPreviewController:(QLPreviewController *)controller {
	return _files.count;
}
- (id<QLPreviewItem>)previewController:(QLPreviewController *)controller previewItemAtIndex:(NSInteger)index {
	NSURL *url = [_files objectAtIndex:index];
	return [[PreviewItem alloc] initPreviewURL:url WithTitle:[url lastPathComponent]];
}
@end

@implementation ChatConversationView

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		scrollOnGrowingEnabled = TRUE;
		_chatRoom = NULL;
		_chatRoomCbs = NULL;
        securityDialog = NULL;
		isOneToOne = TRUE;
		isEncrypted = FALSE;
		imageQualities = [[OrderedDictionary alloc]
			initWithObjectsAndKeys:[NSNumber numberWithFloat:0.9], NSLocalizedString(@"Maximum", nil),
								   [NSNumber numberWithFloat:0.5], NSLocalizedString(@"Average", nil),
								   [NSNumber numberWithFloat:0.0], NSLocalizedString(@"Minimum", nil), nil];
		composingVisible = false;
		[self initSharedPlayer];
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
																tabBar:IPAD ? TabBarView.class :nil
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
	[_imagesCollectionView setDelegate:self];
	[_toggleSelectionButton setImage:[UIImage imageNamed:@"select_all_default.png"] forState:UIControlStateSelected];
	
	_vrInnerView.layer.cornerRadius = 5.0f;
	_vrInnerView.layer.masksToBounds = YES;
	_vrWaveMaskPlayer.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_L"]]; // rgba(1,88,7,0.2);
	_showVoiceRecorderView = false;
	_toggleMenuButton.imageView.contentMode = UIViewContentModeScaleAspectFit;
	_toggleRecord.imageView.contentMode = UIViewContentModeScaleAspectFit;
}

- (void)refreshData {
    [_tableController refreshData];
    [refreshControl endRefreshing];
	if (_tableController.totalNumberOfItems == 0)
		return;
    [_tableController loadData];
    [_tableController.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:_tableController.currentIndex inSection:0]
                          atScrollPosition:UITableViewScrollPositionTop
                                  animated:false];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	if (!_chatRoom)
		[self onLinphoneCoreReady:nil];
	
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(applicationDidEnterBackground)
											   name:UIApplicationDidEnterBackgroundNotification
											 object:nil];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(applicationWillEnterForeground)
											   name:UIApplicationWillEnterForegroundNotification
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
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(onLinphoneCoreReady:)
											   name:kLinphoneGlobalStateUpdate
											 object:nil];
	
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(endVoicePlayingIfDoingSO:)
											   name:kLinphoneVoiceMessagePlayerLostFocus
											 object:nil];
	
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(endVoicePlayingIfDoingSO:)
											   name:kLinphoneVoiceMessagePlayerEOF
											 object:nil];
	if ([_fileContext count] > 0) {
		[UIView animateWithDuration:0
							  delay:0
							options:UIViewAnimationOptionBeginFromCurrentState
						 animations:^{
			// resizing imagesView
			CGRect imagesFrame = [_imagesView frame];
			imagesFrame.origin.y = [_messageView frame].origin.y - 120;
			imagesFrame.size.height = 120;
			[_imagesView setFrame:imagesFrame];
			// resizing chatTable
			CGRect tableViewFrame = [_tableController.tableView frame];
			tableViewFrame.size.height -= 120;
			[_tableController.tableView setFrame:tableViewFrame];
			[self updateFramesInclRecordingAndReplyView];
		} completion:nil];
	}
	[self configureForRoom:self.editing];
	
	// Resize the popup table depending on wether ephemeral messages are enabled or not.
	CGRect popupFrame = _popupMenu.frame;
	popupFrame.size.height = 44 * [_popupMenu numberOfRowsInSection:0];
	_popupMenu.frame = popupFrame;
	
	// Voice recording & Replies
	_vrView.hidden = true;
	_toggleRecord.enabled = linphone_core_get_calls_nb(LC) == 0;
	_replyView.hidden = true;
	_preservePendingActions = false;
	
	_toggleRecord.enabled = linphone_core_get_calls_nb(LC) == 0;
	
	[PhoneMainView.instance hideTabBar:!IPAD];
}

- (void)viewWillDisappear:(BOOL)animated {
	
	if (!_preservePendingActions)
		[self cancelVoiceRecording];
	
	if (!_preservePendingActions)
		[self closePendingReply];
	
	
	
	else if (_isVoiceRecording)
		[self stopVoiceRecording];

	[super viewWillDisappear:animated];

	[self removeCallBacks];

	[_messageField resignFirstResponder];

	[self setComposingVisible:false withDelay:0]; // will hide the "user is composing.." message
	
	[self stopAllPlays];
	
	[NSNotificationCenter.defaultCenter removeObserver:self];
	PhoneMainView.instance.currentRoom = NULL;
	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
	_chatRoom = NULL;
	_tableController.chatRoom = nil;
}

- (void)removeCallBacks {
	if (_chatRoom && _chatRoomCbs) {
		linphone_chat_room_remove_callbacks(_chatRoom, _chatRoomCbs);
		_chatRoomCbs = NULL;
	}
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	if ([[UIApplication sharedApplication] applicationState] == UIApplicationStateBackground) {
		[self refreshImageDrawer];
		return;
	}
	composingVisible = !composingVisible;

	// force offset recomputing
	[_messageField refreshHeight];
	LinphoneAddress *peerAddr = linphone_core_create_address([LinphoneManager getLc], _peerAddress);
	LinphoneAddress *localAddr = linphone_core_create_address([LinphoneManager getLc], _localAddress);
	if (peerAddr && localAddr) {
		_chatRoom = linphone_core_search_chat_room([LinphoneManager getLc], NULL, localAddr, peerAddr, NULL);
		if (_chatRoom) {
			isOneToOne = linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesOneToOne;
			isEncrypted = linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesEncrypted;
			[self setComposingVisible:!composingVisible withDelay:0];
			[self configureForRoom:true];
			[_tableController scrollToBottom:true];
		}
	}

	_backButton.hidden = _tableController.isEditing;
	[self refreshImageDrawer];
	[self stopAllPlays];
	[self keyboardWillHide:nil];
}

#pragma mark -

- (void)applicationDidEnterBackground{
	if (!_preservePendingActions)
		[self cancelVoiceRecording];
	else if (_isVoiceRecording)
		[self stopVoiceRecording];
	if (!_preservePendingActions)
		[self closePendingReply];
	[self stopAllPlays];
	_chatRoom = nil;
	[_messageField resignFirstResponder];
}

- (void)applicationWillEnterForeground{
	if (_chatRoom == nil) {
		if (linphone_core_get_calls_nb(LC) == 0)
			[SVProgressHUD show];
		else
			[self onLinphoneCoreReady:nil];
	}
}

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
		linphone_chat_room_cbs_set_chat_message_sending(_chatRoomCbs, on_chat_room_chat_message_sending);
		linphone_chat_room_cbs_set_is_composing_received(_chatRoomCbs, on_chat_room_is_composing_received);
		linphone_chat_room_cbs_set_conference_joined(_chatRoomCbs, on_chat_room_conference_joined);
		linphone_chat_room_cbs_set_conference_left(_chatRoomCbs, on_chat_room_conference_left);
         linphone_chat_room_cbs_set_security_event(_chatRoomCbs, on_chat_room_conference_alert);
		linphone_chat_room_cbs_set_user_data(_chatRoomCbs, (__bridge void*)self);
		linphone_chat_room_add_callbacks(_chatRoom, _chatRoomCbs);
	}

	[self updateSuperposedButtons];

	if (_tableController.isEditing)
		[_tableController setEditing:editing];

	BOOL fileSharingEnabled = linphone_core_get_file_transfer_server(LC) != NULL;
	[_pictureButton setEnabled:fileSharingEnabled];

	[self updateSuperposedButtons];
	_toggleRecord.enabled = linphone_core_get_calls_nb(LC) == 0;
	
	PhoneMainView.instance.currentRoom = _chatRoom;
	if (isOneToOne) {
		bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
		LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
		const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(_chatRoom);
		[ContactDisplay setDisplayNameLabel:_addressLabel forAddress:addr];
	} else
		_addressLabel.text = [NSString stringWithUTF8String:linphone_chat_room_get_subject(_chatRoom) ?: LINPHONE_DUMMY_SUBJECT];

	[self updateParticipantLabel];
	[self configureMessageField];
	[_tableController setChatRoom:_chatRoom];

	_chatView.hidden = NO;
    UIImage *image = [FastAddressBook imageForSecurityLevel:linphone_chat_room_get_security_level(_chatRoom)];
    [_encryptedButton setImage:image forState:UIControlStateNormal];
    _encryptedButton.hidden = image ? FALSE : TRUE;
	[self update];
    [self shareFile];
	
	[self setupPopupMenu];
	_ephemeralndicator.hidden = !linphone_chat_room_ephemeral_enabled(_chatRoom);
    [self handlePendingTransferIfAny];

}

+(BOOL) isBasicChatRoom:(LinphoneChatRoom *)room {
	if (!room)
		return true;
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(room);
	return capabilities & LinphoneChatRoomCapabilitiesBasic;
}

-(BOOL) isEncryptedChatRoom:(LinphoneChatRoom *)room {
	if (!room)
		return true;
	LinphoneChatRoomCapabilitiesMask capabilities = linphone_chat_room_get_capabilities(room);
	return capabilities & LinphoneChatRoomCapabilitiesEncrypted;
}



- (void)configureMessageField {
	if (isOneToOne) {
		_messageField.editable = TRUE;
		_pictureButton.enabled = TRUE;
		_messageView.userInteractionEnabled = TRUE;
		if (linphone_chat_room_is_read_only(_chatRoom)) {
			linphone_chat_room_add_participant(_chatRoom, linphone_participant_get_address(linphone_chat_room_get_me(_chatRoom)));
		}
	} else {
		_messageField.editable = !linphone_chat_room_is_read_only(_chatRoom);
		_pictureButton.enabled = !linphone_chat_room_is_read_only(_chatRoom);
		_messageView.userInteractionEnabled = !linphone_chat_room_is_read_only(_chatRoom);
	}
}

-(NSData *) nsDataRead {
	NSString* groupName = [NSString stringWithFormat:@"group.%@.linphoneExtension",[[NSBundle mainBundle] bundleIdentifier]];
	NSString *path  =[[[NSFileManager defaultManager] containerURLForSecurityApplicationGroupIdentifier:groupName] path];
	NSString *fullCacheFilePathPath = [NSString stringWithFormat:@"%@/%@",path,@"nsData"];
	return[NSData dataWithContentsOfFile:fullCacheFilePathPath];
}


- (void)shareFile {
	NSString* groupName = [NSString stringWithFormat:@"group.%@.linphoneExtension",[[NSBundle mainBundle] bundleIdentifier]];


    NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:groupName];
    NSDictionary *dict = [defaults valueForKey:@"photoData"];
    NSDictionary *dictFile = [defaults valueForKey:@"icloudData"];
    NSDictionary *dictUrl = [defaults valueForKey:@"url"];
    if (dict) {
        //file shared from photo lib
        NSString *fileName = dict[@"url"];
        [_messageField setText:dict[@"message"]];
				[self confirmShare:[self nsDataRead] url:nil fileName:fileName];
        [defaults removeObjectForKey:@"photoData"];
    } else if (dictFile) {
        NSString *fileName = dictFile[@"url"];
        [_messageField setText:dictFile[@"message"]];
        [self confirmShare:[self nsDataRead] url:nil fileName:fileName];
        [defaults removeObjectForKey:@"icloudData"];
    } else if (dictUrl) {
        NSString *url = dictUrl[@"url"];
        [_messageField setText:dictUrl[@"message"]];
        [self confirmShare:nil url:url fileName:nil];
        [defaults removeObjectForKey:@"url"];
    }
}

// reload the chatroom after the core starts
- (void)onLinphoneCoreReady:(NSNotification *)notif {
	if (linphone_core_get_global_state(LC) == LinphoneGlobalOn) {
		LinphoneAddress *peerAddr = linphone_core_create_address([LinphoneManager getLc], _peerAddress);
		LinphoneAddress *localAddr = linphone_core_create_address([LinphoneManager getLc], _localAddress);
		if (peerAddr && localAddr) {
			_chatRoom = linphone_core_search_chat_room([LinphoneManager getLc], NULL, localAddr, peerAddr, NULL);
			if (_chatRoom) {
				isOneToOne = linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesOneToOne;
				isEncrypted = linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesEncrypted;
				[self configureForRoom:self.editing];
				if (_chatRoom && _markAsRead) {
					if (IPAD) {
						[VIEW(ChatsListView).tableController loadData];
					}
					[ChatConversationView markAsRead:_chatRoom];
				}
				_markAsRead = TRUE;
			}
		}
		[SVProgressHUD dismiss];
	}
}

- (void)callUpdateEvent:(NSNotification *)notif {
    [self updateSuperposedButtons];
	_toggleRecord.enabled = linphone_core_get_calls_nb(LC) == 0;
	[_tableController.tableView reloadData];

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

- (BOOL)sendMessage:(NSString *)message withExterlBodyUrl:(NSURL *)externalUrl rootMessage:(LinphoneChatMessage *)rootMessage {
	if (_chatRoom == NULL) {
		LOGW(@"Cannot send message: No chatroom");
		return FALSE;
	}
	
	LinphoneChatMessage *msg = rootMessage;
	BOOL basic = [ChatConversationView isBasicChatRoom:_chatRoom];
	const LinphoneAccountParams *params = linphone_account_get_params(linphone_core_get_default_account(LC));
	BOOL cpimEnabled = linphone_account_params_cpim_in_basic_chat_room_enabled(params);
	if ((!basic || cpimEnabled) && message && message.length > 0) {
		linphone_chat_message_add_utf8_text_content(msg, message.UTF8String);
	}
	
	if (externalUrl) {
		linphone_chat_message_set_external_body_url(msg, [[externalUrl absoluteString] UTF8String]);
	}
	
	bctbx_list_t const *contentList = linphone_chat_message_get_contents(msg);
	if (bctbx_list_size(contentList) > 0) {
		linphone_chat_message_send(msg);
	}
	
	if (basic && !cpimEnabled && message && message.length > 0) {
		linphone_chat_message_send(linphone_chat_room_create_message_from_utf8(_chatRoom, message.UTF8String));
	}
	
	return TRUE;
}

- (void)saveAndSend:(UIImage *)image assetId:(NSString *)phAssetId withQuality:(float)quality{
	[_fileContext addObject:image withQuality:quality];
	[_qualitySettingsArray addObject:@(quality)];
	[self refreshImageDrawer];
}

- (void)chooseImageQuality:(UIImage *)image assetId:(NSString *)phAssetId {
	[SVProgressHUD show];
	NSMutableDictionary *optionsBlock = [[NSMutableDictionary alloc] init];
	NSMutableDictionary *optionsText = [[NSMutableDictionary alloc] init];
	DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Choose the image size", nil)];
	dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
	  for (NSString *key in [imageQualities allKeys]) {
		  NSNumber *quality = [imageQualities objectForKey:key];
		  NSData *data = UIImageJPEGRepresentation(image, [quality floatValue]);
		  NSNumber *size = [NSNumber numberWithInteger:[data length]];
		  NSString *text = [NSString stringWithFormat:@"%@ (%@)", key, [size toHumanReadableSize]];
		  [optionsBlock setObject:^() {
			  [self saveAndSend:image assetId:phAssetId withQuality:[quality floatValue]];
		  } forKey:key];
		  [optionsText setObject:text forKey:key];
	  }
	  dispatch_async(dispatch_get_main_queue(), ^{
		  for (NSString *key in [imageQualities allKeys]) {
			  [sheet addButtonWithTitle:[optionsText objectForKey:key] block:[optionsBlock objectForKey:key]];
		  }
		[sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
		[SVProgressHUD dismiss];
		[sheet showInView:PhoneMainView.instance.view];
	  });
	});
}

- (void)confirmShare:(NSData *)data url:(NSString *)url fileName:(NSString *)fileName {
    DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Select or create a conversation to share the file(s)", nil)];
    dispatch_async(dispatch_get_main_queue(), ^{
		[sheet addButtonWithTitle:NSLocalizedString(@"Send to this conversation", nil)
							block:^() {
								if (![[self.messageField text] isEqualToString:@""]) {
									[self sendMessageInMessageField:linphone_chat_room_create_empty_message(_chatRoom)];
								}
								if (url)
									[self sendMessage:url withExterlBodyUrl:nil rootMessage:linphone_chat_room_create_empty_message(_chatRoom)];
								else
									[self startFileUpload:data withName:fileName rootMessage:linphone_chat_room_create_empty_message(_chatRoom)];
		}];
     
        [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
		[sheet showInView:PhoneMainView.instance.view];
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
						 _composeIndicatorView.hidden = !visible;
					 }];
	if (visible) {
		if (_tableController.tableView.contentOffset.y + newComposingFrame.size.height >= (_tableController.tableView.contentSize.height - _tableController.tableView.frame.size.height - 1)) {
			[_tableController scrollToBottom:TRUE];
		}
	}
}

- (BOOL) groupCallAvailable {
	if (isOneToOne || !_backToCallButton.hidden || _tableController.tableView.isEditing)
		return false;
	LinphoneAccount *account = linphone_core_get_default_account(LC);
	if (!account)
		return false;
	const LinphoneAccountParams *params = linphone_account_get_params(account);
	if (!params)
		return false;
	return linphone_account_params_get_audio_video_conference_factory_address(params) != nil || linphone_account_params_get_conference_factory_uri(params) != nil;
	
}

- (void)updateSuperposedButtons {
	[_backToCallButton update];
	_callButton.hidden = !_backToCallButton.hidden || _tableController.tableView.isEditing;
	_toggleMenuButton.hidden = _tableController.isEditing;
	
	// Group call :
	if (self.groupCallAvailable ) {
		[_callButton setImage: [LinphoneUtils resizeImage:[UIImage imageNamed:@"voip_conference_new"] newSize:CGSizeMake(50, 50)] forState:UIControlStateNormal];
		_callButton.hidden = false;
	} else {
		[_callButton setImage:[UIImage imageNamed:@"call_alt_start_default"] forState:UIControlStateNormal];
	}
	
}

- (void)updateParticipantLabel {
    CGRect frame = _addressLabel.frame;
	if (isOneToOne) {
		_particpantsLabel.hidden = TRUE;
        frame.origin.y = (_topBar.frame.size.height - _addressLabel.frame.size.height)/2;
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
        frame.origin.y = 0;
	}
    _addressLabel.frame = frame;
}

- (void)sendMessageInMessageField:(LinphoneChatMessage *)rootMessage {
    if ([self sendMessage:[_messageField text] withExterlBodyUrl:nil rootMessage:rootMessage]) {
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

        if ([_fileContext count] > 0) {
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
		[self updateFramesInclRecordingAndReplyView];

		// if we're showing the compose message, update it position
		if (![_composeLabel isHidden]) {
			CGRect frame = [_composeIndicatorView frame];
			frame.origin.y -= diff;
			[_composeIndicatorView setFrame:frame];
		}
	}
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
	NSString *previousViewName = [PhoneMainView.instance getPreviousViewName];
	_sharingMedia = nil;
	if ([previousViewName isEqualToString:@"ContactDetailsView"]) {
		ContactDetailsView *view = VIEW(ContactDetailsView);
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	} else {
		ChatsListView *view = VIEW(ChatsListView);
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	}
}

- (IBAction)onEditClick:(id)event {
	[_tableController setEditing:![_tableController isEditing] animated:TRUE];
	[_messageField resignFirstResponder];
	[self updateSuperposedButtons];
}

- (IBAction)onSendClick:(id)event {

	if (!linphone_core_is_network_reachable(LC)) {
		//[PhoneMainView.instance presentViewController:[LinphoneUtils networkErrorView:@"send a message"] animated:YES completion:nil];
		//return;
	}
	
	LinphoneChatMessage *rootMessage = _replyBubble ? linphone_chat_room_create_reply_message(_chatRoom, _replyBubble.message) : linphone_chat_room_create_empty_message(_chatRoom);
	
	if (_replyBubble) {
		[self closePendingReply];
	}
	
	if (_isPendingVoiceRecord && _voiceRecorder && linphone_recorder_get_file(_voiceRecorder)) {
		LinphoneContent * voiceContent = linphone_recorder_create_content(_voiceRecorder);
		_isPendingVoiceRecord = false;
		[self cancelVoiceRecording];
		[self stopVoiceRecordPlayer];
		linphone_chat_message_add_content(rootMessage, voiceContent);
	}
	
	if ([_fileContext count] > 0) {
		if (linphone_chat_room_get_capabilities(_chatRoom) & LinphoneChatRoomCapabilitiesConference) {
			[self startMultiFilesUpload:rootMessage];
		} else {
			int i = 0;
			for (i = 0; i < [_fileContext count]-1; ++i) {
				[self startUploadData:[_fileContext.datasArray objectAtIndex:i] withType:[_fileContext.typesArray objectAtIndex:i] withName:[_fileContext.namesArray objectAtIndex:i] andMessage:NULL  rootMessage:NULL];
			}
			if (isOneToOne) {
				[self startUploadData:[_fileContext.datasArray objectAtIndex:i] withType:[_fileContext.typesArray objectAtIndex:i] withName:[_fileContext.namesArray objectAtIndex:i] andMessage:NULL  rootMessage:NULL];
				if (![[self.messageField text] isEqualToString:@""]) {
					[self sendMessage:[_messageField text] withExterlBodyUrl:nil rootMessage:rootMessage];
				}
			} else {
				[self startUploadData:[_fileContext.datasArray objectAtIndex:i] withType:[_fileContext.typesArray objectAtIndex:i] withName:[_fileContext.namesArray objectAtIndex:i] andMessage:[self.messageField text]  rootMessage:rootMessage];
			}
		}

		[self clearMessageView];
		return;
	}
	[self sendMessageInMessageField:rootMessage];
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
		_tableController.editButton.hidden = true;
		[_tableController loadData];
		[self onEditionChangeClick:nil];
	}];
}

- (IBAction)onEditionChangeClick:(id)sender {
	_backButton.hidden = _tableController.isEditing;
	[self updateSuperposedButtons];
}

- (IBAction)onEncryptedDevicesClick:(id)sender {
    NSString *message = NSLocalizedString(@"Instant messages are end-to-end encrypted in secured conversations. It is possible to upgrade the security level of a conversation by authenticating participants. To do so, call the contact and follow the authentification process.",nil);
    BOOL notAskAgain = [LinphoneManager.instance lpConfigBoolForKey:@"confirmation_dialog_before_sas_call_not_ask_again"];

    if (notAskAgain) {
        [self goToDeviceListView];
    } else {
        securityDialog = [UIConfirmationDialog ShowWithMessage:message cancelMessage:NSLocalizedString(@"CANCEL", nil) confirmMessage:NSLocalizedString(@"OK", nil) onCancelClick:^() {
        } onConfirmationClick:^() {
            [self goToDeviceListView];
        }];
		[_messageField resignFirstResponder];
        securityDialog.authView.hidden = FALSE;
		[securityDialog setSpecialColor];
    }
}

- (IBAction)onCallClick:(id)sender {
	bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
	LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
	const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(_chatRoom);
	if (self.groupCallAvailable) {
		UIConfirmationDialog *d = [UIConfirmationDialog ShowWithMessage:VoipTexts.conference_start_group_call_dialog_message
															cancelMessage:nil
														 confirmMessage:VoipTexts.conference_start_group_call_dialog_ok_button
															onCancelClick:^() {}
													onConfirmationClick:^() {
			[ConferenceViewModelBridge startGroupCallWithCChatRoom:_chatRoom];
		}];
		d.groupCallImage.hidden = NO;
		[d.groupCallImage setImageNamed:@"voip_conference_new" tintColor:UIColor.whiteColor];
		[d setSpecialColor];
		[d setWhiteCancel];
	} else
		[LinphoneManager.instance call:addr];	
}

- (IBAction)onListSwipe:(id)sender {
	[self onBackClick:sender];
}

- (IBAction)onMessageChange:(id)sender {
	[self setSendButtonState];
}

- (IBAction)onPictureClick:(id)event {
	_preservePendingActions = true;
	[_messageField resignFirstResponder];
	[ImagePickerView SelectImageFromDevice:self atPosition:_pictureButton inView:self.view withDocumentMenuDelegate:self];

}

- (void)displayGroupInfo {
	NSMutableArray *contactsArray = [[NSMutableArray alloc] init];
	NSMutableArray *admins = [[NSMutableArray alloc] init];
	bctbx_list_t *participants = linphone_chat_room_get_participants(_chatRoom);
	while (participants) {
		LinphoneParticipant *participant = (LinphoneParticipant *)participants->data;
		char *curi = linphone_address_as_string_uri_only(linphone_participant_get_address(participant));
		NSString *uri = [NSString stringWithUTF8String:curi];
		[contactsArray addObject:uri];

		if(linphone_participant_is_admin(participant))
		   [admins addObject:uri];
		participants = participants->next;
		ms_free(curi);
	}
	ChatConversationInfoView *view = VIEW(ChatConversationInfoView);
	view.create = FALSE;
	view.contacts = [contactsArray mutableCopy];
	view.oldContacts = [contactsArray mutableCopy];
	view.admins = [admins mutableCopy];
	view.oldAdmins = [admins mutableCopy];
	view.oldSubject = [NSString stringWithUTF8String:linphone_chat_room_get_subject(_chatRoom) ?: LINPHONE_DUMMY_SUBJECT];
	view.room = _chatRoom;
	view.peerAddress = _peerAddress;
	view.localAddress = _localAddress;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onInfoClick:(id)sender {
	[self displayGroupInfo];
}

#pragma mark ChatRoomDelegate

- (BOOL)startMultiFilesUpload:(LinphoneChatMessage *)rootMessage {
	FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
	[fileTransfer setText:[self.messageField text]];
	[fileTransfer uploadFileContent:_fileContext forChatRoom:_chatRoom rootMessage:rootMessage];
	[_tableController scrollToBottom:true];
	return TRUE;
}

- (BOOL)startUploadData:(NSData *)data withType:(NSString*)type withName:(NSString *)name andMessage:(NSString *)message rootMessage:(LinphoneChatMessage *)rootMessage {
	FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
	if (message)
		[fileTransfer setText:message];
	NSString *key = @"localfile";
	if ([type isEqualToString:@"video"]) {
		key = @"localvideo";
	} else if ([type isEqualToString:@"image"]) {
		key = @"localimage";
	}
	[fileTransfer uploadData:data forChatRoom:_chatRoom type:type subtype:type name:name key:key rootMessage:rootMessage];
	[_tableController scrollToBottom:true];
	return TRUE;
}

- (BOOL)startFileUpload:(NSData *)data withName:(NSString *)name rootMessage:(LinphoneChatMessage *)rootMessage {
    FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
	[fileTransfer uploadFile:data forChatRoom:_chatRoom withName:name rootMessage:rootMessage];
    [_tableController scrollToBottom:true];
    return TRUE;
}

- (BOOL)resendMultiFiles:(FileContext *)newFileContext message:(NSString *)message rootMessage:(LinphoneChatMessage *)rootMessage {
	FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
	if (message)
		[fileTransfer setText:message];
	[fileTransfer uploadFileContent:newFileContext forChatRoom:_chatRoom rootMessage:rootMessage];
	[_tableController scrollToBottom:true];
	return TRUE;
}

- (BOOL)resendFile: (NSData *)data withName:(NSString *)name type:(NSString *)type key:(NSString *)key message:(NSString *)message rootMessage:(LinphoneChatMessage *)rootMessage{
	FileTransferDelegate *fileTransfer = [[FileTransferDelegate alloc] init];
	if (message)
		[fileTransfer setText:message];
	[fileTransfer uploadData:data forChatRoom:_chatRoom type:type subtype:type name:name key:key rootMessage:rootMessage];
	[_tableController scrollToBottom:true];
	return TRUE;
}

- (void)resendChat:(NSString *)message withExternalUrl:(NSString *)url rootMessage:(LinphoneChatMessage *)rootMessage  {
	[self sendMessage:message withExterlBodyUrl:[NSURL URLWithString:url] rootMessage:rootMessage];
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


- (void)imagePickerDelegateVideo:(NSURL*)url info:(NSDictionary *)info {
	NSURL * mediaURL = [info objectForKey:UIImagePickerControllerMediaURL];
	[SVProgressHUD show];
	AVAsset *video = [AVAsset assetWithURL:mediaURL];
	AVAssetExportSession *exportSession = [AVAssetExportSession exportSessionWithAsset:video presetName:AVAssetExportPresetMediumQuality];
	exportSession.shouldOptimizeForNetworkUse = YES;
	exportSession.outputFileType = AVFileTypeMPEG4;
	
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	NSString *documentsDirectory = [paths objectAtIndex:0];
	
	NSString *localname = [[[mediaURL absoluteString] md5] stringByAppendingString:@".mp4"];
	NSURL *compressedVideoUrl=[[NSURL fileURLWithPath:documentsDirectory] URLByAppendingPathComponent:localname];
	exportSession.outputURL = compressedVideoUrl;
	[exportSession exportAsynchronouslyWithCompletionHandler:^{
		dispatch_async(dispatch_get_main_queue(), ^{
			[SVProgressHUD dismiss];
			UIImage* image = [UIChatBubbleTextCell getImageFromVideoUrl:compressedVideoUrl];
			[_fileContext addObject:[NSData dataWithContentsOfURL:compressedVideoUrl] name:localname type:@"video" image:image];
			[self refreshImageDrawer];
		});
	}];
	
	BOOL saveToGallery = [ConfigManager.instance lpConfigBoolForKeyWithKey:@"auto_write_to_gallery_preference"];

	if (![info valueForKey:UIImagePickerControllerReferenceURL] && saveToGallery) {
			[self writeVideoToGallery:mediaURL];
	}
}

+ (void)writeMediaToGallery:(NSString *)name fileType:(NSString *)fileType {
	NSString *filePath = [LinphoneManager validFilePath:name];
	NSFileManager *fileManager = [NSFileManager defaultManager];
	if ([fileManager fileExistsAtPath:filePath]) {
		NSData* data = [NSData dataWithContentsOfFile:filePath];

		// define a block , not called immediately. To avoid crash when saving photo before PHAuthorizationStatusNotDetermined.
		void (^block)(void)= ^ {
			if ([fileType isEqualToString:@"image"] ) {
				// we're finished, save the image and update the message
				UIImage *image = [UIImage imageWithData:data];
				if (!image) {
					ChatConversationView *view = VIEW(ChatConversationView);
					[view showFileDownloadError];
					return;
				}
				__block PHObjectPlaceholder *placeHolder;
				[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
					PHAssetCreationRequest *request = [PHAssetCreationRequest creationRequestForAssetFromImage:image];
					placeHolder = [request placeholderForCreatedAsset];
				} completionHandler:^(BOOL success, NSError *error) {
					dispatch_async(dispatch_get_main_queue(), ^{
						if (error) {
							LOGE(@"Cannot save image data downloaded [%@]", [error localizedDescription]);
							UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Transfer error", nil)
								message:NSLocalizedString(@"Cannot write image to photo library",nil)
						 preferredStyle:UIAlertControllerStyleAlert];

							UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];

							[errView addAction:defaultAction];
							[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
						} else {
							LOGI(@"Image saved to [%@]", [placeHolder localIdentifier]);
						}
					});
				}];
			} else if([fileType isEqualToString:@"video"]) {
				__block PHObjectPlaceholder *placeHolder;
				[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
					PHAssetCreationRequest *request = [PHAssetCreationRequest creationRequestForAssetFromVideoAtFileURL:[NSURL fileURLWithPath:filePath]];
					placeHolder = [request placeholderForCreatedAsset];
				} completionHandler:^(BOOL success, NSError * _Nullable error) {
					dispatch_async(dispatch_get_main_queue(), ^{
						if (error) {
							LOGE(@"Cannot save video data downloaded [%@]", [error localizedDescription]);
							UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Transfer error", nil)
								 message:NSLocalizedString(@"Cannot write video to photo library", nil)
						  preferredStyle:UIAlertControllerStyleAlert];

							UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
																	style:UIAlertActionStyleDefault
																  handler:^(UIAlertAction * action) {}];
				
							[errView addAction:defaultAction];
							[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
						} else {
							LOGI(@"video saved to [%@]", [placeHolder localIdentifier]);
						}
					});
				}];
			}
		};

		// When you save an image or video to a photo library, make sure that it is allowed. Otherwise, there will be a backup error.
		if ([PHPhotoLibrary authorizationStatus] == PHAuthorizationStatusAuthorized) {
			block();
		} else {
			[PHPhotoLibrary requestAuthorization:^(PHAuthorizationStatus status) {
				dispatch_async(dispatch_get_main_queue(), ^{
					if ([PHPhotoLibrary authorizationStatus] == PHAuthorizationStatusAuthorized) {
						block();
					} else {
						[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Photo's permission", nil) message:NSLocalizedString(@"Photo not authorized", nil) delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Continue", nil] show];
					}
				});
			}];
		}
	}
}

-(void) writeVideoToGallery:(NSURL *)url {
	
	NSString *localIdentifier;
	PHFetchResult<PHAssetCollection *> *assetCollections = [PHAssetCollection fetchAssetCollectionsWithType:PHAssetCollectionTypeAlbum subtype:PHAssetCollectionSubtypeAlbumRegular options:nil];
	for (PHAssetCollection *assetCollection in assetCollections) {
		if([[assetCollection localizedTitle] isEqualToString:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"]]  ){
			localIdentifier = assetCollection.localIdentifier;
			break;
		}
	}
	if(localIdentifier ){
		PHFetchResult *fetchResult = [PHAssetCollection fetchAssetCollectionsWithLocalIdentifiers:@[localIdentifier] options:nil];
		PHAssetCollection *assetCollection = fetchResult.firstObject;
		
		[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
			PHAssetChangeRequest *assetChangeRequest = [PHAssetChangeRequest creationRequestForAssetFromVideoAtFileURL:url];
			
			PHAssetCollectionChangeRequest *assetCollectionChangeRequest = [PHAssetCollectionChangeRequest changeRequestForAssetCollection:assetCollection];
			[assetCollectionChangeRequest addAssets:@[[assetChangeRequest placeholderForCreatedAsset]]];
		} completionHandler:^(BOOL success, NSError *error) {
			if (!success) {
				NSLog(@"Error creating asset: %@", error);
			}
		}];
	}else{
		__block PHObjectPlaceholder *albumPlaceholder;
		[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
			PHAssetCollectionChangeRequest *changeRequest = [PHAssetCollectionChangeRequest creationRequestForAssetCollectionWithTitle:[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleDisplayName"]];
			albumPlaceholder = changeRequest.placeholderForCreatedAssetCollection;
		} completionHandler:^(BOOL success, NSError *error) {
			if (success) {
				PHFetchResult *fetchResult = [PHAssetCollection fetchAssetCollectionsWithLocalIdentifiers:@[albumPlaceholder.localIdentifier] options:nil];
				PHAssetCollection *assetCollection = fetchResult.firstObject;
				
				[[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
					PHAssetChangeRequest *assetChangeRequest = [PHAssetChangeRequest creationRequestForAssetFromVideoAtFileURL:url];
					PHAssetCollectionChangeRequest *assetCollectionChangeRequest = [PHAssetCollectionChangeRequest changeRequestForAssetCollection:assetCollection];
					[assetCollectionChangeRequest addAssets:@[[assetChangeRequest placeholderForCreatedAsset]]];
				} completionHandler:^(BOOL success, NSError *error) {
					if (!success) {
						NSLog(@"Error creating asset: %@", error);
					}
				}];
			} else {
				NSLog(@"Error creating album: %@", error);
			}
		}];
	}
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
            
            
            if ([_fileContext count] > 0){
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
		if (_showVoiceRecorderView)
				_vrView.hidden = true; // force recalculate
		if (_showReplyView)
			_replyView.hidden = true; // force recalculate
		[self updateFramesInclRecordingAndReplyView];
		
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
            
            if ([_fileContext count] > 0){
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
		if (_showVoiceRecorderView)
				_vrView.hidden = true; // force recalculate
		if (_showReplyView)
			_replyView.hidden = true; // force recalculate
		[self updateFramesInclRecordingAndReplyView];

            
		}
		completion:^(BOOL finished){
		}];
}

#pragma mark - chat room callbacks

void on_chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view configureMessageField];
}

void on_chat_room_subject_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	const char *subject = linphone_chat_room_get_subject(cr) ?: linphone_event_log_get_subject(event_log);
	if (subject) {
		view.addressLabel.text = [NSString stringWithUTF8String:subject];
		[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
		[view.tableController scrollToBottom:true];
		if (IPAD) {
			[VIEW(ChatsListView).tableController loadData];
		}
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
    UIImage *image = [FastAddressBook imageForSecurityLevel:linphone_chat_room_get_security_level(cr)];
    [view.encryptedButton setImage:image forState:UIControlStateNormal];
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
	
	bool isDisplayingBottomOfTable = [view.tableController.tableView indexPathsForVisibleRows].lastObject.row == [view.tableController totalNumberOfItems] - 1;
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	
	if (isDisplayingBottomOfTable) {
		[view.tableController scrollToBottom:TRUE];
	} else {
		[[view.tableController scrollBadge] setHidden:FALSE];
		int unread_msg = linphone_chat_room_get_unread_messages_count(cr);
		[[view.tableController scrollBadge] setText:[NSString stringWithFormat:@"%d", unread_msg]];
	}
}

void on_chat_room_chat_message_sending(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	[view.tableController addEventEntry:(LinphoneEventLog *)event_log];
	[view.tableController scrollToBottom:true];

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
    if (IPAD)
        [NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:nil];
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

void on_chat_room_conference_alert(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
    ChatConversationView *view = (__bridge ChatConversationView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
    [view.tableController addEventEntry:(LinphoneEventLog *)event_log];
    [view.tableController scrollToBottom:true];
    UIImage *image = [FastAddressBook imageForSecurityLevel:linphone_chat_room_get_security_level(cr)];
    [view.encryptedButton setImage:image forState:UIControlStateNormal];
}

- (void)openFileWithURLs:(NSMutableArray<NSURL *>*)urls index:(NSInteger)currentIndex
{
    //create the Quicklook controller.
    QLPreviewController *qlController = [[QLPreviewController alloc] init];
	self.FileDataSource = [[FileDataSource alloc] initWithFiles:urls];
    
    qlController.dataSource = self.FileDataSource;
	qlController.currentPreviewItemIndex = currentIndex;
    qlController.delegate = self;
    
    //present the document.
    [self presentViewController:qlController animated:YES completion:nil];
}

- (void)openFileWithURL:(NSURL *)url
{
	[self openFileWithURLs:[NSMutableArray arrayWithObject:url] index:0];
}

- (void)previewControllerDidDismiss:(QLPreviewController *)controller
{
    // QuickLook: When done button is pushed
    [PhoneMainView.instance fullScreen:NO];
}

+ (NSData *)getFileData: (NSString *)name {
	NSString *filePath = [LinphoneManager validFilePath:name];
	return [NSData dataWithContentsOfFile:filePath];
}

+ (NSURL *)getFileUrl: (NSString *)name {
	NSString *filePath = [LinphoneManager validFilePath:name];
	return [NSURL fileURLWithPath:filePath];
}

+ (void)writeFileInImagesDirectory:(NSData *)data name:(NSString *)name {
	NSString *filePath = [[LinphoneManager imagesDirectory] stringByAppendingPathComponent:name];
	if (name || [name isEqualToString:@""]) {
		LOGW(@"try to write file in %@", filePath);
	}
	[[NSFileManager defaultManager] createFileAtPath:filePath
												contents:data
											  attributes:nil];
}

- (NSURL *)getICloudFileUrl:(NSString *)name {
    if (@available(iOS 11.0, *)) {
        return [NSURL fileURLWithPath:[LinphoneManager documentFile:name]];
    }
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSURL *icloudPath = [[fileManager URLForUbiquityContainerIdentifier:nil] URLByAppendingPathComponent:@"Documents"];
    
    if (icloudPath) {
        if (![fileManager fileExistsAtPath:icloudPath.path isDirectory:nil]) {
            LOGI(@"Create directory");
            [fileManager createDirectoryAtURL:icloudPath withIntermediateDirectories:YES attributes:nil error:nil];
        }
        
        return [icloudPath URLByAppendingPathComponent:name];
    }
    
    return nil;
}

- (void)deleteFileWithUuid:(NSUUID *)uuid {
	[_fileContext deleteContentWithUuid:uuid];
	[self refreshImageDrawer];
}

- (void)clearMessageView {
    [_messageField setText:@""];
	if (!_fileContext) _fileContext = [[FileContext alloc] init];
	[_fileContext clear];
    
    [self refreshImageDrawer];
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section {
    return [_fileContext count];
}


- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath {
	return UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]) ? CGSizeMake(60, 60) : CGSizeMake(120, 120);
}

- (__kindof UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
    UIImageViewDeletable *imgView = [collectionView dequeueReusableCellWithReuseIdentifier:NSStringFromClass([UIImageViewDeletable class]) forIndexPath:indexPath];
    CGRect imgFrame = imgView.frame;
    imgFrame.origin.y = 5;
    if (UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation])) {
        imgFrame.size.height = 60;
    } else {
        imgFrame.size.height = 120;
    }

	[imgView.image setImage:[UIImage resizeImage:[_fileContext.previewsArray objectAtIndex:[indexPath item]] withMaxWidth:imgFrame.size.width andMaxHeight:imgFrame.size.height]];
	[imgView setUuid:[_fileContext.uuidsArray objectAtIndex:[indexPath item]]];
    [imgView setDeleteDelegate:self];
    [imgView setFrame:imgFrame];
	[self setSendButtonState];
    return imgView;
}

- (void)refreshImageDrawer {
    int heightDiff = UIInterfaceOrientationIsLandscape([[UIApplication sharedApplication] statusBarOrientation]) ? 65 : 125;
    
    if ([_fileContext count] == 0) {
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
							 [self updateFramesInclRecordingAndReplyView];
                         }
                         completion:nil];
		[self setSendButtonState];
    } else {
		// resizing imagesView
		CGRect imagesFrame = [_imagesView frame];
		imagesFrame.origin.y = [_messageView frame].origin.y - heightDiff;
		imagesFrame.size.height = heightDiff;
		[_imagesView setFrame:imagesFrame];
		// resizing chatTable
		CGRect tableViewFrame = [_tableController.tableView frame];
		tableViewFrame.size.height = imagesFrame.origin.y - tableViewFrame.origin.y;
		[_tableController.tableView setFrame:tableViewFrame];
		[self updateFramesInclRecordingAndReplyView];
		[_imagesCollectionView reloadData];
    }
}

- (void)showFileDownloadError {
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
}

+ (NSString *)getKeyFromFileType:(NSString *)fileType fileName:(NSString *)name{
	if ([fileType isEqualToString:@"video"]) {
		return @"localvideo";
	} else if ([fileType isEqualToString:@"image"] || [name hasSuffix:@"JPG"] || [name hasSuffix:@"PNG"] || [name hasSuffix:@"jpg"] || [name hasSuffix:@"png"]) {
		return @"localimage";
	}
	return @"localfile";
}

/* There are three cases: auto download in foreground, auto download in background, on click download*/
+ (void)autoDownload:(LinphoneChatMessage *)message {
	ChatConversationView *view = VIEW(ChatConversationView);
	LinphoneContent *content = linphone_chat_message_get_file_transfer_information(message);
	NSString *name = [NSString stringWithUTF8String:linphone_content_get_name(content)];
	NSString *fileType = [NSString stringWithUTF8String:linphone_content_get_type(content)];
	NSString *key = [ChatConversationView getKeyFromFileType:fileType fileName:name];

	[LinphoneManager setValueInMessageAppData:name forKey:key inMessage:message];
	dispatch_async(dispatch_get_main_queue(), ^{
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneMessageReceived object:view];
		if (![VFSUtil vfsEnabledWithGroupName:kLinphoneMsgNotificationAppGroupId] && [ConfigManager.instance lpConfigBoolForKeyWithKey:@"auto_write_to_gallery_preference"]) {
			[ChatConversationView writeMediaToGallery:name fileType:fileType];
		}
	});
}

-(void) documentMenu:(UIDocumentMenuViewController *)documentMenu didPickDocumentPicker:(UIDocumentPickerViewController *)documentPicker {
	documentPicker.delegate = self;
	[PhoneMainView.instance presentViewController:documentPicker animated:YES completion:nil];
}

-(void) documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentAtURL:(NSURL *)url {
	[url startAccessingSecurityScopedResource];
	NSFileCoordinator *co =[[NSFileCoordinator alloc] init];
	NSError *error = nil;
	[co coordinateReadingItemAtURL:url options:0 error:&error byAccessor:^(NSURL * _Nonnull newURL) {
		UIImage *image = [UIChatBubbleTextCell getImageFromFileName:[newURL lastPathComponent] forReplyBubble:false];
		[_fileContext addObject:[NSData dataWithContentsOfURL:newURL] name:[newURL lastPathComponent] type:@"file" image:image];
		[self refreshImageDrawer];
	}];
	[url stopAccessingSecurityScopedResource];
}

+(UIImage *)getBasicImage {
	UIColor *color=[UIColor grayColor];
	CGRect frame = CGRectMake(0, 0, 200, 200);
	UIGraphicsBeginImageContext(frame.size);
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGContextSetFillColorWithColor(context, [color CGColor]);
	CGContextFillRect(context, frame);
	UIImage *image = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return image;
}

+(UIImage*)drawText:(NSString*)text image:(UIImage *)image textSize:(CGFloat)textSize
{
	UIFont *font = [UIFont boldSystemFontOfSize:textSize];
	UIGraphicsBeginImageContext(image.size);
	[image drawInRect:CGRectMake(0,0,image.size.width,image.size.height)];
	CGRect rect = CGRectMake(0, 30, image.size.width, image.size.height);
	[[UIColor whiteColor] set];
	[text drawInRect:CGRectIntegral(rect) withFont:font];
	UIImage *newImage = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();

	return newImage;
}

// Popup menu
-(void) addOrGoToContact:(const LinphoneAddress *)contactAddress {
	Contact *contact = [FastAddressBook getContactWithAddress:contactAddress];
	
	if (contact) {
		ContactDetailsView *view = VIEW(ContactDetailsView);
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
		[ContactSelection setSelectionMode:ContactSelectionModeNone];
		[view setContact:contact];
	} else {
		char *lAddress = linphone_address_as_string_uri_only(contactAddress);
		if (lAddress != NULL) {
			NSString *normSip = [NSString stringWithUTF8String:lAddress];
			normSip = [normSip hasPrefix:@"sip:"] ? [normSip substringFromIndex:4] : normSip;
			normSip = [normSip hasPrefix:@"sips:"] ? [normSip substringFromIndex:5] : normSip;
			[ContactSelection setAddAddress:normSip];
			[ContactSelection setSelectionMode:ContactSelectionModeEdit];
			[ContactSelection enableSipFilter:FALSE];
			[PhoneMainView.instance changeCurrentView:ContactsListView.compositeViewDescription];
			ms_free(lAddress);
		}
	}
}

-(void) showAddressAndIdentityPopup {
	
	char *localAddress = linphone_address_as_string(linphone_chat_room_get_local_address(_chatRoom));
	char *peerAddress = linphone_address_as_string(linphone_chat_room_get_peer_address(_chatRoom));
	NSString *infoMsg = [NSString stringWithFormat:@"Chat room id:\n%s\nLocal account:\n%s", peerAddress, localAddress];
	ms_free(localAddress);
	ms_free(peerAddress);
	
	UIAlertController *popupView =
	[UIAlertController alertControllerWithTitle:NSLocalizedString(@"Chatroom debug infos", nil)
										message:infoMsg
								 preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Copy to clipboard", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction *action) {
		UIPasteboard *pasteboard = [UIPasteboard generalPasteboard];
		pasteboard.string = infoMsg;
	}];
	
	[popupView addAction:defaultAction];
	[self presentViewController:popupView animated:YES completion:nil];
	
}

-(BOOL) canAdminEphemeral:(const LinphoneChatRoom *)cr {
	if (!cr || !isEncrypted) return FALSE;
	
	// If ephemeral mode is DeviceManaged, then we don't need to check anything else
	return	(linphone_chat_room_params_get_ephemeral_mode(linphone_chat_room_get_current_params(cr)) == LinphoneChatRoomEphemeralModeDeviceManaged)
	||	( linphone_chat_room_has_capability(cr, LinphoneChatRoomCapabilitiesEphemeral) && linphone_chat_room_params_get_ephemeral_mode(linphone_chat_room_get_current_params(cr)) == LinphoneChatRoomEphemeralModeAdminManaged && linphone_participant_is_admin(linphone_chat_room_get_me(cr)));
}

- (void) setupPopupMenu {
	_popupMenu.dataSource = self;
	_popupMenu.delegate = self;
	_tableController.editButton.hidden = true;
	_popupMenu.layer.shadowColor = [UIColor lightGrayColor].CGColor;
	_popupMenu.layer.shadowOpacity = 0.5;
	_popupMenu.layer.shadowOffset = CGSizeZero;
	_popupMenu.layer.shadowRadius = 10;
	_popupMenu.layer.masksToBounds = false;
	_toggleMenuButton.hidden = false;
	_popupMenu.tableFooterView = [UIView new];
	_popupMenu.separatorStyle = UITableViewCellSeparatorStyleNone;
	[_popupMenu reloadData];
}

-(void) tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[self onToggleMenu:nil];
	
	int firstIndex = isOneToOne ? 0 : 1;
	
	if (!isOneToOne && indexPath.row == 0) { //Schedule meeting
		[ConferenceViewModelBridge scheduleFromGroupChatWithCChatRoom:_chatRoom];
		[PhoneMainView.instance popToView:ConferenceSchedulingView.compositeViewDescription];
	}

	if (indexPath.row == firstIndex) {
		if (isOneToOne) {
			if  (isEncrypted) {
				LinphoneAddress* contactAddress = linphone_address_clone(linphone_participant_get_address(bctbx_list_nth_data(linphone_chat_room_get_participants(_chatRoom), 0)));
				linphone_address_clean(contactAddress);
				[self addOrGoToContact:contactAddress];
				linphone_address_unref(contactAddress);
			} else {
				[self addOrGoToContact:linphone_chat_room_get_peer_address(_chatRoom)];
			}
		} else {
			[self displayGroupInfo];
		}
	}
	
	if (isEncrypted && indexPath.row == 1+firstIndex) {
		[self goToDeviceListView];
	}
	
	BOOL canEphemeral = [self canAdminEphemeral:_chatRoom];
	if (canEphemeral && indexPath.row == 2+firstIndex) {
		EphemeralSettingsView *view = VIEW(EphemeralSettingsView);
		view.room = _chatRoom;
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	}
	if ((!isEncrypted && indexPath.row == 1+firstIndex) || (isEncrypted && indexPath.row == 3+firstIndex)) {
		[LinphoneManager setChatroomPushEnabled:_chatRoom withPushEnabled:![LinphoneManager getChatroomPushEnabled:_chatRoom]];
		[_popupMenu reloadData];
	}
	
	if ((!isEncrypted && indexPath.row == 2+firstIndex) || (isEncrypted && indexPath.row == 4+firstIndex)) {
		[_tableController onEditClick:nil];
		[self onEditionChangeClick:nil];
	}
	
	if ((isEncrypted && ((!canEphemeral && indexPath.row == 4+firstIndex)||(canEphemeral && indexPath.row == 5+firstIndex)))
		|| (!isEncrypted && indexPath.row == 3+firstIndex)) {
		[self showAddressAndIdentityPopup];
	}
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	int nbRows = 3;
	
	if ([LinphoneManager.instance lpConfigIntForKey:@"debugenable_preference"] == 1) // DEBOGUE == ALL
		++nbRows;
	
	if (!isEncrypted)
		return nbRows;
	else
		++nbRows;
	
	if ([self canAdminEphemeral:_chatRoom])
		++nbRows;
	
	if (!isOneToOne) // schedule meeting
		++nbRows;
	
	return nbRows;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [[UITableViewCell alloc] init];
	
	if (!_chatRoom) {
		// Workaround to avoid crash in background for release 4.7. This shouldn't happen though, so there may be a deeper issue not found yet
		return cell;
	}
	
	int firstIndex = isOneToOne ? 0 : 1;
	
	if (!isOneToOne && indexPath.row == 0) {
		cell.imageView.image = [UIImage imageNamed:@"menu_voip_meeting_schedule"];
		cell.textLabel.text = NSLocalizedString(@"Schedule a meeting",nil);
	}
	
	if (indexPath.row == firstIndex) {
		if (isOneToOne) {
			Contact *contact;
			if  (isEncrypted) {
				LinphoneAddress * contactAddress = linphone_address_clone(linphone_participant_get_address(bctbx_list_nth_data(linphone_chat_room_get_participants(_chatRoom), 0)));
				linphone_address_clean(contactAddress);
				contact = [FastAddressBook getContactWithAddress:contactAddress];
				linphone_address_unref(contactAddress);
			} else {
				contact = [FastAddressBook getContactWithAddress:linphone_chat_room_get_peer_address(_chatRoom)];
			}
			if (contact == nil) {
				cell.imageView.image = [UIImage imageNamed:@"contact_add_default.png"];
				cell.textLabel.text = NSLocalizedString(@"Add to contacts",nil);
			} else {
				cell.imageView.image = [UIImage imageNamed:@"contacts_all_default.png"];
				cell.textLabel.text = NSLocalizedString(@"Go to contact",nil);
			}
		} else {
			cell.imageView.image = [UIImage imageNamed:@"chat_group_informations.png"];
			cell.textLabel.text = NSLocalizedString(@"Group infos",nil);
		}
	}
	
	if (isEncrypted && indexPath.row == 1+firstIndex) {
		cell.imageView.image = [UIImage imageNamed:@"menu_security_default.png"];
		cell.textLabel.text = NSLocalizedString(@"Conversation's devices",nil);
	}
	
	bool canEphemeral = [self canAdminEphemeral:_chatRoom];
	if (canEphemeral && indexPath.row == 2+firstIndex) {
		cell.imageView.image =  [UIImage imageNamed:@"ephemeral_messages_default.png"];
		cell.textLabel.text = NSLocalizedString(@"Ephemeral messages",nil);
	}
	
	if ((isEncrypted && indexPath.row == 3+firstIndex) || (!isEncrypted && indexPath.row == 1+firstIndex)) {
		if ([LinphoneManager getChatroomPushEnabled:_chatRoom]) {
			cell.imageView.image = [UIImage imageNamed:@"menu_notifications_off.png"];
			cell.textLabel.text = NSLocalizedString(@"Mute notifications",nil);
		} else {
			cell.imageView.image = [UIImage imageNamed:@"menu_notifications_on.png"];
			cell.textLabel.text = NSLocalizedString(@"Un-mute notifications",nil);
		}
	}
	
	if ((isEncrypted && indexPath.row == 4+firstIndex) || (!isEncrypted && indexPath.row == 2+firstIndex)) {
		cell.imageView.image =  [UIImage imageNamed:@"delete_default.png"];
		cell.textLabel.text = NSLocalizedString(@"Delete messages",nil);
	}
	
	if ((isEncrypted && ((!canEphemeral && indexPath.row == 4+firstIndex)||(canEphemeral && indexPath.row == 5+firstIndex)))
		|| (!isEncrypted && indexPath.row == 3+firstIndex)) {
		cell.imageView.image =  [UIImage imageNamed:@"chat_group_informations.png"];
		cell.textLabel.text = NSLocalizedString(@"Debug infos",nil);
	}
	
	UIImageView * icon = [[UIImageView alloc] initWithFrame:CGRectMake(tableView.frame.size.width-37, 7, 30, 30)];
	icon.contentMode = UIViewContentModeScaleAspectFit;
	icon.image =  cell.imageView.image;
	[cell.contentView addSubview:icon];
	cell.imageView.image = nil;
	
	return cell;
}
- (IBAction)onToggleMenu:(id)sender {
	_popupMenu.hidden = !_popupMenu.hidden;
	if (!_popupMenu.hidden)
		[_popupMenu selectRowAtIndexPath:nil animated:false scrollPosition:UITableViewScrollPositionNone];
}

// Voice recording

- (IBAction)onVrDelete:(id)sender {
	[self cancelVoiceRecording];
	[self stopVoiceRecordPlayer];
}

- (IBAction)onvrPlayPauseStop:(id)sender {
	if (_isVoiceRecording) {
		[self stopVoiceRecording];
	} else {
		if (_isPlayingVoiceRecording)
			[self stopVoiceRecordPlayer];
		else
			[self playRecordedMessage];
	}
}

- (IBAction)onVrStart:(id)sender {
	if (_isVoiceRecording) {
		[self stopVoiceRecording];
	} else {
		[self startVoiceRecording];
	}
}

-(void) createVoiceRecorder {
	LinphoneRecorderParams *p = linphone_core_create_recorder_params(LC);
	linphone_recorder_params_set_file_format(p, LinphoneRecorderFileFormatMkv);
	_voiceRecorder = linphone_core_create_recorder(LC, p);
}

-(void) cancelVoiceRecording {
	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
	_showVoiceRecorderView = false;
	_toggleRecord.selected = false;
	[self updateFramesInclRecordingAndReplyView];
	_isPendingVoiceRecord = false;
	_isVoiceRecording = false;
	if (_voiceRecorder && linphone_recorder_get_state(_voiceRecorder) != LinphoneRecorderClosed) {
		linphone_recorder_close(_voiceRecorder);
		const char *recordingFile = linphone_recorder_get_file(_voiceRecorder);
		if (recordingFile) {
			[AppManager removeFileWithFile:[NSString stringWithUTF8String:recordingFile]];
		}
	}
	[self setSendButtonState];
}

-(void) stopVoiceRecording {
	[[UIApplication sharedApplication] setIdleTimerDisabled:false];
	if (_voiceRecorder && linphone_recorder_get_state(_voiceRecorder) == LinphoneRecorderRunning) {
		LOGI(@"[Chat Message Sending] Pausing / closing voice recorder");
		linphone_recorder_pause(_voiceRecorder);
		linphone_recorder_close(_voiceRecorder);
		_vrDurationLabel.text = [self formattedDuration:linphone_recorder_get_duration(_voiceRecorder)];
	}
	_isVoiceRecording = false;
	if ([LinphoneManager.instance lpConfigBoolForKey:@"voice_recording_send_right_away" withDefault:false]) {
		[self onSendClick:nil];
	}
	[_vrPlayButton setImage:[UIImage imageNamed:@"vr_play"] forState:UIControlStateNormal];
	_toggleRecord.selected = false;
	_vrWaveMask.frame = CGRectZero;
	[_vrRecordTimer invalidate];
	_isPendingVoiceRecord = linphone_recorder_get_duration(_voiceRecorder) > 0;
	[self setSendButtonState];

}

-(void) startVoiceRecording {
	[[UIApplication sharedApplication] setIdleTimerDisabled:true];
	
	if (!_voiceRecorder)
		[self createVoiceRecorder];
	[CallManager.instance activateAudioSession];
	_toggleRecord.selected = true;
	[_vrPlayButton setImage:[UIImage imageNamed:@"vr_stop"] forState:UIControlStateNormal];
	
	
	_showVoiceRecorderView = true;
	[self updateFramesInclRecordingAndReplyView];
	_isVoiceRecording = true;
	_vrWaveMaskPlayer.frame = CGRectZero;
	
	switch (linphone_recorder_get_state(_voiceRecorder)) {
		case LinphoneRecorderClosed: {
			NSString *filename = [NSString stringWithFormat:@"%@/voice-recording-%@.mkv",[LinphoneManager imagesDirectory], [NSUUID UUID].UUIDString];
			linphone_recorder_open(_voiceRecorder, filename.UTF8String);
			linphone_recorder_start(_voiceRecorder);
			LOGW(@"[Chat Message Sending] Recorder is closed opening it with %@",filename);
			break;
		};
		case LinphoneRecorderRunning: {
			LOGW(@"[Chat Message Sending] Recorder is already recording");
			break;
		}
		case LinphoneRecorderPaused: {
			LOGW(@"[Chat Message Sending] Recorder isn't closed, resuming recording");
			linphone_recorder_start(_voiceRecorder);
		}
	}
	_vrWaveMask.frame = _vrWave.frame;
	_vrDurationLabel.text = [self formattedDuration:linphone_recorder_get_duration(_voiceRecorder)];
	_vrRecordTimer =  [NSTimer scheduledTimerWithTimeInterval:1.0
													target:self
												  selector:@selector(voiceRecordTimerUpdate)
												  userInfo:nil
												   repeats:YES];
	
	
}

-(void) voiceRecordTimerUpdate {
	int recorderDuration = linphone_recorder_get_duration(_voiceRecorder);
	if (recorderDuration > [LinphoneManager.instance lpConfigIntForKey:@"voice_recording_max_duration" withDefault:60000]) {
		LOGW(@"[Chat Message Sending] Max duration for voice recording exceeded, stopping. (max = %d)",[LinphoneManager.instance lpConfigIntForKey:@"voice_recording_max_duration" withDefault:60000]);
		[self stopVoiceRecording];
	} else {
		_vrDurationLabel.text = [self formattedDuration:linphone_recorder_get_duration(_voiceRecorder)];
		CGRect r = _vrWaveMask.frame;
		r.origin.x += 30;
		r.size.width -= 30;
		if (r.origin.x > _vrWave.frame.size.width) {
			r = _vrWave.frame;
			_vrWaveMask.frame = r;
		} else {
			[UIView animateWithDuration:1.0 delay:0.0 options:UIViewAnimationOptionCurveLinear animations:^{
				_vrWaveMask.frame = r;
				}completion:^(BOOL finished) {}];
		}
	}
}

// Playback Shared Player (new recording & chat bubble)

- (void) initSharedPlayer {
	LOGI(@"[Voice Message] Creating shared player");
	_sharedVoicePlayer = linphone_core_create_local_player(LC, [CallManager.instance getSpeakerSoundCard].UTF8String, nil, nil);
	LinphonePlayerCbs *cbs = linphone_factory_create_player_cbs(linphone_factory_get());
	linphone_player_cbs_set_eof_reached(cbs, on_shared_player_eof_reached);
	linphone_player_cbs_set_user_data(cbs, (__bridge void*)self);
	linphone_player_add_callbacks(_sharedVoicePlayer, cbs);
}

-(void) startSharedPlayer:(const char *)path {
	LOGI(@"[Voice Message] Starting shared player path = %s",path);
	if (linphone_player_get_user_data(_sharedVoicePlayer)) {
		LOGI(@"[Voice Message] a play was requested (%s), but there is already one going (%s)",path,(const char *)linphone_player_get_user_data(_sharedVoicePlayer) );
		NSDictionary* userInfo = @{@"path": [NSString stringWithUTF8String:linphone_player_get_user_data(_sharedVoicePlayer)]};
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneVoiceMessagePlayerLostFocus object:nil userInfo:userInfo];
	}
	[CallManager.instance changeRouteToSpeaker];
	linphone_player_set_user_data(_sharedVoicePlayer, (void *)path);
	linphone_player_open(_sharedVoicePlayer, path);
	linphone_player_start(_sharedVoicePlayer);
}

-(void) stopSharedPlayer {
	LOGI(@"[Voice Message] Stopping shared player path = %s",linphone_player_get_user_data(_sharedVoicePlayer) ? (const char *)linphone_player_get_user_data(_sharedVoicePlayer) : "nil");
	linphone_player_pause(_sharedVoicePlayer);
	linphone_player_seek(_sharedVoicePlayer,0);
	linphone_player_close(_sharedVoicePlayer);
	linphone_player_set_user_data(_sharedVoicePlayer, nil);
}

-(BOOL) sharedPlayedIsPlaying:(const char *)path {
	return path && linphone_player_get_user_data(_sharedVoicePlayer) && !strcmp(path,linphone_player_get_user_data(_sharedVoicePlayer));
}

void on_shared_player_eof_reached(LinphonePlayer *p) {
	LOGI(@"[Voice Message] End of file reached for player");
	const char * currentPlayedFile = (const char *) linphone_player_get_user_data(p);
	if (currentPlayedFile) {
		NSDictionary* userInfo = @{@"path": [NSString stringWithUTF8String:currentPlayedFile]};
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneVoiceMessagePlayerEOF object:nil userInfo:userInfo];
	}
}

// Playback of new recordings

-(void) playRecordedMessage {
	[_vrPlayButton setImage:[UIImage imageNamed:@"vr_stop"] forState:UIControlStateNormal];
	_vrDurationLabel.text =  [self formattedDuration:linphone_player_get_duration(_sharedVoicePlayer)];
	_vrWaveMask.frame = CGRectZero;
	CGRect r = CGRectZero;
	r.size.height = _vrInnerView.frame.size.height;
	_vrWaveMaskPlayer.frame = r;
	_vrPlayerTimer =  [NSTimer scheduledTimerWithTimeInterval:1.0
													target:self
												  selector:@selector(voicePlayTimerUpdate)
												  userInfo:nil
												   repeats:YES];
	[self startSharedPlayer:linphone_recorder_get_file(_voiceRecorder)];
	[self animPlayerOnce];
	_isPlayingVoiceRecording = true;
}

-(void) voicePlayTimerUpdate {
	_vrDurationLabel.text =  [self formattedDuration:linphone_player_get_duration(_sharedVoicePlayer)];
	[self animPlayerOnce];
}

-(void) animPlayerOnce {
	CGRect r = _vrWaveMaskPlayer.frame;
	r.size.width += _vrInnerView.frame.size.width / ((linphone_player_get_duration(_sharedVoicePlayer) / 1000)+1) ;
	if (r.size.width > _vrInnerView.frame.size.width) {
		r.size.width = _vrInnerView.frame.size.width;
	}
	[UIView animateWithDuration:1.0 delay:0.0 options:UIViewAnimationOptionCurveLinear animations:^{
		_vrWaveMaskPlayer.frame = r;
		}completion:^(BOOL finished) {}];
}

-(void) endVoicePlayingIfDoingSO:(NSNotification *)notif {
	if (_isPlayingVoiceRecording)
		[self stopVoiceRecordPlayer];
}

-(void) stopVoiceRecordPlayer {
	[self stopSharedPlayer];
	[_vrPlayButton setImage:[UIImage imageNamed:@"vr_play"] forState:UIControlStateNormal];
	_isPlayingVoiceRecording = false;
	[_vrPlayerTimer invalidate];
	_vrWaveMaskPlayer.frame = CGRectZero;
}

-(NSString *)formattedDuration:(long)valueMs {
	return [NSString stringWithFormat:@"%02ld:%02ld", valueMs/ 60000, (valueMs % 60000) / 1000 ];
}

-(void) updateFramesInclRecordingAndReplyView { // place below the messages table.
	BOOL showHideVoice = _showVoiceRecorderView != !_vrView.hidden;
	if (showHideVoice)
		_vrView.hidden = !_showVoiceRecorderView;
	
	CGRect vrFrame = _vrView.frame;
	CGRect tableFrame = _tableController.tableView.frame;
	if (showHideVoice) {
		tableFrame.size.height = _showVoiceRecorderView ? tableFrame.size.height  - vrFrame.size.height : tableFrame.size.height + vrFrame.size.height;
		_tableController.tableView.frame = tableFrame;
		[_tableController.tableView reloadData];
	}
	vrFrame.origin.y = tableFrame.origin.y+tableFrame.size.height;
	_vrView.frame = vrFrame;
	
	BOOL showHideReply = _showReplyView != !_replyView.hidden;
	if (showHideReply)
		_replyView.hidden = !_showReplyView;
	
	CGRect repFrame = _replyView.frame;
	tableFrame = _tableController.tableView.frame;
	if (showHideReply) {
		tableFrame.size.height = _showReplyView ? tableFrame.size.height  - repFrame.size.height : tableFrame.size.height + repFrame.size.height;
		_tableController.tableView.frame = tableFrame;
		[_tableController.tableView reloadData];
	}
	repFrame.origin.y = _showVoiceRecorderView ? vrFrame.origin.y + vrFrame.size.height : tableFrame.origin.y+tableFrame.size.height;
	_replyView.frame = repFrame;
	
}

-(void) stopAllPlays {
	if (linphone_player_get_user_data(_sharedVoicePlayer)) {
		NSDictionary* userInfo = @{@"path": [NSString stringWithUTF8String:linphone_player_get_user_data(_sharedVoicePlayer)]};
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneVoiceMessagePlayerLostFocus object:nil userInfo:userInfo];
	}
}

// send button state

-(void) setSendButtonState {
	_sendButton.enabled = !_isVoiceRecording && ((_isPendingVoiceRecord && linphone_recorder_get_duration(_voiceRecorder) > 0)  || [[_messageField text] length] > 0 || _fileContext.count  > 0);
}


// Reply

-(void) closePendingReply {
	if (_replyBubble != nil) {
		_showReplyView = false;
		[_replyBubble.view removeFromSuperview];
		[self updateFramesInclRecordingAndReplyView];
		_replyBubble = nil;
	}
}

-(void) initiateReplyViewForMessage:(LinphoneChatMessage *)message {
	if (_replyBubble != nil)
		[self closePendingReply];
	_replyBubble = [[UIChatReplyBubbleView alloc] initWithNibName:@"UIChatReplyBubbleView" bundle:nil];
	[self addChildViewController:_replyBubble];
	[_replyView addSubview:_replyBubble.view];
	[_replyBubble didMoveToParentViewController:self];
	[_replyBubble configureForMessage:message withDimissBlock:^{
		[self closePendingReply];
	} hideDismiss:false withClickBlock:^{}];
	_showReplyView = true;
	[self updateFramesInclRecordingAndReplyView];
	[self.tableController scrollToMessage:message];
	[self.messageField becomeFirstResponder];
}

-(void) handlePendingTransferIfAny {
	if (self.pendingForwardMessage) {
		LinphoneChatMessage *message = self.pendingForwardMessage;
		self.pendingForwardMessage = nil;
		UIConfirmationDialog *d = [UIConfirmationDialog ShowWithMessage:NSLocalizedString(@"Transfer this message to this conversation ?",nil)
														  cancelMessage:nil
														 confirmMessage:NSLocalizedString(@"TRANSFER",nil)
														  onCancelClick:^() {}
													onConfirmationClick:^() {
			linphone_chat_message_send(linphone_chat_room_create_forward_message(_chatRoom, message));
			
		}];
		d.forwardImage.hidden = NO;
		[d setSpecialColor];
	}
}


@end
