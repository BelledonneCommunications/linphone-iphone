//
//  UIAssistantTextField.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/09/15.
//
//

#import "UIAssistantTextField.h"

@implementation UIAssistantTextField

- (void)showError:(NSString *)msg {
	if (!_errorLabel) {
		[self showError:msg when:nil];
	}
	_errorLabel.text = msg;
	_errorLabel.hidden = NO;
	self.layer.borderColor = _errorLabel.hidden ? [[UIColor clearColor] CGColor] : [[UIColor redColor] CGColor];
}

- (void)showError:(NSString *)msg when:(UIDisplayError)apred {
	_showErrorPredicate = apred;

	self.layer.borderWidth = .8;
	self.layer.cornerRadius = 4.f;
	self.autoresizingMask = YES;

	if (!_errorLabel) {
		_errorLabel = [[UILabel alloc] initWithFrame:self.frame];
		_errorLabel.font = [UIFont fontWithName:@"Helvetica" size:11.];
		_errorLabel.textColor = [UIColor redColor];
	}
	_errorLabel.text = msg;

	CGSize maximumLabelSize = CGSizeMake(self.frame.size.width, 9999);
	CGSize expectedLabelSize =
		[msg sizeWithFont:_errorLabel.font constrainedToSize:maximumLabelSize lineBreakMode:_errorLabel.lineBreakMode];

	CGRect newFrame = _errorLabel.frame;
	newFrame.size.height = expectedLabelSize.height;
	newFrame.origin.y = self.frame.origin.y + self.frame.size.height;
	_errorLabel.frame = newFrame;

	_lastText = self.text;

	[self checkDisplayError];
	[self.superview addSubview:_errorLabel];
}

- (void)checkDisplayError {
	_errorLabel.hidden = ![self isInvalid];
	self.layer.borderColor = _errorLabel.hidden ? [[UIColor clearColor] CGColor] : [[UIColor redColor] CGColor];
}

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

- (BOOL)isInvalid {
	return _canShowError && _showErrorPredicate && _showErrorPredicate(_lastText);
}

@end
