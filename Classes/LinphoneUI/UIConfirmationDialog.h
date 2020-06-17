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

#include "UIRoundBorderedButton.h"

typedef void (^UIConfirmationBlock)(void);

@interface UIConfirmationDialog : UIViewController {
	UIConfirmationBlock onCancelCb;
	UIConfirmationBlock onConfirmCb;
}

+ (UIConfirmationDialog *)ShowWithMessage:(NSString *)message
							cancelMessage:(NSString *)cancel
						   confirmMessage:(NSString *)confirm
							onCancelClick:(UIConfirmationBlock)onCancel
					  onConfirmationClick:(UIConfirmationBlock)onConfirm;
+ (UIConfirmationDialog *)ShowWithMessage:(NSString *)message
							cancelMessage:(NSString *)cancel
						   confirmMessage:(NSString *)confirm
							onCancelClick:(UIConfirmationBlock)onCancel
					  onConfirmationClick:(UIConfirmationBlock)onConfirm
							 inController:(UIViewController *)controller;
+ (UIConfirmationDialog *)ShowWithAttributedMessage:(NSMutableAttributedString *)attributedText
                                      cancelMessage:(NSString *)cancel
                                     confirmMessage:(NSString *)confirm
                                      onCancelClick:(UIConfirmationBlock)onCancel
                                onConfirmationClick:(UIConfirmationBlock)onConfirm;

@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *cancelButton;
@property (weak, nonatomic) IBOutlet UIImageView *securityImage;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *confirmationButton;
@property (weak, nonatomic) IBOutlet UIView *authView;
@property(weak, nonatomic) IBOutlet UILabel *titleLabel;
@property (weak, nonatomic) IBOutlet UIButton *authButton;

- (void)setSpecialColor;
- (IBAction)onCancelClick:(id)sender;
- (IBAction)onConfirmationClick:(id)sender;
- (IBAction)onAuthClick:(id)sender;
- (void)dismiss;
@end
