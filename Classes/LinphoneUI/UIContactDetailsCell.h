/* UIEditableTableViewCell.h
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

#import "UIIconButton.h"

@interface UIContactDetailsCell : UITableViewCell

// this is broken design... but we need this to know which cell was modified
// last... must be totally revamped
@property(strong) NSIndexPath *indexPath;

@property(weak, nonatomic) IBOutlet UIView *defaultView;
@property(weak, nonatomic) IBOutlet UILabel *addressLabel;
@property(weak, nonatomic) IBOutlet UITextField *editTextfield;
@property(weak, nonatomic) IBOutlet UIView *editView;
@property(weak, nonatomic) IBOutlet UIIconButton *deleteButton;
@property(weak, nonatomic) IBOutlet UIIconButton *callButton;
@property(weak, nonatomic) IBOutlet UIIconButton *chatButton;
@property (weak, nonatomic) IBOutlet UIImageView *linphoneImage;

- (id)initWithIdentifier:(NSString *)identifier;
- (void)setAddress:(NSString *)address;
- (void)hideDeleteButton:(BOOL)hidden;
- (void)shouldHideLinphoneImageOfAddress;

- (IBAction)onCallClick:(id)sender;
- (IBAction)onChatClick:(id)sender;
- (IBAction)onDeleteClick:(id)sender;
@end
