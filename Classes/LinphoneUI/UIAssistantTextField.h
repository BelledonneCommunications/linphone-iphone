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
