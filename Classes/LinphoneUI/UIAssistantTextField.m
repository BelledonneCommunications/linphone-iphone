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

INIT_WITH_COMMON_CF {
	self.delegate = self;
	self.enabled = self.isEnabled; //force refresh bg color
	return self;
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

- (void)showError:(NSString *)msg when:(DisplayErrorPred)apred {
	_showErrorPredicate = apred;
	[self showError:msg];
	[self checkDisplayError];
}

- (void)checkDisplayError {
	_errorLabel.hidden = !(_canShowError && [self isInvalid]);
	self.layer.borderColor = _errorLabel.hidden ? [[UIColor clearColor] CGColor] : [[UIColor redColor] CGColor];
}

- (BOOL)isVisible {
	UIView* aview = self;
	while (aview) {
		if (aview.isHidden || !aview.isUserInteractionEnabled) return NO;
		aview = aview.superview;
	}
	return YES;
}

- (BOOL)isInvalid {
	return self.isVisible && _showErrorPredicate && _showErrorPredicate(_lastText);
}

- (void)setEnabled:(BOOL)enabled {
	[super setEnabled:enabled];
	self.backgroundColor = [self.backgroundColor colorWithAlphaComponent:enabled?1:0.3];
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
	// we must not show any error until user typed at least one character
	_canShowError |= (string.length > 0);
	_lastText = [textField.text stringByReplacingCharactersInRange:range withString:string];
	[self checkDisplayError];
	return YES;
}

- (void)textFieldDidBeginEditing:(UITextField *)textField {
	if (self.nextFieldResponder && !self.nextFieldResponder.hidden) {
		self.returnKeyType = UIReturnKeyNext;
	} else {
		self.returnKeyType = UIReturnKeyDone;
	}

}
- (void)textFieldDidEndEditing:(UITextField *)textField {
	_lastText = textField.text;
	[self checkDisplayError];
}

@end
