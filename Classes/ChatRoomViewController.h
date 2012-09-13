/* ChatRoomViewController.h
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

#import <UIKit/UIKit.h>

#import "UIToggleButton.h"
#import "UICompositeViewController.h"
#import "ChatRoomTableViewController.h"
#import "ChatModel.h"

#include "linphonecore.h"

@interface ChatRoomViewController : UIViewController<UITextFieldDelegate, UICompositeViewDelegate,UIActionSheetDelegate,UIImagePickerControllerDelegate,UINavigationControllerDelegate,NSURLConnectionDataDelegate> {
    @private
    LinphoneChatRoom *chatRoom;
	NSString *_remoteAddress;
	UIActionSheet* photoSourceSelector;
	NSURLConnection* uploadCnx;
	NSURLConnection* downloadCnx;
	NSString* pendingFileUrl; /*Url received from the remote party to be downloaded*/
	NSMutableData* downloadedData;
	NSInteger totalBytesExpectedToRead;
}


@property (nonatomic, retain) IBOutlet ChatRoomTableViewController* tableController;
@property (nonatomic, retain) IBOutlet UIToggleButton *editButton;
@property (nonatomic, retain) IBOutlet UITextView* messageField;
@property (nonatomic, retain) IBOutlet UIButton* sendButton;
@property (nonatomic, retain) IBOutlet UILabel *addressLabel;
@property (nonatomic, retain) IBOutlet UIImageView *avatarImage;
@property (nonatomic, retain) IBOutlet UIView *headerView;
@property (nonatomic, retain) IBOutlet UIView *footerView;
@property (nonatomic, retain) IBOutlet UIView *chatView;
@property (nonatomic, retain) IBOutlet UIImageView *fieldBackgroundImage;
@property (nonatomic, copy) NSString *remoteAddress;
@property (nonatomic, retain) IBOutlet UIButton* pictButton;
@property (nonatomic, retain) IBOutlet UIButton* cancelTransfertButton;
@property (nonatomic, retain) IBOutlet UIProgressView* imageTransferProgressBar;

- (IBAction)onBackClick:(id)event;
- (IBAction)onEditClick:(id)event;
- (IBAction)onMessageChange:(id)sender;
- (IBAction)onSendClick:(id)event;
- (IBAction)onPictClick:(id)event;
- (IBAction)onTransferCancelClick:(id)event;

@end
