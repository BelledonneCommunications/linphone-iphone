//
//  ChatConversationCreateViewViewController.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/10/15.
//
//

#import <UIKit/UIKit.h>
#import "ChatConversationCreateTableView.h"
#import "ChatConversationCreateCollectionViewController.h"
#import "UICompositeView.h"

@interface ChatConversationCreateView : UIViewController <UICompositeViewDelegate, UIGestureRecognizerDelegate, UICollectionViewDataSource>

@property(strong, nonatomic) IBOutlet ChatConversationCreateTableView *tableController;
@property(strong, nonatomic) IBOutlet ChatConversationCreateCollectionViewController *collectionController;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;

@property(weak, nonatomic) IBOutlet UIIconButton *backButton;
@property (weak, nonatomic) IBOutlet UIIconButton *nextButton;

@property (weak, nonatomic) IBOutlet UIIconButton *allButton;
@property (weak, nonatomic) IBOutlet UIIconButton *linphoneButton;
@property (weak, nonatomic) IBOutlet UIImageView *selectedButtonImage;

- (IBAction)onBackClick:(id)sender;
- (IBAction)onNextClick:(id)sender;

@end
