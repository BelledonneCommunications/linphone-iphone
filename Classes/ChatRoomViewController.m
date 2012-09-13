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
#import <MobileCoreServices/UTCoreTypes.h>
#import <NinePatch.h>
#import <AssetsLibrary/ALAssetsLibrary.h>
#import "ImageViewerViewController.h"

#define FILE_DOWNLOAD_ACTION_SHEET 1
#define FILE_CHOOSER_ACTION_SHEET 2

@implementation ChatRoomViewController

@synthesize tableController;
@synthesize sendButton;
@synthesize messageField;
@synthesize editButton;
@synthesize remoteAddress = _remoteAddress;
@synthesize addressLabel;
@synthesize avatarImage;
@synthesize headerView;
@synthesize footerView;
@synthesize chatView;
@synthesize fieldBackgroundImage;
@synthesize pictButton;
@synthesize imageTransferProgressBar;
@synthesize cancelTransfertButton;

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
    [_remoteAddress release];
    [addressLabel release];
    [avatarImage release];
    [headerView release];
    [footerView release];
    [fieldBackgroundImage release];
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
                                                                 tabBar:@"UIMainBar" 
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
	[self enableTransfertView:FALSE];
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
    
    [fieldBackgroundImage setImage:[TUNinePatchCache imageOfSize:[fieldBackgroundImage bounds].size
                                               forNinePatchNamed:@"chat_field"]];
	BOOL fileSharingEnabled = [[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url_preference"] != NULL 
								&& [[[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url_preference"] length]>0 ;
	[pictButton setHidden:!fileSharingEnabled];
	
/*	CGRect frame = messageField.frame;
	if (fileSharingEnabled) {
		frame.origin.x=61;
		frame.size.width=175;
	} else {
		frame.origin.x=0;
		frame.size.width=175+61;
	}
	[messageField setFrame:frame];*/
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
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
    if(_remoteAddress != nil) {
        [_remoteAddress release];
    }
    _remoteAddress = [aRemoteAddress copy];
    [messageField setText:@""];
    [self update];
	[tableController setRemoteAddress: _remoteAddress];
}

- (void)update {
    if(_remoteAddress == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room header: null contact"];
        return;
    }
    
    NSString *displayName = nil;
    UIImage *image = nil;
	LinphoneAddress* linphoneAddress =linphone_core_interpret_url([LinphoneManager getLc],[_remoteAddress cStringUsingEncoding: NSUTF8StringEncoding]);
	if (linphoneAddress==NULL)
		return ;
	char *tmp=linphone_address_as_string_uri_only(linphoneAddress);
	NSString *normalizedSipAddress=[NSString stringWithUTF8String:tmp];
	ms_free(tmp);
	
    ABRecordRef acontact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
    if(acontact != nil) {
        displayName = [FastAddressBook getContactDisplayName:acontact];
        image = [FastAddressBook getContactImage:acontact thumbnail:true];
    }
	[_remoteAddress release];
    _remoteAddress =[normalizedSipAddress retain];
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
	ChatRoomViewController* thiz=(ChatRoomViewController*)ud;
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
    if(_remoteAddress == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot send message: Null remoteAddress"];
        return FALSE;
    }
    if(chatRoom == NULL) {
		chatRoom = linphone_core_create_chat_room([LinphoneManager getLc], [_remoteAddress UTF8String]);
    }
    
    // Save message in database
    ChatModel *chat = [[ChatModel alloc] init];
    [chat setRemoteContact:_remoteAddress];
    [chat setLocalContact:@""];
    [chat setMessage:message];
    [chat setDirection:[NSNumber numberWithInt:0]];
    [chat setTime:[NSDate date]];
    [chat setRead:[NSNumber numberWithInt:1]];
	[chat setState:[NSNumber numberWithInt:1]]; //INPROGRESS
    [chat create];
    [tableController addChatEntry:chat];
   // [chat release]; commenting this line avoid a crash on first message sent, specially when picture
    LinphoneChatMessage* msg = linphone_chat_room_create_message(chatRoom,[message UTF8String]);
	linphone_chat_message_set_user_data(msg,chat);
    if (url) {
		linphone_chat_message_set_external_body_url(msg, [url UTF8String]);
	} 
	linphone_chat_room_send_message2(chatRoom, msg,message_status,self);
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
                caseInsensitiveCompare:_remoteAddress] == NSOrderedSame) {
                [chat setRead:[NSNumber numberWithInt:1]];
                [chat update];
                [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
                [tableController addChatEntry:chat];
            }
            ms_free(fromStr);
        }
		if ([[notif userInfo] objectForKey:@"external_body_url"]) {
			pendingFileUrl=[[[notif userInfo] objectForKey:@"external_body_url"] retain];
			UIActionSheet* new_incoming_file = [[UIActionSheet alloc] initWithTitle:NSLocalizedString(@"Incoming file stored to your photo library",nil)
																		   delegate:self 
																  cancelButtonTitle:NSLocalizedString(@"Ignore",nil) 
															 destructiveButtonTitle:nil
																  otherButtonTitles:NSLocalizedString(@"Accept",nil),nil]; 
			[new_incoming_file setTag:FILE_DOWNLOAD_ACTION_SHEET];
			[new_incoming_file showInView:self.view];
			[new_incoming_file release];
		}
	} else {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Invalid textReceivedEvent"];
    }
}


#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return YES;
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popCurrentView];
}

- (IBAction)onEditClick:(id)event {
    [tableController setEditing:![tableController isEditing] animated:TRUE];
}

- (IBAction)onSendClick:(id)event {
    if([self sendMessage:[messageField text] withExterlBodyUrl:nil]) {
        [messageField setText:@""];
    }
}

- (IBAction)onMessageChange:(id)sender {
    if([[messageField text] length] > 0) {
        [sendButton setEnabled:TRUE];
    } else {
        [sendButton setEnabled:FALSE];
    }
}

- (IBAction)onPictClick:(id)event {
	
    photoSourceSelector = [[UIActionSheet alloc] initWithTitle:NSLocalizedString(@"Select picture source",nil)
													 delegate:self 
											cancelButtonTitle:NSLocalizedString(@"Cancel",nil) 
									   destructiveButtonTitle:nil
											otherButtonTitles:NSLocalizedString(@"Camera",nil),NSLocalizedString(@"Photo library",nil), nil];
    
    photoSourceSelector.actionSheetStyle = UIActionSheetStyleDefault;
	[photoSourceSelector setTag:FILE_CHOOSER_ACTION_SHEET];
    [photoSourceSelector showInView:self.view];
    [photoSourceSelector release];

	
}
- (IBAction)onTransferCancelClick:(id)event {
	[uploadCnx cancel];
	[downloadCnx cancel];
	[self stopUpload];
	[self stopDownload];
	[LinphoneLogger log:LinphoneLoggerLog format:@"File transfert interrupted by user "];
}

-(void) enableTransfertView:(BOOL) isTranfer {
	if (isTranfer) {
		[imageTransferProgressBar setProgress:0.0];
	} else {
		//[uploadCnx cancel];
		
	}
	[imageTransferProgressBar setHidden:!isTranfer];
	[cancelTransfertButton setHidden:!isTranfer];
	[pictButton setEnabled:!isTranfer];
	[sendButton setEnabled:!isTranfer];
}

-(void) startUpload {
	[self enableTransfertView:TRUE];
}
-(void) stopUpload {
	[self enableTransfertView:FALSE];
}
-(void) startDownload {
	[self enableTransfertView:TRUE];
}
-(void) stopDownload {
	[self enableTransfertView:FALSE];
}

-(void) actionSheet:(UIActionSheet *)actionSheet clickedButtonAtIndex:(NSInteger)buttonIndex {
	
	switch (actionSheet.tag) {
		case FILE_CHOOSER_ACTION_SHEET: {
			UIImagePickerController *mediaUI = [[UIImagePickerController alloc] init];
			switch (buttonIndex) {
				case 0: {
					if ([UIImagePickerController isSourceTypeAvailable:
						 UIImagePickerControllerSourceTypeCamera] == NO) {
						[LinphoneLogger log:LinphoneLoggerLog format:@"no camera found, using image library"];
					} else {
						mediaUI.sourceType = UIImagePickerControllerSourceTypeCamera;
						
						// Displays a control that allows the user to choose picture or
						// movie capture, if both are available:
						mediaUI.mediaTypes =
						[UIImagePickerController availableMediaTypesForSourceType:
						 UIImagePickerControllerSourceTypeCamera];
						
						// Hides the controls for moving & scaling pictures, or for
						// trimming movies. To instead show the controls, use YES.
						mediaUI.allowsEditing = NO;
						break;
					}
				}
				case 1: {
					
					mediaUI.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
					
					// Displays saved pictures and movies, if both are available, from the
					// Camera Roll album.
					mediaUI.mediaTypes =
					[UIImagePickerController availableMediaTypesForSourceType:
					 UIImagePickerControllerSourceTypePhotoLibrary];
					
					// Hides the controls for moving & scaling pictures, or for
					// trimming movies. To instead show the controls, use YES.
					mediaUI.allowsEditing = NO;
					
					break;
				}
				default: 
					[mediaUI release];
					return ;break;
					
			}
			mediaUI.delegate = self;
			[self presentModalViewController: mediaUI animated: YES];
			break;
		}
		case FILE_DOWNLOAD_ACTION_SHEET: {
			switch (buttonIndex) {
				case 0: 
					[downloadCnx release];
					downloadCnx= [self downloadImageFrom:pendingFileUrl];
					[self startDownload];
					break;
				case 1: 
				default: {
					//nop
				}
			break;
		}
			break;
		}
			default: 
			[LinphoneLogger log:LinphoneLoggerError format:@"Unexpected action sheet result for tag [%i]",actionSheet.tag];
			
	}
	
}
#pragma mark - NSURLConnectionDelegate
- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error {
	UIAlertView* errorAlert = [UIAlertView alloc];
	if (connection == uploadCnx) {
		[self stopUpload];
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot upload file to server [%@] because [%@]",[[LinphoneManager instance] lpConfigStringForKey:@"file_upload_url"],[error localizedDescription]];
		[errorAlert	initWithTitle:NSLocalizedString(@"Tranfer error",nil)
						  message:NSLocalizedString(@"Cannot transfert file to remote pary",nil) 
						 delegate:nil 
				cancelButtonTitle:NSLocalizedString(@"Ok",nil) 
				otherButtonTitles:nil ,nil];
		[errorAlert show];
	}else if (connection == downloadCnx) {
		[LinphoneLogger log:LinphoneLoggerError format:@"Cannot dowanlod file from [%@] because [%@]",pendingFileUrl,[error localizedDescription]];
		[errorAlert	initWithTitle:NSLocalizedString(@"Tranfer error",nil)
						  message:NSLocalizedString(@"Cannot transfert file from remote pary",nil) 
						 delegate:nil 
				cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
				otherButtonTitles:nil ,nil];		
		[errorAlert show];
	} else {
		[LinphoneLogger log:LinphoneLoggerError format:@"Unknown connection error [%@]",[error localizedDescription]];
	}
	[errorAlert release];
	
}

-(void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
	[imageTransferProgressBar setProgress:(float)((float)totalBytesWritten/(float)totalBytesExpectedToWrite) animated:FALSE];
	
}
- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data {
	if (connection == uploadCnx) {
		NSString* imageRemoteUrl=[[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
		[LinphoneLogger log:LinphoneLoggerLog format:@"File can be downloaded from [%@]",imageRemoteUrl];
		[self sendMessage:NSLocalizedString(@"Image sent",nil) withExterlBodyUrl:imageRemoteUrl];
	} else if (connection == downloadCnx) {
		if (downloadedData == nil) downloadedData = [[NSMutableData alloc] initWithCapacity:4096];
		[downloadedData appendData:data];
		[imageTransferProgressBar setProgress:(float)((float)downloadedData.length/(float)totalBytesExpectedToRead) animated:FALSE];
	} else {
		[LinphoneLogger log:LinphoneLoggerError format:@"Unknown received value error"];
	}
}
- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
	NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *) response;
	int statusCode = httpResponse.statusCode;;	
	[LinphoneLogger log:LinphoneLoggerLog format:@"File transfert status code [%i]",statusCode];
	UIAlertView* errorAlert = [UIAlertView alloc];
	if (connection == uploadCnx) {
		if (statusCode == 200) {
			//nop
		} else if (statusCode >= 400) {
			
			[errorAlert 	initWithTitle:NSLocalizedString(@"Transfer error",nil)
													message:NSLocalizedString(@"Cannot transfert file to remote pary",nil) 
												   delegate:nil 
										  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
										  otherButtonTitles:nil ,nil];
			[errorAlert show];
			
		}
		
	} else if (connection == downloadCnx) {
		if (statusCode == 200) {
			totalBytesExpectedToRead=[response expectedContentLength];
		} else if (statusCode >= 400) {
			[errorAlert	initWithTitle:NSLocalizedString(@"Transfer error",nil)
													message:NSLocalizedString(@"Cannot transfert  file from remote pary",nil) 
												   delegate:nil 
										  cancelButtonTitle:NSLocalizedString(@"Continue",nil) 
										  otherButtonTitles:nil ,nil];	
			[errorAlert show];
		} else {
			//TODO
		}
		
	} else {
		//FIXE
	}
	
	[errorAlert release];
	
	
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
	if (connection == uploadCnx) {
		//nothing to do [self enableTransfert:FALSE];
		[self stopUpload];
		//[uploadCnx release];
		uploadCnx=nil;
	} else if (connection == downloadCnx) {
		ALAssetsLibrary *library = [[ALAssetsLibrary alloc] init];
		[library writeImageDataToSavedPhotosAlbum:downloadedData 
										 metadata:nil
								  completionBlock:^(NSURL *assetURL, NSError *error){
									  if (error) {
										  [LinphoneLogger log:LinphoneLoggerError format:@"Cannot save image data downloaded  because [%@]",[error localizedDescription]];
									  } else {
										  [LinphoneLogger log:LinphoneLoggerLog format:@"Image saved to [%@]",[assetURL absoluteString]];
									  }
									  
									  ImageViewerViewController* imageView = [[ImageViewerViewController alloc ]initWithNibName:@"ImageViewerViewController" bundle:[NSBundle mainBundle]];
									  [imageView setImageToDisplay:[UIImage imageWithData:downloadedData]];
									  [self presentModalViewController: imageView animated: YES];
									  [downloadedData release];
									  downloadedData=nil;
								  }];
		
		
		[library release];
		[self stopDownload];
		//[downloadCnx release];
		downloadCnx=nil;
	}
}
-(NSURLConnection*) downloadImageFrom:(NSString*) address {
	[LinphoneLogger log:LinphoneLoggerLog format:@"downloading  [%@]",address];
	NSURL* url = [NSURL URLWithString: address ];
	NSURLRequest* request = [NSURLRequest requestWithURL:url
											 cachePolicy:NSURLRequestUseProtocolCachePolicy
										 timeoutInterval:60.0];
	
	return [[NSURLConnection alloc] initWithRequest:request delegate: self];
}


-(NSURLConnection*) uploadImage:(UIImage*) image Named:(NSString*) name {
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
	NSString *boundary = [NSString stringWithString:@"---------------------------14737809831466499882746641449"];
	NSString *contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@",boundary];
	[request addValue:contentType forHTTPHeaderField: @"Content-Type"];
	
	/*
	 now lets create the body of the post
	 */
	NSMutableData *body = [NSMutableData data];
	[body appendData:[[NSString stringWithFormat:@"\r\n--%@\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];	
	[body appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"userfile\"; filename=\"%@\"\r\n",name] dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[[NSString stringWithString:@"Content-Type: application/octet-stream\r\n\r\n"] dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[NSData dataWithData:imageData]];
	[body appendData:[[NSString stringWithFormat:@"\r\n--%@--\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
	// setting the body of the post to the reqeust
	[request setHTTPBody:body];
	
	return [NSURLConnection connectionWithRequest:(NSURLRequest *)request 
										 delegate:self];
}

#pragma mark UIImagePickerControllerDelegate
// For responding to the user tapping Cancel.
- (void) imagePickerControllerDidCancel: (UIImagePickerController *) picker {
    [self dismissModalViewControllerAnimated: YES];
    [picker release];
}

- (void) imagePickerController: (UIImagePickerController *) picker
 didFinishPickingMediaWithInfo: (NSDictionary *) info {
	
    NSURL *imageURL = [info valueForKey: UIImagePickerControllerReferenceURL];
	UIImage* imageToUse = (UIImage *) [info objectForKey: UIImagePickerControllerOriginalImage];
	NSString* imageName;
	if (imageURL) {
		// extract id from asset-url ex: assets-library://asset/asset.JPG?id=1645156-6151-1513&ext=JPG
		NSArray *parameters = [[imageURL query] componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"=&"]];
		for (int i = 0; i < [parameters count]; i=i+2) {
			if ([(NSString*)[parameters objectAtIndex:i] isEqualToString:@"id"]) {
				imageName=[NSString stringWithFormat:@"%@.jpg",(NSString*)[parameters objectAtIndex:i+1]];
			}
		}
	} else {
		// must be "unique"
		imageName=[NSString stringWithFormat:@"%i.jpg",[imageToUse hash]];
	}
	uploadCnx =[self uploadImage:imageToUse Named: imageName];
	[self startUpload];
		
    [picker.presentingViewController dismissModalViewControllerAnimated: YES];
    [picker release];
}
#pragma mark - Keyboard Event Functions

- (void)keyboardWillHide:(NSNotification *)notif {
    //CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];
    
    // Move view
    CGRect frame = [[self chatView/*view*/] frame];
    frame.origin.y = frame.origin.y + endFrame.size.height /*0*/;
    [[self /*view*/chatView] setFrame:frame];
    
    // Resize table view
    CGRect tableFrame = [tableController.view frame];
    tableFrame.origin.y = [headerView frame].origin.y + [headerView frame].size.height;
    tableFrame.size.height = [footerView frame].origin.y - tableFrame.origin.y;
    [tableController.view setFrame:tableFrame];
    
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
    
    // Move view
    {
        CGRect frame = [[self chatView/*view*/] frame];
       // CGRect rect = [PhoneMainView instance].view.bounds;
		
       // CGPoint pos = {frame.size.width, frame.size.height};
       // CGPoint gPos = [self.view convertPoint:pos toView:[UIApplication sharedApplication].keyWindow.rootViewController.view]; // Bypass IOS bug on landscape mode
        frame.origin.y = /*(rect.size.height - gPos.y*/ frame.origin.y - endFrame.size.height;
        if(frame.origin.y > 0) frame.origin.y = 0;
        [[self chatView] setFrame:frame];
    }
    
    // Resize table view
    {
        /*CGPoint pos = {0, 0};
        CGPoint gPos = [[self.view superview] convertPoint:pos toView:self.view];*/
        CGRect tableFrame = [tableController.view frame];
        tableFrame.origin.y += endFrame.size.height - headerView.frame.size.height/*gPos.y*/;
        tableFrame.size.height = tableFrame.size.height - endFrame.size.height+headerView.frame.size.height;
        [tableController.view setFrame:tableFrame];
    }

    // Scroll
    int lastSection = [tableController.tableView numberOfSections] -1;
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
