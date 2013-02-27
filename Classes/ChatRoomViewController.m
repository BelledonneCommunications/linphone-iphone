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
@synthesize remoteAddress;
@synthesize addressLabel;
@synthesize avatarImage;
@synthesize headerView;
@synthesize chatView;
@synthesize messageView;
@synthesize messageBackgroundImage;
@synthesize transferBackgroundImage;
@synthesize listTapGestureRecognizer;
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
        self->imageQualities = [[OrderedDictionary alloc] initWithObjectsAndKeys:
                                [NSNumber numberWithFloat:0.9], NSLocalizedString(@"Maximum", nil),
                                [NSNumber numberWithFloat:0.5], NSLocalizedString(@"Average", nil),
                                [NSNumber numberWithFloat:0.0], NSLocalizedString(@"Minimum", nil), nil];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [tableController release];
    [messageField release];
    [sendButton release];
    [editButton release];
    [remoteAddress release];
    [addressLabel release];
    [avatarImage release];
    [headerView release];
    [messageView release];
    [messageBackgroundImage release];
    [transferBackgroundImage release];
    
    [listTapGestureRecognizer release];
    
	[transferView release];
	[pictureButton release];
	[imageTransferProgressBar release];
	[cancelTransferButton release];
    
    [imageQualities release];
    [waitView release];
    
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
                                             selector:@selector(coreUpdateEvent:)
                                                 name:kLinphoneCoreUpdate
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
    
    if(chatRoom != NULL) {
        linphone_chat_room_destroy(chatRoom);
        chatRoom = NULL;
    }
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
                                                    name:kLinphoneCoreUpdate
												  object:nil];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
    [messageBackgroundImage setImage:[TUNinePatchCache imageOfSize:[messageBackgroundImage bounds].size
                                                 forNinePatchNamed:@"chat_message_background"]];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration {
    [super willAnimateRotationToInterfaceOrientation:toInterfaceOrientation duration:duration];

}

-(void)didReceiveMemoryWarning {
    [TUNinePatchCache flushCache]; // will remove any images cache (freeing any cached but unused images)
}


#pragma mark - 

- (void)setRemoteAddress:(NSString*)aRemoteAddress {
    if(remoteAddress != nil) {
        [remoteAddress release];
    }
    if ([aRemoteAddress hasPrefix:@"sip:"]) {
        remoteAddress = [aRemoteAddress copy];
    } else {
        char normalizedUserName[256];
        LinphoneCore *lc = [LinphoneManager getLc];
        LinphoneProxyConfig* proxyCfg;
        linphone_core_get_default_proxy(lc,&proxyCfg);
        LinphoneAddress* linphoneAddress = linphone_address_new(linphone_core_get_identity(lc));
        linphone_proxy_config_normalize_number(proxyCfg, [aRemoteAddress cStringUsingEncoding:[NSString defaultCStringEncoding]], normalizedUserName, sizeof(normalizedUserName));
        linphone_address_set_username(linphoneAddress, normalizedUserName);
        remoteAddress = [[NSString stringWithUTF8String:linphone_address_as_string_uri_only(linphoneAddress)] copy];
        linphone_address_destroy(linphoneAddress);
    }
    [messageField setText:@""];
    [self update];
	[tableController setRemoteAddress: remoteAddress];
    [ChatModel readConversation:remoteAddress];
    [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
}

- (void)applicationWillEnterForeground:(NSNotification*)notif {
    if(remoteAddress != nil) {
        [ChatModel readConversation:remoteAddress];
        [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
    }
}

- (void)update {
    if(remoteAddress == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room header: null contact"];
        return;
    }
    
    NSString *displayName = nil;
    UIImage *image = nil;
	LinphoneAddress* linphoneAddress = linphone_core_interpret_url([LinphoneManager getLc], [remoteAddress UTF8String]);
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
	[remoteAddress release];
    remoteAddress = [normalizedSipAddress retain];
    
    // Display name
    if(displayName == nil) {
        displayName = [NSString stringWithUTF8String:linphone_address_get_username(linphoneAddress)];
    }
    [addressLabel setText:displayName];
    
    // Avatar
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown_small.png"];
    }
    [avatarImage setImage:image];
    
    linphone_address_destroy(linphoneAddress);
}

static void message_status(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud) {
	ChatRoomViewController* thiz = (ChatRoomViewController*)ud;
	ChatModel *chat = (ChatModel *)linphone_chat_message_get_user_data(msg); 
	[LinphoneLogger log:LinphoneLoggerLog 
				 format:@"Delivery status for [%@] is [%s]",(chat.message?chat.message:@""),linphone_chat_message_state_to_string(state)];
	[chat setState:[NSNumber numberWithInt:state]];
	[chat update];
	[thiz.tableController updateChatEntry:chat];
	linphone_chat_message_set_user_data(msg, NULL);
	[chat release]; // no longuer need to keep reference
	
}

- (BOOL)sendMessage:(NSString *)message withExterlBodyUrl:(NSURL*)externalUrl withInternalUrl:(NSURL*)internalUrl {
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot send message: Linphone core not ready"];
        return FALSE;
    }
    if(remoteAddress == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot send message: Null remoteAddress"];
        return FALSE;
    }
    if(chatRoom == NULL) {
		chatRoom = linphone_core_create_chat_room([LinphoneManager getLc], [remoteAddress UTF8String]);
    }
    
    // Save message in database
    ChatModel *chat = [[ChatModel alloc] init];
    [chat setRemoteContact:remoteAddress];
    [chat setLocalContact:@""];
    if(internalUrl == nil) {
        [chat setMessage:message];
    } else {
        [chat setMessage:[internalUrl absoluteString]];
    }
    [chat setDirection:[NSNumber numberWithInt:0]];
    [chat setTime:[NSDate date]];
    [chat setRead:[NSNumber numberWithInt:1]];
	[chat setState:[NSNumber numberWithInt:1]]; //INPROGRESS
    [chat create];
    [tableController addChatEntry:chat];
    [tableController scrollToBottom:TRUE];
    [chat release];
    
    LinphoneChatMessage* msg = linphone_chat_room_create_message(chatRoom, [message UTF8String]);
	linphone_chat_message_set_user_data(msg, [chat retain]);
    if(externalUrl) {
        linphone_chat_message_set_external_body_url(msg, [[externalUrl absoluteString] UTF8String]);
    }
	linphone_chat_room_send_message2(chatRoom, msg, message_status, self);
    return TRUE;
}

- (void)saveAndSend:(UIImage*)image url:(NSURL*)url {
    if(url == nil) {
        [waitView setHidden:FALSE];
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
            [[LinphoneManager instance].photoLibrary writeImageToSavedPhotosAlbum:image.CGImage
                                                                      orientation:(ALAssetOrientation)[image imageOrientation]
                                                                  completionBlock:^(NSURL *assetURL, NSError *error){
                                                                      dispatch_async(dispatch_get_main_queue(), ^{
                                                                          [waitView setHidden:TRUE];
                                                                          if (error) {
                                                                              [LinphoneLogger log:LinphoneLoggerError format:@"Cannot save image data downloaded [%@]", [error localizedDescription]];
                                                                              
                                                                              UIAlertView* errorAlert = [UIAlertView alloc];
                                                                              [errorAlert	initWithTitle:NSLocalizedString(@"Transfer error", nil)
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


#pragma mark - Event Functions

- (void)coreUpdateEvent:(NSNotification*)notif {
    if(![LinphoneManager isLcReady]) {
        chatRoom = NULL;
    }
}

- (void)textReceivedEvent:(NSNotification *)notif {
    //LinphoneChatRoom *room = [[[notif userInfo] objectForKey:@"room"] pointerValue];
    //NSString *message = [[notif userInfo] objectForKey:@"message"];
    LinphoneAddress *from = [[[notif userInfo] objectForKey:@"from"] pointerValue];
    
	ChatModel *chat = [[notif userInfo] objectForKey:@"chat"];
    if(from == NULL || chat == NULL) {
        return;
    }
    char *fromStr = linphone_address_as_string_uri_only(from);
    if(fromStr != NULL) {
        if([[NSString stringWithUTF8String:fromStr]
            caseInsensitiveCompare:remoteAddress] == NSOrderedSame) {
            if (![[UIDevice currentDevice] respondsToSelector:@selector(isMultitaskingSupported)]
                || [UIApplication sharedApplication].applicationState == UIApplicationStateActive) {
                [chat setRead:[NSNumber numberWithInt:1]];
                [chat update];
                [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
            }
            [tableController addChatEntry:chat];
            [tableController scrollToLastUnread:TRUE];
        }
        ms_free(fromStr);
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
    }
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popCurrentView];
}

- (IBAction)onEditClick:(id)event {
    [tableController setEditing:![tableController isEditing] animated:TRUE];
    [messageField resignFirstResponder];
}

- (IBAction)onSendClick:(id)event {
    if([self sendMessage:[messageField text] withExterlBodyUrl:nil withInternalUrl:nil]) {
        scrollOnGrowingEnabled = FALSE;
        [messageField setText:@""];
        scrollOnGrowingEnabled = TRUE;
        [self onMessageChange:nil];
    }
}

- (IBAction)onListTap:(id)sender {
    [messageField resignFirstResponder];
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
        imageSharing = [ImageSharing imageSharingDownload:url delegate:self userInfo:userInfo];
        [messageView setHidden:TRUE];
        [transferView setHidden:FALSE];
        return TRUE;
    }
    return FALSE;
}

- (BOOL)chatRoomStartImageUpload:(UIImage*)image url:(NSURL*)url{
    if(imageSharing == nil) {
        NSString *urlString = [[LinphoneManager instance] lpConfigStringForKey:@"sharing_server_preference"];
        imageSharing = [ImageSharing imageSharingUpload:[NSURL URLWithString:urlString] image:image delegate:self userInfo:url];
        [messageView setHidden:TRUE];
        [transferView setHidden:FALSE];
        return TRUE;
    }
    return FALSE;
}


#pragma mark ImageSharingDelegate

- (void)imageSharingProgress:(ImageSharing*)aimageSharing progress:(float)progress {
    [imageTransferProgressBar setProgress:progress];
}

- (void)imageSharingAborted:(ImageSharing*)aimageSharing {
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    imageSharing = NULL;
}

- (void)imageSharingError:(ImageSharing*)aimageSharing error:(NSError *)error {
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    NSString *url = [aimageSharing.connection.currentRequest.URL absoluteString];
    if (aimageSharing.upload) {
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot upload file to server [%@] because [%@]", url, [error localizedDescription]];
        UIAlertView* errorAlert = [UIAlertView alloc];
		[errorAlert	initWithTitle:NSLocalizedString(@"Transfer error", nil)
						  message:NSLocalizedString(@"Cannot transfer file to remote contact", nil)
						 delegate:nil
				cancelButtonTitle:NSLocalizedString(@"Ok",nil)
				otherButtonTitles:nil ,nil];
		[errorAlert show];
        [errorAlert release];
	} else {
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot dowanlod file from [%@] because [%@]", url, [error localizedDescription]];
        UIAlertView* errorAlert = [UIAlertView alloc];
		[errorAlert	initWithTitle:NSLocalizedString(@"Transfer error", nil)
						  message:NSLocalizedString(@"Cannot transfer file from remote contact", nil)
						 delegate:nil
				cancelButtonTitle:NSLocalizedString(@"Continue", nil)
				otherButtonTitles:nil, nil];
		[errorAlert show];
        [errorAlert release];
	}
    imageSharing = NULL;
}

- (void)imageSharingUploadDone:(ImageSharing*)aimageSharing url:(NSURL*)url{
    NSURL *imageURL = [aimageSharing userInfo];
    
    [self sendMessage:nil withExterlBodyUrl:url withInternalUrl:imageURL];
    
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    imageSharing = NULL;
}

- (void)imageSharingDownloadDone:(ImageSharing*)aimageSharing image:(UIImage *)image {
    [messageView setHidden:FALSE];
	[transferView setHidden:TRUE];
    
    ChatModel *chat = (ChatModel *)[imageSharing userInfo];
    [[LinphoneManager instance].photoLibrary writeImageToSavedPhotosAlbum:image.CGImage
                                                              orientation:(ALAssetOrientation)[image imageOrientation]
                                                          completionBlock:^(NSURL *assetURL, NSError *error){
                                                              if (error) {
                                                                  [LinphoneLogger log:LinphoneLoggerError format:@"Cannot save image data downloaded [%@]", [error localizedDescription]];
                                                                  
                                                                  UIAlertView* errorAlert = [UIAlertView alloc];
                                                                  [errorAlert initWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                                                    message:NSLocalizedString(@"Cannot write image to photo library", nil)
                                                                                   delegate:nil
                                                                          cancelButtonTitle:NSLocalizedString(@"Ok",nil)
                                                                          otherButtonTitles:nil ,nil];
                                                                  [errorAlert show];
                                                                  [errorAlert release];
                                                                  return;
                                                              }
                                                              [LinphoneLogger log:LinphoneLoggerLog format:@"Image saved to [%@]", [assetURL absoluteString]];
                                                              [chat setMessage:[assetURL absoluteString]];
                                                              [chat update];
                                                              [tableController updateChatEntry:chat];
                                                          }];
    imageSharing = NULL;
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
    //CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    //CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];
    
    // Resize chat view
    {
        CGRect chatFrame = [[self chatView] frame];
        chatFrame.size.height = [[self view] frame].size.height - chatFrame.origin.y;
        [[self chatView] setFrame:chatFrame];
    }
    
    // Move header view
    {
        CGRect headerFrame = [headerView frame];
        headerFrame.origin.y = 0;
        [headerView setFrame:headerFrame];
    }
    
    // Resize & Move table view
    {
        CGRect tableFrame = [tableController.view frame];
        tableFrame.origin.y = [headerView frame].origin.y + [headerView frame].size.height;
        double diff = tableFrame.size.height;
        tableFrame.size.height = [messageView frame].origin.y - tableFrame.origin.y;
        diff = tableFrame.size.height - diff;
        [tableController.view setFrame:tableFrame];
        
        // Always stay at bottom
        CGPoint contentPt = [tableController.tableView contentOffset];
        contentPt.y -= diff;
        if(contentPt.y + tableFrame.size.height > tableController.tableView.contentSize.height)
             contentPt.y += diff;
        [tableController.tableView setContentOffset:contentPt animated:FALSE];
    }
    
    [UIView commitAnimations];
}

- (void)keyboardWillShow:(NSNotification *)notif {
    //CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];

    if(UIInterfaceOrientationIsLandscape([UIApplication sharedApplication].statusBarOrientation)) {
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
    }
    
    // Resize & Move table view
    {
        CGRect tableFrame = [tableController.view frame];
        tableFrame.origin.y = [headerView frame].origin.y + [headerView frame].size.height;
        tableFrame.size.height = [messageView frame].origin.y - tableFrame.origin.y;
        [tableController.view setFrame:tableFrame];
    }
    
    // Scroll
    int lastSection = [tableController.tableView numberOfSections] - 1;
    if(lastSection >= 0) {
        int lastRow = [tableController.tableView numberOfRowsInSection:lastSection] - 1;
        if(lastRow >=0) {
            [tableController.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:lastRow inSection:lastSection] 
                                             atScrollPosition:UITableViewScrollPositionBottom 
                                                     animated:TRUE];
        }
    }
    [UIView commitAnimations];
}

@end
