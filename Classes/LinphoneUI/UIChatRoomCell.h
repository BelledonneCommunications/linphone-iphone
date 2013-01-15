/* UIChatRoomCell.h
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */  

#import <UIKit/UIKit.h>

#import "ChatModel.h"
#import "ChatRoomTableViewController.h"
#import "UILoadingImageView.h"


@interface UIChatRoomCell : UITableViewCell {
}

@property (nonatomic, retain) ChatModel *chat;
@property (nonatomic, retain) IBOutlet UIView *innerView;
@property (nonatomic, retain) IBOutlet UIView *bubbleView;
@property (nonatomic, retain) IBOutlet UIImageView* backgroundImage;
@property (nonatomic, retain) IBOutlet UITextView *messageText;
@property (nonatomic, retain) IBOutlet UILoadingImageView *messageImageView;
@property (nonatomic, retain) IBOutlet UIButton *deleteButton;
@property (nonatomic, retain) IBOutlet UILabel *dateLabel;
@property (nonatomic, retain) IBOutlet UIImageView* statusImage;
@property (nonatomic, retain) IBOutlet UIButton* downloadButton;
@property (nonatomic, retain) IBOutlet UITapGestureRecognizer* imageTapGestureRecognizer;

- (id)initWithIdentifier:(NSString*)identifier;
+ (CGFloat)height:(ChatModel*)chat width:(int)width;

@property (nonatomic, retain) id<ChatRoomDelegate> chatRoomDelegate;

- (IBAction)onDeleteClick:(id)event;
- (IBAction)onDownloadClick:(id)event;
- (IBAction)onImageClick:(id)event;

@end
