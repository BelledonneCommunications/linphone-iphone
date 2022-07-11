/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface UIChatReplyBubbleView : UIViewController <UICollectionViewDataSource>
@property (weak, nonatomic) IBOutlet UILabel *senderName;
@property (weak, nonatomic) IBOutlet UIButton *dismissButton;
@property (weak, nonatomic) IBOutlet UIView *leftBar;
@property (weak, nonatomic) IBOutlet UIView *rightBar;
@property LinphoneChatMessage *message;
@property (weak, nonatomic) IBOutlet UILabel *textContent;
@property void (^ dismissAction)(void);
@property void (^ clickAction)(void);
@property (weak, nonatomic) IBOutlet UICollectionView *contentCollection;
@property NSArray *dataContent;
@property (weak, nonatomic) IBOutlet UILabel *originalMessageGone;
@property (weak, nonatomic) IBOutlet UIImageView *icsIcon;

-(void) configureForMessage:(LinphoneChatMessage *)message withDimissBlock:(void (^)(void))dismissBlock hideDismiss:(BOOL)hideDismiss withClickBlock:(void (^)(void))clickBlock;
@end

NS_ASSUME_NONNULL_END
