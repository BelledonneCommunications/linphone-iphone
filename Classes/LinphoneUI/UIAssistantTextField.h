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
@property(atomic, readonly) BOOL canShowError;

- (void)showError:(NSString *)msg when:(UIDisplayError)pred;
- (void)showError:(NSString *)msg;
- (BOOL)isInvalid;

@end
