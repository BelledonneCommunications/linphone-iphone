//
//  UIAssistantTextField.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/09/15.
//
//

#import <UIKit/UIKit.h>

typedef BOOL (^DisplayErrorPred)(NSString *inputEntry);

@interface UIAssistantTextField : UITextField <UITextFieldDelegate>

@property(nonatomic, strong) IBOutlet UIView* nextFieldResponder;
@property(nonatomic, strong) IBOutlet UILabel *errorLabel;
@property(nonatomic, readonly) DisplayErrorPred showErrorPredicate;

@property(nonatomic, strong) NSString *lastText;
// we should show error only when user finished editted the field at least once
@property(atomic) BOOL canShowError;

- (void)showError:(NSString *)msg when:(DisplayErrorPred)pred;
- (void)showError:(NSString *)msg;
- (BOOL)isInvalid;
- (BOOL)isVisible;

@end
