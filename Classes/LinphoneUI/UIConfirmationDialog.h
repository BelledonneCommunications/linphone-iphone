//
//  UIConfirmationDialog.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

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

@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *cancelButton;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *confirmationButton;
@property(weak, nonatomic) IBOutlet UILabel *titleLabel;
- (IBAction)onCancelClick:(id)sender;
- (IBAction)onConfirmationClick:(id)sender;
- (void)dismiss;
@end
