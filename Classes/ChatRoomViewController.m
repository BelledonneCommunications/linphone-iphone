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

@implementation ChatRoomViewController


@synthesize tableController;
@synthesize sendButton;
@synthesize messageField;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ChatRoomViewController" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
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

- (void)viewWillAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(keyboardWillShow:) 
                                                 name:UIKeyboardWillShowNotification 
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(keyboardWillHide:) 
                                                 name:UIKeyboardWillHideNotification 
                                               object:nil];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [[tableController tableView] reloadData];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillShowNotification 
                                                  object:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIKeyboardWillHideNotification 
                                                  object:nil];
}


#pragma mark - 

- (void)setRemoteContact:(NSString*)remoteContact {
    [tableController setData:[ChatModel listMessages:remoteContact]];
}


#pragma mark - Keyboard Event Functions

- (void)keyboardWillHide:(NSNotification *)notif {
    //CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];
    CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect frame = [[self view] frame];
    CGPoint pos = {0, 0};
    CGPoint gPos = [[self view] convertPoint:pos toView:nil];
    frame.size.height = endFrame.origin.y - gPos.y;
    [[self view] setFrame:frame];
    [UIView commitAnimations];
}

- (void)keyboardWillShow:(NSNotification *)notif {
    //CGRect beginFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue];
    UIViewAnimationCurve curve = [[[notif userInfo] objectForKey:UIKeyboardAnimationCurveUserInfoKey] intValue];
    NSTimeInterval duration = [[[notif userInfo] objectForKey:UIKeyboardAnimationDurationUserInfoKey] doubleValue];
    [UIView beginAnimations:@"resize" context:nil];
    [UIView setAnimationDuration:duration];
    [UIView setAnimationCurve:curve];
    [UIView setAnimationBeginsFromCurrentState:TRUE];
    CGRect endFrame = [[[notif userInfo] objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
    CGRect frame = [[self view] frame];
    CGPoint pos = {0, 0};
    CGPoint gPos = [[self view] convertPoint:pos toView:nil];
    frame.size.height = endFrame.origin.y - gPos.y;
    [[self view] setFrame:frame];
    [UIView commitAnimations];
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
    
}

- (IBAction)onSendClick:(id)event {
    [messageField endEditing:TRUE];
}

- (IBAction)onMessageChange:(id)sender {
    if([[messageField text] length] > 0) {
        [sendButton setEnabled:TRUE];
    } else {
        [sendButton setEnabled:FALSE];
    }
}

@end
