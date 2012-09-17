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

#import <MobileCoreServices/UTCoreTypes.h>
#import <NinePatch.h>
#import <AssetsLibrary/ALAssetsLibrary.h>


@implementation ChatRoomViewController

@synthesize tableController;
@synthesize sendButton;
@synthesize messageField;
@synthesize editButton;
@synthesize remoteAddress;
@synthesize addressLabel;
@synthesize avatarImage;
@synthesize headerView;
@synthesize footerView;
@synthesize chatView;
@synthesize messageView;
@synthesize messageBackgroundImage;
@synthesize footerBackgroundImage;
@synthesize listTapGestureRecognizer;
@synthesize pictureButton;
@synthesize imageTransferProgressBar;
@synthesize cancelTransferButton;
@synthesize transferView;


#pragma mark - Lifecycle Functions

- (id)init {
    self = [super initWithNibName:@"ChatRoomViewController" bundle:[NSBundle mainBundle]];
    if (self != nil) {
        self->chatRoom = NULL;
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
    [footerView release];
    [messageView release];
    [messageBackgroundImage release];
    [footerBackgroundImage release];
    
    [listTapGestureRecognizer release];
    
	[transferView release];
	[pictureButton release];
	[imageTransferProgressBar release];
	[cancelTransferButton release];
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
                                                          landscapeMode:[LinphoneManager runningOnIpad]
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions


- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set selected+over background: IB lack !
    [editButton setImage:[UIImage imageNamed:@"chat_ok_over.png"] 
                forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    messageField.minNumberOfLines = 1;
	messageField.maxNumberOfLines = ([LinphoneManager runningOnIpad])?10:3;
    messageField.delegate = self;
	messageField.font = [UIFont systemFontOfSize:18.0f];
    messageField.contentInset = UIEdgeInsetsZero;
    messageField.backgroundColor = [UIColor clearColor];
	[self enableTransferView:FALSE];
    [sendButton setEnabled:FALSE];
}


- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
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
	if([tableController isEditing])
        [tableController setEditing:FALSE animated:FALSE];
    [editButton setOff];
    [[tableController tableView] reloadData];
    
    [messageBackgroundImage setImage:[TUNinePatchCache imageOfSize:[messageBackgroundImage bounds].size
                                               forNinePatchNamed:@"chat_field"]];
    
    [footerBackgroundImage setImage:[TUNinePatchCache imageOfSize:[footerBackgroundImage bounds].size
                                               forNinePatchNamed:@"chat_background"]];
	BOOL fileSharingEnabled = [[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url_preference"] != NULL 
								&& [[[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url_preference"] length]>0;
    
    CGRect pictureFrame = pictureButton.frame;
	CGRect messageframe = messageView.frame;
    CGRect sendFrame = sendButton.frame;
	if (fileSharingEnabled) {
        [pictureButton setHidden:FALSE];
        messageframe.origin.x = pictureFrame.origin.x + pictureFrame.size.width;
        messageframe.size.width = sendFrame.origin.x - messageframe.origin.x;
	} else {
        [pictureButton setHidden:TRUE];
        messageframe.origin.x = pictureFrame.origin.x;
        messageframe.size.width = sendFrame.origin.x - messageframe.origin.x;
	}
	[messageView setFrame:messageframe];
	
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [messageField resignFirstResponder];
    
    if(chatRoom != NULL) {
        linphone_chat_room_destroy(chatRoom);
        chatRoom = NULL;
    }
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
}

-(void)didReceiveMemoryWarning {
    [TUNinePatchCache flushCache]; // will remove any images cache (freeing any cached but unused images)
}


#pragma mark - 

- (void)setRemoteAddress:(NSString*)aRemoteAddress {
    if(remoteAddress != nil) {
        [remoteAddress release];
    }
    remoteAddress = [aRemoteAddress copy];
    [messageField setText:@""];
    [self update];
	[tableController setRemoteAddress: remoteAddress];
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
}

- (BOOL)sendMessage:(NSString *)message withExterlBodyUrl:(NSString*) url{
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
    [chat setMessage:message];
    [chat setDirection:[NSNumber numberWithInt:0]];
    [chat setTime:[NSDate date]];
    [chat setRead:[NSNumber numberWithInt:1]];
	[chat setState:[NSNumber numberWithInt:1]]; //INPROGRESS
    [chat create];
    [tableController addChatEntry:chat];
    [chat release];
    
    LinphoneChatMessage* msg = linphone_chat_room_create_message(chatRoom, [message UTF8String]);
	linphone_chat_message_set_user_data(msg, chat);
    if (url) {
		linphone_chat_message_set_external_body_url(msg, [url UTF8String]);
	} 
	linphone_chat_room_send_message2(chatRoom, msg, message_status, self);
    return TRUE;
}


#pragma mark - Event Functions

- (void)textReceivedEvent:(NSNotification *)notif {
    //LinphoneChatRoom *room = [[[notif userInfo] objectForKey:@"room"] pointerValue];
    //NSString *message = [[notif userInfo] objectForKey:@"message"];
    LinphoneAddress *from = [[[notif userInfo] objectForKey:@"from"] pointerValue];
    
	ChatModel *chat = [[notif userInfo] objectForKey:@"chat"];
    if(from != NULL && chat != NULL) {
        char *fromStr = linphone_address_as_string_uri_only(from);
        if(fromStr != NULL) {
            if([[NSString stringWithUTF8String:fromStr] 
                caseInsensitiveCompare:remoteAddress] == NSOrderedSame) {
                [chat setRead:[NSNumber numberWithInt:1]];
                [chat update];
                [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
                [tableController addChatEntry:chat];
            }
            ms_free(fromStr);
        }

		if ([[notif userInfo] objectForKey:@"external_body_url"]) {
			NSString *pendingFileUrl = [[[notif userInfo] objectForKey:@"external_body_url"] retain];
            
			DTActionSheet *sheet = [[[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Incoming file stored to your photo library",nil)] autorelease];
            [sheet addButtonWithTitle:NSLocalizedString(@"Accept",nil) block:^(){
                [downloadContext release];
                downloadContext = [self downloadImageFrom:pendingFileUrl];
                [self startDownload];
            }];
            [sheet addCancelButtonWithTitle:NSLocalizedString(@"Ignore",nil)];
            [sheet showInView:self.view];
		}
	} else {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Invalid textReceivedEvent"];
    }
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)growingTextViewShouldBeginEditing:(HPGrowingTextView *)growingTextView {
    if(editButton.selected) {
        [tableController setEditing:FALSE animated:TRUE];
        [editButton setOff];
        [listTapGestureRecognizer setEnabled:TRUE];
    }
    return TRUE;
}

- (void)growingTextView:(HPGrowingTextView *)growingTextView willChangeHeight:(float)height {
    int diff = height - growingTextView.bounds.size.height;
    
    CGRect footerRect = [footerView frame];
    footerRect.origin.y -= diff;
    footerRect.size.height += diff;
    [footerView setFrame:footerRect];
    
    // Always stay at bottom
    CGPoint contentPt = [tableController.tableView contentOffset];
    contentPt.y += diff;
    [tableController.tableView setContentOffset:contentPt animated:FALSE];
    
    CGRect tableRect = [tableController.view frame];
    tableRect.size.height -= diff;
    [tableController.view setFrame:tableRect];
    
    [messageBackgroundImage setImage:[TUNinePatchCache imageOfSize:[messageBackgroundImage bounds].size
                                               forNinePatchNamed:@"chat_field"]];
    
    [footerBackgroundImage setImage:[TUNinePatchCache imageOfSize:[footerBackgroundImage bounds].size
                                                forNinePatchNamed:@"chat_background"]];
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popCurrentView];
}

- (IBAction)onEditClick:(id)event {
    [listTapGestureRecognizer setEnabled:[tableController isEditing]];
    [tableController setEditing:![tableController isEditing] animated:TRUE];
    [messageField resignFirstResponder];
}

- (IBAction)onSendClick:(id)event {
    if([self sendMessage:[messageField text] withExterlBodyUrl:nil]) {
        [messageField setText:@""];
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
    
    [ImagePickerViewController promptSelectSource:^(UIImagePickerControllerSourceType type) {
        UICompositeViewDescription *description = [[[ImagePickerViewController compositeViewDescription] copy] autorelease];
        description.tabBar = nil; // Disable default tarbar
        description.tabBarEnabled = false;
        ImagePickerViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:description push:TRUE], ImagePickerViewController);
        if(controller != nil) {
            controller.sourceType = type;
            
            // Displays a control that allows the user to choose picture or
            // movie capture, if both are available:
            controller.mediaTypes = [UIImagePickerController availableMediaTypesForSourceType:type];
            
            // Hides the controls for moving & scaling pictures, or for
            // trimming movies. To instead show the controls, use YES.
            controller.allowsEditing = NO;
            controller.imagePickerDelegate = self;
        }
    }];
}

- (IBAction)onTransferCancelClick:(id)event {
    if(uploadContext) {
        [uploadContext cancel];
        [self stopUpload];
    }
    if(downloadContext) {
    	[downloadContext cancel];
        [self stopDownload];
    }
	[LinphoneLogger log:LinphoneLoggerLog format:@"File transfer interrupted by user"];
}

- (void)enableTransferView:(BOOL)isTranfer {
	if (isTranfer) {
		[imageTransferProgressBar setProgress:0.0];
	} else {
		//[uploadContext cancel];
	}
    [footerView setHidden:isTranfer];
	[transferView setHidden:!isTranfer];
	[imageTransferProgressBar setHidden:!isTranfer];
	[cancelTransferButton setHidden:!isTranfer];
	[sendButton setEnabled:!isTranfer];
}

- (void)startUpload {
	[self enableTransferView:TRUE];
}

- (void)stopUpload {
	[self enableTransferView:FALSE];
}

- (void)startDownload {
	[self enableTransferView:TRUE];
}

- (void)stopDownload {
	[self enableTransferView:FALSE];
}


#pragma mark - NSURLConnectionDelegate

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
	if (connection == uploadContext) {
		[self stopUpload];
        NSString *serverUrl = [[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url"];
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot upload file to server [%@] because [%@]", serverUrl, [error localizedDescription]];
        	UIAlertView* errorAlert = [UIAlertView alloc];
		[errorAlert	initWithTitle:NSLocalizedString(@"Transfer error", nil)
						  message:NSLocalizedString(@"Cannot transfer file to remote contact", nil) 
						 delegate:nil 
				cancelButtonTitle:NSLocalizedString(@"Ok",nil) 
				otherButtonTitles:nil ,nil];
		[errorAlert show];
        [errorAlert release];
	}else if (connection == downloadContext) {
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot dowanlod file from [%@] because [%@]", [connection.currentRequest.URL absoluteString], [error localizedDescription]];
        	UIAlertView* errorAlert = [UIAlertView alloc];
		[errorAlert	initWithTitle:NSLocalizedString(@"Transfer error", nil)
						  message:NSLocalizedString(@"Cannot transfer file from remote contact", nil)
						 delegate:nil 
				cancelButtonTitle:NSLocalizedString(@"Continue", nil) 
				otherButtonTitles:nil, nil];		
		[errorAlert show];
        [errorAlert release];
	} else {
        [LinphoneLogger log:LinphoneLoggerError format:@"Invalid file transfer connection", connection];
	}
}

- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
	[imageTransferProgressBar setProgress:(float)((float)totalBytesWritten/(float)totalBytesExpectedToWrite) animated:FALSE];
	
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
	if (connection == uploadContext) {
		NSString* imageRemoteUrl=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
		[LinphoneLogger log:LinphoneLoggerLog format:@"File can be downloaded from [%@]", imageRemoteUrl];
		[self sendMessage:NSLocalizedString(@"Image sent",nil) withExterlBodyUrl:imageRemoteUrl];
	} else if (connection == downloadContext) {
		if (downloadedData == nil) downloadedData = [[NSMutableData alloc] initWithCapacity:[data length]];
		[downloadedData appendData:data];
		[imageTransferProgressBar setProgress:((float)downloadedData.length/(float)totalBytesExpectedToRead) animated:FALSE];
	} else {
        [LinphoneLogger log:LinphoneLoggerError format:@"Invalid file transfer connection", connection];
	}
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
	NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *) response;
	int statusCode = httpResponse.statusCode;;	
	[LinphoneLogger log:LinphoneLoggerLog format:@"File transfer status code [%i]",statusCode];
    
	if (connection == uploadContext) {
		if (statusCode == 200) {
			//nop
		} else if (statusCode >= 400) {
            UIAlertView* errorAlert = [UIAlertView alloc];
			[errorAlert initWithTitle:NSLocalizedString(@"Transfer error",nil)
													message:NSLocalizedString(@"Cannot transfer file to remote contact",nil) 
												   delegate:nil 
										  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
										  otherButtonTitles:nil ,nil];
			[errorAlert show];
            [errorAlert release];
		}
	} else if (connection == downloadContext) {
		if (statusCode == 200) {
			totalBytesExpectedToRead = [response expectedContentLength];
		} else if (statusCode >= 400) {
            UIAlertView* errorAlert = [UIAlertView alloc];
			[errorAlert	initWithTitle:NSLocalizedString(@"Transfer error",nil)
													message:NSLocalizedString(@"Cannot transfer file from remote contact",nil)
												   delegate:nil 
										  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
										  otherButtonTitles:nil ,nil];	
			[errorAlert show];
            [errorAlert release];
		} 
	} else {
        [LinphoneLogger log:LinphoneLoggerError format:@"Invalid file transfer connection", connection];
	}
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
	if (connection == uploadContext) {
		[self stopUpload];
		uploadContext = nil;
	} else if (connection == downloadContext) {
		ALAssetsLibrary *library = [[ALAssetsLibrary alloc] init];
		[library writeImageDataToSavedPhotosAlbum:downloadedData
										 metadata:nil
								  completionBlock:^(NSURL *assetURL, NSError *error){
									  if (error) {
										  [LinphoneLogger log:LinphoneLoggerError format:@"Cannot save image data downloaded [%@]",[error localizedDescription]];
									  } else {
										  [LinphoneLogger log:LinphoneLoggerLog format:@"Image saved to [%@]",[assetURL absoluteString]];
									  }
                                      ImageViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ImageViewController compositeViewDescription] push:TRUE], ImageViewController);
                                      if(controller != nil) {
                                          [controller setImage:[UIImage imageWithData:downloadedData]];
                                      }
									  [downloadedData release];
									  downloadedData = nil;
								  }];
		
		
		[library release];
		[self stopDownload];
		downloadContext = nil;
	} else {
        [LinphoneLogger log:LinphoneLoggerError format:@"Invalid file transfer connection", connection];
	}
}

- (NSURLConnection*)downloadImageFrom:(NSString*)address {
	[LinphoneLogger log:LinphoneLoggerLog format:@"downloading [%@]", address];
	NSURL* url = [NSURL URLWithString: address ];
	NSURLRequest* request = [NSURLRequest requestWithURL:url
											 cachePolicy:NSURLRequestUseProtocolCachePolicy
										 timeoutInterval:60.0];
	
	return [[NSURLConnection alloc] initWithRequest:request delegate: self];
}


- (NSURLConnection*)uploadImage:(UIImage*)image Named:(NSString*)name {
	/*
	 turning the image into a NSData object
	 getting the image back out of the UIImageView
	 setting the quality to 90
	 */
	NSData *imageData = UIImageJPEGRepresentation(image, 80);
	// setting up the URL to post to
	NSString *urlString = [[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url_preference"];
	
	// setting up the request object now
	NSMutableURLRequest *request = [[[NSMutableURLRequest alloc] init] autorelease];
	[request setURL:[NSURL URLWithString:urlString]];
	[request setHTTPMethod:@"POST"];
	
	/*
	 add some header info now
	 we always need a boundary when we post a file
	 also we need to set the content type
	 
	 You might want to generate a random boundary.. this is just the same 
	 as my output from wireshark on a valid html post
	 */
	NSString *boundary = @"---------------------------14737809831466499882746641449";
	NSString *contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@",boundary];
	[request addValue:contentType forHTTPHeaderField: @"Content-Type"];
	
	/*
	 now lets create the body of the post
	 */
	NSMutableData *body = [NSMutableData data];
	[body appendData:[[NSString stringWithFormat:@"\r\n--%@\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];	
	[body appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"userfile\"; filename=\"%@\"\r\n",name] dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[@"Content-Type: application/octet-stream\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[NSData dataWithData:imageData]];
	[body appendData:[[NSString stringWithFormat:@"\r\n--%@--\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
	// setting the body of the post to the reqeust
	[request setHTTPBody:body];
	
	return [NSURLConnection connectionWithRequest:(NSURLRequest *)request delegate:self];
}


#pragma mark ImpagePickerDelegate

- (void)imagePickerDelegateImage:(UIImage*)image {
    NSString *imageName = [NSString stringWithFormat:@"%i.jpg", [image hash]];
    uploadContext = [self uploadImage:image Named: imageName];
    [self startUpload];
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
        tableFrame.size.height = [footerView frame].origin.y - tableFrame.origin.y;
        diff = tableFrame.size.height - diff;
        [tableController.view setFrame:tableFrame];
        
        // Always stay at bottom
        CGPoint contentPt = [tableController.tableView contentOffset];
        contentPt.y -= diff;
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
        tableFrame.size.height = [footerView frame].origin.y - tableFrame.origin.y;
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
