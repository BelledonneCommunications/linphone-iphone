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
@property (weak, nonatomic) IBOutlet UIIconButton *nextButton;
@property (weak, nonatomic) IBOutlet UIIconButton *allButton;
@property (weak, nonatomic) IBOutlet UIIconButton *linphoneButton;
@property (weak, nonatomic) IBOutlet UIImageView *selectedButtonImage;
@property (weak, nonatomic) IBOutlet UIView *waitView;

@property(nonatomic) Boolean isForEditing;

- (IBAction)onBackClick:(id)sender;
- (IBAction)onNextClick:(id)sender;

@end

#endif /* ChatConversationCreateView_h */
