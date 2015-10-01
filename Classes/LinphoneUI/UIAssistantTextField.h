//
//  UIAssistantTextField.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/09/15.
//
//

#import <UIKit/UIKit.h>

typedef BOOL (^UIDisplayError)(NSString *inputEntry);

@interface UIAssistantTextField : UITextField <UITextFieldDelegate>

@property(nonatomic) UILabel *errorLabel;
@property(nonatomic, readonly) UIDisplayError showErrorPredicate;
@property(nonatomic, readonly) NSString *lastText;
// we should show error only when user finished editted the field at least once
@property(atomic) BOOL canShowError;

- (void)showError:(NSString *)msg when:(UIDisplayError)pred;
- (void)showError:(NSString *)msg;
- (BOOL)isInvalid;

@end
