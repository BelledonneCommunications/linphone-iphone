/* UICallCell.h
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

#include "linphonecore.h"
#include "UIPauseButton.h"

@interface UICallCellData : NSObject {
    @public
    bool minimize;
    LinphoneCall *call;
}   

- (id)init:(LinphoneCall*) call;

@end

@interface UICallCell : UITableViewCell {
    @private
    BOOL firstCell;
    BOOL conferenceCell;
    BOOL currentCall;
    
    UIImageView* headerBackgroundImage;
    UIImageView* headerBackgroundHighlightImage;
    
    UILabel *addressLabel;
    UILabel *stateLabel;
    UIImageView *stateImage;
    UIPauseButton *pauseButton;
    UIButton *removeButton;
    UIImageView *avatarImage;
    
    UIView *headerView;
    UIView *avatarView;
    
    UICallCellData *data;
}

@property (nonatomic, retain) UICallCellData *data;

@property (nonatomic, retain) IBOutlet UIImageView* headerBackgroundImage;
@property (nonatomic, retain) IBOutlet UIImageView* headerBackgroundHighlightImage;

@property (nonatomic, retain) IBOutlet UILabel* addressLabel;
@property (nonatomic, retain) IBOutlet UILabel* stateLabel;
@property (nonatomic, retain) IBOutlet UIImageView* stateImage;
@property (nonatomic, retain) IBOutlet UIImageView* avatarImage;
@property (nonatomic, retain) IBOutlet UIButton *removeButton;
@property (nonatomic, retain) IBOutlet UIPauseButton *pauseButton;

@property (nonatomic, retain) IBOutlet UIView* headerView;
@property (nonatomic, retain) IBOutlet UIView* avatarView;

@property (assign) BOOL firstCell;
@property (assign) BOOL conferenceCell;
@property (nonatomic, assign) BOOL currentCall;

- (void)update;

- (id)initWithIdentifier:(NSString*)identifier;

- (IBAction)doHeaderClick:(id)sender;
- (IBAction)doRemoveClick:(id)sender;

+ (int)getMaximizedHeight;
+ (int)getMinimizedHeight;

@end
