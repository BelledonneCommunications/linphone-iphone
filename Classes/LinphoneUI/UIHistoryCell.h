/* UIHistoryCell.h
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

#import "UIRoundedImageView.h"
#import "UIIconButton.h"

#include "linphone/linphonecore.h"

@interface UIHistoryCell : UITableViewCell

@property (nonatomic, assign) LinphoneCallLog *callLog;

@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(nonatomic, strong) IBOutlet UILabel *displayNameLabel;
@property(weak, nonatomic) IBOutlet UIImageView *stateImage;
@property(weak, nonatomic) IBOutlet UIIconButton *detailsButton;

- (id)initWithIdentifier:(NSString*)identifier;

- (IBAction)onDetails:(id)event;

@end
