//
//  ChatConversationCreateConfirmView.h
//  linphone
//
//  Created by REIS Benjamin on 04/10/2017.
//

#import <UIKit/UIKit.h>
#import "UICompositeView.h"
#import "ChatConversationCreateConfirmCollectionViewController.h"

@interface ChatConversationCreateConfirmView : UIViewController <UICompositeViewDelegate, UITextFieldDelegate, UIGestureRecognizerDelegate, UICollectionViewDataSource>
@property (weak, nonatomic) IBOutlet UITextField *nameField;
@property (weak, nonatomic) IBOutlet UIIconButton *validateButton;
@property(nonatomic, strong) NSMutableDictionary *contacts;
@property(nonatomic, strong) NSMutableArray *contactsGroup;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property(strong, nonatomic) IBOutlet ChatConversationCreateConfirmCollectionViewController *collectionController;
- (IBAction)onBackClick:(id)sender;
- (IBAction)onValidateClick:(id)sender;
- (void)deleteContact:(NSString *)uri;

@end
