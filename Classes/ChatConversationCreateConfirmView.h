//
//  ChatConversationCreateConfirmView.h
//  linphone
//
//  Created by REIS Benjamin on 04/10/2017.
//

#import <UIKit/UIKit.h>
#import "UICompositeView.h"

@interface ChatConversationCreateConfirmView : UIViewController <UICompositeViewDelegate, UITextFieldDelegate, UIGestureRecognizerDelegate, UICollectionViewDataSource>
@property (weak, nonatomic) IBOutlet UITextField *nameField;
@property (weak, nonatomic) IBOutlet UIIconButton *validateButton;
@property(nonatomic, strong) NSMutableDictionary *contacts;
- (IBAction)onBackClick:(id)sender;
- (IBAction)onValidateClick:(id)sender;

@end
