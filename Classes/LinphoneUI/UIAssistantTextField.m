/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
