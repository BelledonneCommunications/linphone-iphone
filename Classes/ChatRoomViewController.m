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

#import "ChatRoomViewController.h"
#import "PhoneMainView.h"
#import "DTActionSheet.h"
#import "UILinphone.h"

#import <NinePatch.h>
#import <MobileCoreServices/UTCoreTypes.h>
#import "Utils.h"

@implementation ChatRoomViewController

@synthesize tableController;
@synthesize sendButton;
@synthesize messageField;
@synthesize editButton;
@synthesize addressLabel;
@synthesize composeLabel;
@synthesize composeIndicatorView;
@synthesize avatarImage;
@synthesize headerView;
@synthesize chatView;
@synthesize messageView;
@synthesize messageBackgroundImage;
@synthesize transferBackgroundImage;
@synthesize listTapGestureRecognizer;
@synthesize listSwipeGestureRecognizer;
@synthesize pictureButton;
@synthesize imageTransferProgressBar;
@synthesize cancelTransferButton;
@synthesize transferView;
@synthesize waitView;

#pragma mark - Lifecycle Functions

- (id)init {
    self = [super initWithNibName:@"ChatRoomViewController" bundle:[NSBundle mainBundle]];
    if (self != nil) {
        self->scrollOnGrowingEnabled = TRUE;
        self->chatRoom = NULL;
        self->imageSharing = NULL;
        self->listTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onListTap:)];
        self.listSwipeGestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(onListSwipe:)];
        self->imageQualities = [[OrderedDictionary alloc] initWithObjectsAndKeys:
                                [NSNumber numberWithFloat:0.9], NSLocalizedString(@"Maximum", nil),
                                [NSNumber numberWithFloat:0.5], NSLocalizedString(@"Average", nil),
                                [NSNumber numberWithFloat:0.0], NSLocalizedString(@"Minimum", nil), nil];
        self->composingVisible = TRUE;
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [tableController release];
    [messageField release];
    [sendButton release];
    [editButton release];
    [addressLabel release];
    [avatarImage release];
    [headerView release];
    [messageView release];
    [messageBackgroundImage release];
    [transferBackgroundImage release];

    [listTapGestureRecognizer release];
    [listSwipeGestureRecognizer release];

	[transferView release];
	[pictureButton release];
	[imageTransferProgressBar release];
	[cancelTransferButton release];

    [imageQualities release];
    [waitView release];

    [composeLabel release];
    [composeIndicatorView release];
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"ChatRoom"
                                                                content:@"ChatRoomViewController"
                                                               stateBar:nil
                                                        stateBarEnabled:false
                                                                 tabBar:/*@"UIMainBar"*/nil
                                                          tabBarEnabled:false /*to keep room for chat*/
                                                             fullscreen:false
                                                          landscapeMode:true
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    [tableController setChatRoomDelegate:self];

    // Set selected+over background: IB lack !
    [editButton setBackgroundImage:[UIImage imageNamed:@"chat_ok_over.png"]
                forState:(UIControlStateHighlighted | UIControlStateSelected)];

    [LinphoneUtils buttonFixStates:editButton];

    messageField.minNumberOfLines = 1;
	messageField.maxNumberOfLines = ([LinphoneManager runningOnIpad])?10:3;
    messageField.delegate = self;
	messageField.font = [UIFont systemFontOfSize:18.0f];
    messageField.contentInset = UIEdgeInsetsMake(0, -5, -2, -5);
    messageField.internalTextView.scrollIndicatorInsets = UIEdgeInsetsMake(0, 0, 0, 10);
    messageField.backgroundColor = [UIColor clearColor];
    [sendButton setEnabled:FALSE];

    [tableController.tableView addGestureRecognizer:listTapGestureRecognizer];
    [listTapGestureRecognizer setEnabled:FALSE];

    listSwipeGestureRecognizer.direction = UISwipeGestureRecognizerDirectionRight;
    [tableController.tableView addGestureRecognizer:listSwipeGestureRecognizer];
    listSwipeGestureRecognizer.enabled = TRUE;

    [tableController.tableView setBackgroundColor:[UIColor clearColor]]; // Can't do it in Xib: issue with ios4
    [tableController.tableView setBackgroundView:nil];
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillEnterForeground:)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillShow:)
                                                 name:UIKeyboardWillShowNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillHide:)
                                                 name:UIKeyboardWillHideNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textReceivedEvent:)
                                                 name:kLinphoneTextReceived
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(onMessageChange:)
												 name:UITextViewTextDidChangeNotification
											   object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textComposeEvent:)
                                                 name:kLinphoneTextComposeEvent
                                               object:nil];
	if([tableController isEditing])
        [tableController setEditing:FALSE animated:FALSE];
    [editButton setOff];
    [[tableController tableView] reloadData];

    [messageBackgroundImage setImage:[TUNinePatchCache imageOfSize:[messageBackgroundImage bounds].size
                                               forNinePatchNamed:@"chat_message_background"]];

	BOOL fileSharingEnabled = [[LinphoneManager instance] lpConfigStringForKey:@"sharing_server_preference"] != NULL
								&& [[[LinphoneManager instance] lpConfigStringForKey:@"sharing_server_preference"] length]>0;
    [pictureButton setEnabled:fileSharingEnabled];
    [waitView setHidden:TRUE];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];

    if(imageSharing) {
        [imageSharing cancel];
    }

    [messageField resignFirstResponder];

    [self setComposingVisible:FALSE withDelay:0]; // will hide the "user is composing.." message

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationDidBecomeActiveNotification
                                                  object:nil];

    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillShowNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillHideNotification
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneTextReceived
                                                    object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UITextViewTextDidChangeNotification
												  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneTextComposeEvent
												  object:nil];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
    [messageBackgroundImage setImage:[TUNinePatchCache imageOfSize:[messageBackgroundImage bounds].size
                                                 forNinePatchNamed:@"chat_message_background"]];
    [tableController scrollToBottom:true];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];

}

-(void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    [TUNinePatchCache flushCache]; // will remove any images cache (freeing any cached but unused images)
}


#pragma mark -

- (void)setChatRoom:(LinphoneChatRoom *)room {
    self->chatRoom = room;
    [messageField setText:@""];
	[tableController setChatRoom:room];
    [self update];
    linphone_chat_room_mark_as_read(chatRoom);
    [self setComposingVisible:linphone_chat_room_is_remote_composing(chatRoom) withDelay:0];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
}

- (void)applicationWillEnterForeground:(NSNotification*)notif {
    if(chatRoom != nil) {
        linphone_chat_room_mark_as_read(chatRoom);
        [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
    }
}

- (void)update {
    if(chatRoom == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room header: null contact"];
        return;
    }

    NSString *displayName = nil;
    UIImage *image = nil;
	const LinphoneAddress* linphoneAddress = linphone_chat_room_get_peer_address(chatRoom);
	if (linphoneAddress == NULL) {
        [[PhoneMainView instance] popCurrentView];
		UIAlertView* error = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Invalid SIP address",nil)
														message:NSLocalizedString(@"Either configure a SIP proxy server from settings prior to send a message or use a valid SIP address (I.E sip:john@example.net)",nil)
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
											  otherButtonTitles:nil];
		[error show];
		[error release];
        return;
    }
	char *tmp = linphone_address_as_string_uri_only(linphoneAddress);
	NSString *normalizedSipAddress = [NSString stringWithUTF8String:tmp];
	ms_free(tmp);

    ABRecordRef acontact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
    if(acontact != nil) {
        displayName = [FastAddressBook getContactDisplayName:acontact];
        image = [FastAddressBook getContactImage:acontact thumbnail:true];
    }

    // Display name
    if(displayName == nil) {
		const char* username = linphone_address_get_username(linphoneAddress);
		char* address = linphone_address_as_string(linphoneAddress);
		displayName = [NSString stringWithUTF8String:username?:address];
		ms_free(address);
    }
    [addressLabel setText:displayName];
    [addressLabel setAccessibilityValue:displayName];

    // Avatar
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown_small.png"];
    }
    [avatarImage setImage:image];

}

static void message_status(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud) {
	ChatRoomViewController* thiz = (ChatRoomViewController*)ud;
    const char*text = linphone_chat_message_get_text(msg);
	[LinphoneLogger log:LinphoneLoggerLog
				 format:@"Delivery status for [%s] is [%s]",text,linphone_chat_message_state_to_string(state)];
	[thiz.tableController updateChatEntry:msg];
}

- (BOOL)sendMessage:(NSString *)message withExterlBodyUrl:(NSURL*)externalUrl withInternalURL:(NSURL*)internalUrl {
    if(chatRoom == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot send message: No chatroom"];
        return FALSE;
    }

    LinphoneChatMessage* msg = linphone_chat_room_create_message(chatRoom, [message UTF8String]);
    if(externalUrl) {
        linphone_chat_message_set_external_body_url(msg, [[externalUrl absoluteString] UTF8String]);
    }

	linphone_chat_room_send_message2(chatRoom, msg, message_status, self);

    if ( internalUrl ) {
        // internal url is saved in the appdata for display and later save
        [LinphoneManager setValueInMessageAppData:[internalUrl absoluteString] forKey:@"localimage" inMessage:msg];
    }

    [tableController addChatEntry:msg];
    [tableController scrollToBottom:true];
    return TRUE;
}

- (void)saveAndSend:(UIImage*)image url:(NSURL*)url {
    if(url == nil) {
        [waitView setHidden:FALSE];
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [[LinphoneManager instance].photoLibrary
             writeImageToSavedPhotosAlbum:image.CGImage
             orientation:(ALAssetOrientation)[image imageOrientation]
             completionBlock:^(NSURL *assetURL, NSError *error){
                 dispatch_async(dispatch_get_main_queue(), ^{
                     [waitView setHidden:TRUE];
                     if (error) {
                         [LinphoneLogger log:LinphoneLoggerError format:@"Cannot save image data downloaded [%@]", [error localizedDescription]];

                         UIAlertView* errorAlert = [[UIAlertView alloc]	initWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                                              message:NSLocalizedString(@"Cannot write image to photo library", nil)
                                                                             delegate:nil
                                                                    cancelButtonTitle:NSLocalizedString(@"Ok",nil)
                                                                    otherButtonTitles:nil ,nil];
                         [errorAlert show];
                         [errorAlert release];
                         return;
                     }
                     [LinphoneLogger log:LinphoneLoggerLog format:@"Image saved to [%@]", [assetURL absoluteString]];
                     [self chatRoomStartImageUpload:image url:assetURL];
                 });
             }];
        });
    } else {
        [self chatRoomStartImageUpload:image url:url];
    }
}

- (void)chooseImageQuality:(UIImage*)image url:(NSURL*)url {
    [waitView setHidden:FALSE];

    DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Choose the image size", nil)];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        //UIImage *image = [original_image normalizedImage];
        for(NSString *key in [imageQualities allKeys]) {
            NSNumber *number = [imageQualities objectForKey:key];
            NSData *data = UIImageJPEGRepresentation(image, [number floatValue]);
            NSNumber *size = [NSNumber numberWithInteger:[data length]];

            NSString *text = [NSString stringWithFormat:@"%@ (%@)", key, [size toHumanReadableSize]];
            [sheet addButtonWithTitle:text block:^(){
                [self saveAndSend:[UIImage imageWithData:data] url:url];
            }];
        }
        [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel", nil) block:nil];
        dispatch_async(dispatch_get_main_queue(), ^{
            [waitView setHidden:TRUE];
            [sheet showInView:[PhoneMainView instance].view];
        });
    });
}

- (void)setComposingVisible:(BOOL)visible withDelay:(CGFloat)delay {

    if( composingVisible == visible ) return;

    CGRect keyboardFrame     = [self.messageView frame];
    CGRect newComposingFrame = [self.composeIndicatorView frame];
    CGRect newTableFrame     = [self.tableController.tableView frame];

    if( visible ){
        [composeLabel setText:[NSString stringWithFormat:NSLocalizedString(@"%@ is composing...", @""), [addressLabel text]]];
        // pull up the composing frame and shrink the table view

        newTableFrame.size.height -= newComposingFrame.size.height;
        newComposingFrame.origin.y = keyboardFrame.origin.y - newComposingFrame.size.height;
    } else {
        // pull down the composing frame and widen the tableview
        newTableFrame.size.height += newComposingFrame.size.height;
        newComposingFrame.origin.y = keyboardFrame.origin.y;
    }
    composingVisible = visible;
    [UIView animateWithDuration:delay
                     animations:^{
                         self.tableController.tableView.frame = newTableFrame;
                         self.composeIndicatorView.frame      = newComposingFrame;
                     }
                     completion:^(BOOL finished) {
                         [self.tableController scrollToBottom:TRUE];
                     }];
}


#pragma mark - Event Functions

- (void)textReceivedEvent:(NSNotification *)notif {
    LinphoneAddress * from    = [[[notif userInfo] objectForKey:@"from_address"] pointerValue];
    LinphoneChatRoom* room    = [[notif.userInfo objectForKey:@"room"] pointerValue];
    LinphoneChatMessage* chat = [[notif.userInfo objectForKey:@"message"] pointerValue];

    if(from == NULL || chat == NULL) {
        return;
    }
    char *fromStr = linphone_address_as_string_uri_only(from);
    const LinphoneAddress* cr_from = linphone_chat_room_get_peer_address(chatRoom);
    char* cr_from_string = linphone_address_as_string_uri_only(cr_from);

    if(fromStr && cr_from_string ) {

        if(strcasecmp(cr_from_string, fromStr) == 0) {
            if ([UIApplication sharedApplication].applicationState != UIApplicationStateBackground) {
                linphone_chat_room_mark_as_read(room);
                [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
            }
            [tableController addChatEntry:chat];
            [tableController scrollToLastUnread:TRUE];
        }
    }
    ms_free(fromStr);
    ms_free(cr_from_string);
}

- (void)textComposeEvent:(NSNotification*)notif {
    LinphoneChatRoom* room = [[[notif userInfo] objectForKey:@"room"] pointerValue];
    if( room && room == chatRoom ){
        BOOL composing = linphone_chat_room_is_remote_composing(room);
        [self setComposingVisible:composing withDelay:0.3];
    }
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)growingTextViewShouldBeginEditing:(HPGrowingTextView *)growingTextView {
    if(editButton.selected) {
        [tableController setEditing:FALSE animated:TRUE];
        [editButton setOff];
    }
    [listTapGestureRecognizer setEnabled:TRUE];
    return TRUE;
}

- (BOOL)growingTextViewShouldEndEditing:(HPGrowingTextView *)growingTextView {
    [listTapGestureRecognizer setEnabled:FALSE];
    return TRUE;
}

- (void)growingTextChanged:(HPGrowingTextView *)growingTextView text:(NSString *)text {
    if( [text length] > 0 && chatRoom )
        linphone_chat_room_compose(chatRoom);
}

- (void)growingTextView:(HPGrowingTextView *)growingTextView willChangeHeight:(float)height {
    int diff = height - growingTextView.bounds.size.height;

    if(diff != 0) {
        CGRect messageRect = [messageView frame];
        messageRect.origin.y -= diff;
        messageRect.size.height += diff;
        [messageView setFrame:messageRect];

        // Always stay at bottom
        if(scrollOnGrowingEnabled) {
            CGRect tableFrame = [tableController.view frame];
            CGPoint contentPt = [tableController.tableView contentOffset];
            contentPt.y += diff;
            if(contentPt.y + tableFrame.size.height > tableController.tableView.contentSize.height)
                contentPt.y += diff;
            [tableController.tableView setContentOffset:contentPt animated:FALSE];
        }

        CGRect tableRect = [tableController.view frame];
        tableRect.size.height -= diff;
        [tableController.view setFrame:tableRect];

        [messageBackgroundImage setImage:[TUNinePatchCache imageOfSize:[messageBackgroundImage bounds].size
                                                     forNinePatchNamed:@"chat_message_background"]];
        // if we're showing the compose message, update it position
        if ( ![composeLabel isHidden] ) {
            CGRect frame = [composeLabel frame];
            frame.origin.y -= diff;
            [composeLabel setFrame:frame];
        }
    }
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [self.tableController setChatRoom:NULL];
    [[PhoneMainView instance] popCurrentView];
}

- (IBAction)onEditClick:(id)event {
    [tableController setEditing:![tableController isEditing] animated:TRUE];
    [messageField resignFirstResponder];
}

- (IBAction)onSendClick:(id)event {
    if([self sendMessage:[messageField text] withExterlBodyUrl:nil withInternalURL:nil]) {
        scrollOnGrowingEnabled = FALSE;
        [messageField setText:@""];
        scrollOnGrowingEnabled = TRUE;
        [self onMessageChange:nil];
    }
}

- (IBAction)onListTap:(id)sender {
    [messageField resignFirstResponder];
}
- (IBAction)onListSwipe:(id)sender {
    [self onBackClick:sender];
}

- (IBAction)onMessageChange:(id)sender {
    if([[messageField text] length] > 0) {
        [sendButton setEnabled:TRUE];
    } else {
        [sendButton setEnabled:FALSE];
    }
}

- (IBAction)onPictureClick:(id)event {
	[messageField resignFirstResponder];

    void (^block)(UIImagePickerControllerSourceType) = ^(UIImagePickerControllerSourceType type) {
        UICompositeViewDescription *description = [ImagePickerViewController compositeViewDescription];
        ImagePickerViewController *controller;
        if([LinphoneManager runningOnIpad]) {
            controller = DYNAMIC_CAST([[PhoneMainView instance].mainViewController getCachedController:description.content], ImagePickerViewController);
        } else {
            controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:description push:TRUE], ImagePickerViewController);
        }
        if(controller != nil) {
            controller.sourceType = type;

            // Displays a control that allows the user to choose picture or
            // movie capture, if both are available:
            controller.mediaTypes = [NSArray arrayWithObject:(NSString *)kUTTypeImage];

            // Hides the controls for moving & scaling pictures, or for
            // trimming movies. To instead show the controls, use YES.
            controller.allowsEditing = NO;
            controller.imagePickerDelegate = self;

            if([LinphoneManager runningOnIpad]) {
                CGRect rect = [self.messageView convertRect:[pictureButton frame] toView:self.view];
                [controller.popoverController presentPopoverFromRect:rect inView:self.view permittedArrowDirections:UIPopoverArrowDirectionAny animated:FALSE];
            }
        }
    };

    DTActionSheet *sheet = [[[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Select picture source",nil)] autorelease];
    if([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypeCamera]) {
	    [sheet addButtonWithTitle:NSLocalizedString(@"Camera",nil) block:^(){
            block(UIImagePickerControllerSourceTypeCamera);
        }];
	}
	if([UIImagePickerController isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]) {
	    [sheet addButtonWithTitle:NSLocalizedString(@"Photo library",nil) block:^(){
            block(UIImagePickerControllerSourceTypePhotoLibrary);
        }];
	}
    [sheet addCancelButtonWithTitle:NSLocalizedString(@"Cancel",nil) block:nil];

    [sheet showInView:[PhoneMainView instance].view];
}

- (IBAction)onTransferCancelClick:(id)event {
    if(imageSharing) {
        [imageSharing cancel];
    }
}


#pragma mark ChatRoomDelegate

- (BOOL)chatRoomStartImageDownload:(NSURL*)url userInfo:(id)userInfo {
    if(imageSharing == nil) {
        imageSharing = [ImageSharing newImageSharingDownload:url delegate:self userInfo:userInfo];
        [messageView setHidden:TRUE];
        [transferView setHidden:FALSE];
        return TRUE;
    }
    return FALSE;
}

- (BOOL)chatRoomStartImageUpload:(UIImage*)image url:(NSURL*)url{
    if(imageSharing == nil) {
        NSString *urlString = [[LinphoneManager instance] lpConfigStringForKey:@"sharing_server_preference"];
        imageSharing = [ImageSharing newImageSharingUpload:[NSURL URLWithString:urlString] image:image delegate:self userInfo:url];
        [messageView setHidden:TRUE];
        [transferView setHidden:FALSE];
        return TRUE;
    }
    return FALSE;
}

- (void)resendChat:(NSString *)message withExternalUrl:(NSString *)url {
    [self sendMessage:message withExterlBodyUrl:[NSURL URLWithString:url] withInternalURL:nil];
}

#pragma mark ImageSharingDelegate

- (void)imageSharingProgress:(ImageSharing*)aimageSharing progress:(float)progress {
    [imageTransferProgressBar setProgress:progress];
}

- (void)imageSharingAborted:(ImageSharing*)aimageSharing {
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    [imageSharing release];
    imageSharing = nil;
}

- (void)imageSharingError:(ImageSharing*)aimageSharing error:(NSError *)error {
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    NSString *url = [aimageSharing.connection.currentRequest.URL absoluteString];
    if (aimageSharing.upload) {
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot upload file to server [%@] because [%@]", url, [error localizedDescription]];
        UIAlertView* errorAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                             message:NSLocalizedString(@"Cannot transfer file to remote contact", nil)
                                                            delegate:nil
                                                   cancelButtonTitle:NSLocalizedString(@"Ok",nil)
                                                   otherButtonTitles:nil ,nil];
		[errorAlert show];
        [errorAlert release];
	} else {
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot download file from [%@] because [%@]", url, [error localizedDescription]];
        UIAlertView* errorAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                             message:NSLocalizedString(@"Cannot transfer file from remote contact", nil)
                                                            delegate:nil
                                                   cancelButtonTitle:NSLocalizedString(@"Continue", nil)
                                                   otherButtonTitles:nil, nil];
		[errorAlert show];
        [errorAlert release];
	}
    imageSharing = nil;
}

- (void)imageSharingUploadDone:(ImageSharing*)aimageSharing url:(NSURL*)url{
    [self sendMessage:nil withExterlBodyUrl:url withInternalURL:[aimageSharing userInfo] ];

    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    imageSharing = nil;
}

- (void)imageSharingDownloadDone:(ImageSharing*)aimageSharing image:(UIImage *)image {
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];

    __block LinphoneChatMessage *chat = (LinphoneChatMessage *)[(NSValue*)[imageSharing userInfo] pointerValue];
    [[LinphoneManager instance].photoLibrary writeImageToSavedPhotosAlbum:image.CGImage
                                                              orientation:(ALAssetOrientation)[image imageOrientation]
                                                          completionBlock:^(NSURL *assetURL, NSError *error){
                                                              if (error) {
                                                                  [LinphoneLogger log:LinphoneLoggerError format:@"Cannot save image data downloaded [%@]", [error localizedDescription]];

                                                                  UIAlertView* errorAlert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                                                                                       message:NSLocalizedString(@"Cannot write image to photo library", nil)
                                                                                                                      delegate:nil
                                                                                                             cancelButtonTitle:NSLocalizedString(@"Ok",nil)
                                                                                                             otherButtonTitles:nil ,nil];
                                                                  [errorAlert show];
                                                                  [errorAlert release];
                                                                  return;
                                                              }
                                                              [LinphoneLogger log:LinphoneLoggerLog format:@"Image saved to [%@]", [assetURL absoluteString]];
                                                              [LinphoneManager setValueInMessageAppData:[assetURL absoluteString] forKey:@"localimage" inMessage:chat];
                                                              [tableController updateChatEntry:chat];
                                                          }];
    imageSharing = nil;
}


#pragma mark ImagePickerDelegate

- (void)imagePickerDelegateImage:(UIImage*)image info:(NSDictionary *)info {
    // Dismiss popover on iPad
    if([LinphoneManager runningOnIpad]) {
        UICompositeViewDescription *description = [ImagePickerViewController compositeViewDescription];
        ImagePickerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance].mainViewController getCachedController:description.content], ImagePickerViewController);
        if(controller != nil) {
            [controller.popoverController dismissPopoverAnimated:TRUE];
        }
    }

    NSURL *url = [info valueForKey:UIImagePickerControllerReferenceURL];
    [self chooseImageQuality:image url:url];
}


#pragma mark - Keyboard Event Functions

- (void)keyboardWillHide:(NSNotification *)notif {
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView animateWithDuration:duration
                          delay:0
                        options:UIViewAnimationOptionBeginFromCurrentState
                     animations:^{
        CGFloat composeIndicatorCompensation = composingVisible ? composeIndicatorView.frame.size.height : 0.0f;

        // Resize chat view
        {
            CGRect chatFrame = [[self chatView] frame];
            chatFrame.size.height = [[self view] frame].size.height - chatFrame.origin.y;
            [[self chatView] setFrame:chatFrame];
        }

        // Move header view back into place (was hidden before)
        {
            CGRect headerFrame = [headerView frame];
            headerFrame.origin.y = 0;
            [headerView setFrame:headerFrame];
            [headerView setAlpha:1.0];
        }

        // Resize & Move table view
        {
            CGRect tableFrame = [tableController.view frame];
            tableFrame.origin.y = [headerView frame].origin.y + [headerView frame].size.height;
            tableFrame.size.height = [messageView frame].origin.y - tableFrame.origin.y - composeIndicatorCompensation;
            [tableController.view setFrame:tableFrame];

            // Scroll to bottom
            NSInteger lastSection = [tableController.tableView numberOfSections] - 1;
            if(lastSection >= 0) {
                NSInteger lastRow = [tableController.tableView numberOfRowsInSection:lastSection] - 1;
                if(lastRow >=0) {
                    [tableController.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:lastRow inSection:lastSection]
                                                     atScrollPosition:UITableViewScrollPositionBottom
                                                             animated:FALSE];
                }
            }
        }

    } completion:^(BOOL finished) {}];
}

- (void)keyboardWillShow:(NSNotification *)notif {
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    CGFloat composeIndicatorCompensation = composingVisible ? composeIndicatorView.frame.size.height : 0.0f;

    [UIView animateWithDuration:duration
                          delay:0
                        options:UIViewAnimationOptionBeginFromCurrentState
                     animations:^{

        CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];

    if(([[UIDevice currentDevice].systemVersion floatValue] < 8) &&
       UIInterfaceOrientationIsLandscape([UIApplication sharedApplication].statusBarOrientation)) {
        int width = endFrame.size.height;
        endFrame.size.height = endFrame.size.width;
        endFrame.size.width = width;
    }

        // Resize chat view
        {
            CGRect viewFrame = [[self view] frame];
            CGRect rect = [PhoneMainView instance].view.bounds;
            CGPoint pos = {viewFrame.size.width, viewFrame.size.height};
            CGPoint gPos = [self.view convertPoint:pos toView:[UIApplication sharedApplication].keyWindow.rootViewController.view]; // Bypass IOS bug on landscape mode
            float diff = (rect.size.height - gPos.y - endFrame.size.height);
            if(diff > 0) diff = 0;
            CGRect chatFrame = [[self chatView] frame];
            chatFrame.size.height = viewFrame.size.height - chatFrame.origin.y + diff;
            [[self chatView] setFrame:chatFrame];
        }

        // Move header view
        {
            CGRect headerFrame = [headerView frame];
            headerFrame.origin.y = -headerFrame.size.height;
            [headerView setFrame:headerFrame];
            [headerView setAlpha:0.0];
        }

        // Resize & Move table view
        {
            CGRect tableFrame = [tableController.view frame];
            tableFrame.origin.y = [headerView frame].origin.y + [headerView frame].size.height;
            tableFrame.size.height = [messageView frame].origin.y - tableFrame.origin.y - composeIndicatorCompensation;
            [tableController.view setFrame:tableFrame];
        }

        // Scroll
        NSInteger lastSection = [tableController.tableView numberOfSections] - 1;
        if(lastSection >= 0) {
            NSInteger lastRow = [tableController.tableView numberOfRowsInSection:lastSection] - 1;
            if(lastRow >=0) {
                [tableController.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:lastRow inSection:lastSection]
                                                 atScrollPosition:UITableViewScrollPositionBottom
                                                         animated:FALSE];
            }
        }

    } completion:^(BOOL finished) {}];
}

@end
