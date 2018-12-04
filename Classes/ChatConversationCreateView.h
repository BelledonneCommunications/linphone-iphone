//
//  ChatConversationCreateViewViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/10/15.
//
//

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
@property (weak, nonatomic) IBOutlet UIIconButton *backButton;
@property (weak, nonatomic) IBOutlet UIIconButton *nextButton;
@property (weak, nonatomic) IBOutlet UIIconButton *allButton;
@property (weak, nonatomic) IBOutlet UIIconButton *linphoneButton;
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
