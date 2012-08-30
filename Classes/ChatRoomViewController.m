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

#import <NinePatch.h>

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
@synthesize fieldBackgroundImage;


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
    if(remoteAddress != nil) {
        [remoteAddress release];
    }
    remoteAddress = [aRemoteAddress copy];
    [messageField setText:@""];
    [tableController setRemoteAddress: remoteAddress];
    [self update];
}

- (void)update {
    if(remoteAddress == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room header: null contact"];
        return;
    }
    
    NSString *displayName = nil;
    UIImage *image = nil;
    NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:remoteAddress];
    ABRecordRef acontact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
    if(acontact != nil) {
        displayName = [FastAddressBook getContactDisplayName:acontact];
        image = [FastAddressBook getContactImage:acontact thumbnail:true];
    }
    
    // Display name
    if(displayName == nil) {
        displayName = remoteAddress;
    }
    [addressLabel setText:displayName];
    
    // Avatar
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown_small.png"];
    }
    [avatarImage setImage:image];
}
static void message_status(LinphoneChatMessage* msg,LinphoneChatMessageState state,void* ud) {
	ChatRoomViewController* thiz=(ChatRoomViewController*)ud;
	ChatModel *chat = (ChatModel *)linphone_chat_message_get_user_data(msg); 
	[LinphoneLogger log:LinphoneLoggerLog 
				 format:@"Delivery status for [%@] is [%s]",chat.message,linphone_chat_message_state_to_string(state)];
	[chat setState:[NSNumber numberWithInt:state]];
	[chat update];
	[thiz.tableController updateChatEntry:chat];
}

- (BOOL)sendMessage:(NSString *)message {
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot send message: Linphone core not ready"];
        return FALSE;
    }
    if(remoteAddress == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot send message: Null remoteAddress"];
        return FALSE;
    }
    if(chatRoom == NULL) {
		LinphoneProxyConfig* proxyCfg;
		linphone_core_get_default_proxy([LinphoneManager getLc], &proxyCfg);
		if (![remoteAddress hasPrefix:@"sip:"] && proxyCfg) {
			//hmm probably a username only
			char normalizedUserName[256];
			LinphoneAddress* linphoneAddress = linphone_address_new(linphone_core_get_identity([LinphoneManager getLc]));  
			linphone_proxy_config_normalize_number(proxyCfg,[remoteAddress cStringUsingEncoding:[NSString defaultCStringEncoding]],normalizedUserName,sizeof(normalizedUserName));
			linphone_address_set_username(linphoneAddress, normalizedUserName);
			remoteAddress=[NSString stringWithUTF8String:normalizedUserName];
			linphone_address_destroy(linphoneAddress);
			
		}

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
    LinphoneChatMessage* msg = linphone_chat_room_create_message(chatRoom,[message UTF8String]);
	linphone_chat_message_set_user_data(msg,chat);
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
                caseInsensitiveCompare:remoteAddress] == NSOrderedSame) {
                [chat setRead:[NSNumber numberWithInt:1]];
                [chat update];
                [[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneTextReceived object:self];
                [tableController addChatEntry:chat];
            }
            ms_free(fromStr);
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
    if([self sendMessage:[messageField text]]) {
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
