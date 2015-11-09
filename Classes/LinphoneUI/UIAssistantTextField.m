//
//  UIAssistantTextField.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/09/15.
//
//

#import "UIAssistantTextField.h"
#import "Utils.h"

@implementation UIAssistantTextField

INIT_WITH_COMMON {
	self.delegate = self;
}

- (void)showError:(NSString *)msg {
	_errorLabel.text = msg;
	_lastText = self.text;

	_errorLabel.hidden = NO;
	self.layer.borderWidth = .8;
	self.layer.cornerRadius = 4.f;
	self.autoresizingMask = YES;
	self.layer.borderColor = _errorLabel.hidden ? [[UIColor clearColor] CGColor] : [[UIColor redColor] CGColor];
}

- (void)showError:(NSString *)msg when:(UIDisplayError)apred {
	_showErrorPredicate = apred;
	[self showError:msg];
	[self checkDisplayError];
}

- (void)checkDisplayError {
	_errorLabel.hidden = !(_canShowError && [self isInvalid]);
	self.layer.borderColor = _errorLabel.hidden ? [[UIColor clearColor] CGColor] : [[UIColor redColor] CGColor];
}

- (BOOL)isInvalid {
	return _showErrorPredicate && _showErrorPredicate(_lastText);
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	_lastText = [textField.text stringByReplacingCharactersInRange:range withString:string];
	[self checkDisplayError];
	return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	_canShowError = YES;
	[self checkDisplayError];
}

@end
