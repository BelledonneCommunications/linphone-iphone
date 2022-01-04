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

#ifndef ChatConversationCreateView_h
#define ChatConversationCreateView_h

#import <UIKit/UIKit.h>
#import "ChatConversationCreateTableView.h"
#import "ChatConversationCreateCollectionViewController.h"
#import "UICompositeView.h"

@interface ChatConversationCreateView : UIViewController <UICompositeViewDelegate, UIGestureRecognizerDelegate, UICollectionViewDataSource>

@property(strong, nonatomic) IBOutlet ChatConversationCreateTableView *tableController;
@property(strong, nonatomic) IBOutlet ChatConversationCreateCollectionViewController *collectionController;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (weak, nonatomic) IBOutlet UIButton *backButton;
@property (weak, nonatomic) IBOutlet UIButton *nextButton;
@property (weak, nonatomic) IBOutlet UIButton *allButton;
@property (weak, nonatomic) IBOutlet UIButton *linphoneButton;
@property (weak, nonatomic) IBOutlet UIImageView *selectedButtonImage;
@property (weak, nonatomic) IBOutlet UIView *waitView;
@property (weak, nonatomic) IBOutlet UIView *chiffreOptionView;
@property (weak, nonatomic) IBOutlet UIView *switchView;
@property (weak, nonatomic) IBOutlet UIImageView *chiffreImage;
@property (weak, nonatomic) IBOutlet UIButton *chiffreButton;

@property(nonatomic) Boolean isForEditing;
@property(nonatomic) Boolean isGroupChat;
@property(nonatomic) Boolean isEncrypted;

- (IBAction)onBackClick:(id)sender;
- (IBAction)onNextClick:(id)sender;
- (IBAction)onChiffreClick:(id)sender;

@end

#endif /* ChatConversationCreateView_h */
