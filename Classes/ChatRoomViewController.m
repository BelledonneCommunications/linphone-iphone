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
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

+ (UICompositeViewDescription*) compositeViewDescription {
    UICompositeViewDescription *description = [UICompositeViewDescription alloc];
    description->content = @"ChatRoomViewController";
    description->tabBar = nil;
    description->tabBarEnabled = false;
    description->stateBar = nil;
    description->stateBarEnabled = false;
    description->fullscreen = false;
    return description;
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
                                                 name:@"LinphoneTextReceived" 
                                               object:nil];
    if([tableController isEditing])
        [tableController setEditing:FALSE animated:FALSE];
    [editButton setOff];
    [[tableController tableView] reloadData];
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
                                                    name:@"LinphoneTextReceived" 
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
    [tableController setRemoteAddress: remoteAddress];
    
    if(remoteAddress == NULL) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room header: null contact"];
        return;
    }
    
    NSString *displayName = nil;
    UIImage *image = nil;
    NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:remoteAddress];
    ABRecordRef acontact =[[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
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
        chatRoom = linphone_core_create_chat_room([LinphoneManager getLc], [remoteAddress UTF8String]);
    }
    
    // Save message in database
    ChatModel *chat = [[ChatModel alloc] init];
    [chat setRemoteContact:remoteAddress];
    [chat setMessage:message];
    [chat setDirection:[NSNumber numberWithInt:0]];
    [chat setTime:[NSDate date]];
    [chat setRead:[NSNumber numberWithInt:1]];
    [chat create];
    [tableController addChatEntry:chat];
    [chat release];
    
    linphone_chat_room_send_message(chatRoom, [message UTF8String]);
    return TRUE;
}


#pragma mark - Event Functions

- (void)textReceivedEvent:(NSNotification *)notif {
    //LinphoneChatRoom *room = [[[notif userInfo] objectForKey:@"room"] pointerValue];
    //NSString *message = [[notif userInfo] objectForKey:@"message"];
    LinphoneAddress *from = [[[notif userInfo] objectForKey:@"from"] pointerValue];
    ChatModel *chat = [[notif userInfo] objectForKey:@"chat"];
    if(from != NULL && chat != NULL) {
        if([[NSString stringWithUTF8String:linphone_address_as_string_uri_only(from)] 
            caseInsensitiveCompare:remoteAddress] == NSOrderedSame) {
            [chat setRead:[NSNumber numberWithInt:1]];
            [chat update];
            [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneTextReceived" object:self]; 
            [tableController addChatEntry:chat];
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
    [[PhoneMainView instance] popView];
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
    CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];
    CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect frame = [[self view] frame];
    /*
    CGPoint pos = {0, 0};
    CGPoint gPos = [[self view] convertPoint:pos toView:nil];
    frame.size.height = endFrame.origin.y - gPos.y;
    */
    frame.origin.y += endFrame.origin.y - beginFrame.origin.y;
    [[self view] setFrame:frame];
    [UIView commitAnimations];
}

- (void)keyboardWillShow:(NSNotification *)notif {
    CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];
    CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect frame = [[self view] frame];
    /*CGPoint pos = {0, 0};
    CGPoint gPos = [[self view] convertPoint:pos toView:nil];
    frame.size.height = endFrame.origin.y - gPos.y;*/
    frame.origin.y += endFrame.origin.y - beginFrame.origin.y;
    [[self view] setFrame:frame];
    [UIView commitAnimations];
}

@end
