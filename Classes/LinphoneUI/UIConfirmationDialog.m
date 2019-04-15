//
//  UIConfirmationDialog.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

#import "UIConfirmationDialog.h"
#import "PhoneMainView.h"

@implementation UIConfirmationDialog
+ (UIConfirmationDialog *)initDialog:(NSString *)cancel
                           confirmMessage:(NSString *)confirm
                            onCancelClick:(UIConfirmationBlock)onCancel
                      onConfirmationClick:(UIConfirmationBlock)onConfirm
                             inController:(UIViewController *)controller {
    UIConfirmationDialog *dialog =
    [[UIConfirmationDialog alloc] initWithNibName:NSStringFromClass(self.class) bundle:NSBundle.mainBundle];
    
    dialog.view.frame = PhoneMainView.instance.mainViewController.view.frame;
    [controller.view addSubview:dialog.view];
    [controller addChildViewController:dialog];
    
    dialog->onCancelCb = onCancel;
    dialog->onConfirmCb = onConfirm;
    
    if (cancel) {
        [dialog.cancelButton setTitle:cancel forState:UIControlStateNormal];
    }
    if (confirm) {
        [dialog.confirmationButton setTitle:confirm forState:UIControlStateNormal];
    }
    
    dialog.confirmationButton.layer.borderColor =
    [[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]] CGColor];
    dialog.cancelButton.layer.borderColor =
    [[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_F.png"]] CGColor];
    return dialog;
}

+ (UIConfirmationDialog *)ShowWithMessage:(NSString *)message
							cancelMessage:(NSString *)cancel
						   confirmMessage:(NSString *)confirm
							onCancelClick:(UIConfirmationBlock)onCancel
					  onConfirmationClick:(UIConfirmationBlock)onConfirm
							 inController:(UIViewController *)controller {
	UIConfirmationDialog *dialog =
    [UIConfirmationDialog initDialog:cancel confirmMessage:confirm onCancelClick:onCancel onConfirmationClick:onConfirm inController:controller];
    [dialog.titleLabel setText:message];
	return dialog;
}

+ (UIConfirmationDialog *)ShowWithMessage:(NSString *)message
							cancelMessage:(NSString *)cancel
						   confirmMessage:(NSString *)confirm
							onCancelClick:(UIConfirmationBlock)onCancel
					  onConfirmationClick:(UIConfirmationBlock)onConfirm {
	return [self ShowWithMessage:message
				   cancelMessage:cancel
				  confirmMessage:confirm
				   onCancelClick:onCancel
			 onConfirmationClick:onConfirm
					inController:PhoneMainView.instance.mainViewController];
}

+ (UIConfirmationDialog *)ShowWithAttributedMessage:(NSMutableAttributedString *)attributedText
                            cancelMessage:(NSString *)cancel
                           confirmMessage:(NSString *)confirm
                            onCancelClick:(UIConfirmationBlock)onCancel
                      onConfirmationClick:(UIConfirmationBlock)onConfirm {
    UIConfirmationDialog *dialog =
    [UIConfirmationDialog initDialog:cancel confirmMessage:confirm onCancelClick:onCancel onConfirmationClick:onConfirm inController:PhoneMainView.instance.mainViewController];
    dialog.titleLabel.attributedText = attributedText;
    return dialog;
}

- (void)setSpecialColor {
	[_confirmationButton setBackgroundImage:[UIImage imageNamed:@"color_L.png"] forState:UIControlStateNormal];
	[_cancelButton setBackgroundImage:[UIImage imageNamed:@"color_I.png"] forState:UIControlStateNormal];
	[_cancelButton setTitleColor:[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_H.png"]] forState:UIControlStateNormal];
	
	_confirmationButton.layer.borderColor =
	[[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_L.png"]] CGColor];
	_cancelButton.layer.borderColor =
	[[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]] CGColor];
}

- (IBAction)onCancelClick:(id)sender {
	[self.view removeFromSuperview];
	[self removeFromParentViewController];
	if (onCancelCb) {
		onCancelCb();
	}
}

- (IBAction)onConfirmationClick:(id)sender {
	[self.view removeFromSuperview];
	[self removeFromParentViewController];
	if (onConfirmCb) {
		onConfirmCb();
	}
}

- (IBAction)onAuthClick:(id)sender {
    BOOL notAskAgain = ![LinphoneManager.instance lpConfigBoolForKey:@"confirmation_dialog_before_sas_call_not_ask_again"];
    UIImage *image = notAskAgain ? [UIImage imageNamed:@"checkbox_checked.png"] : [UIImage imageNamed:@"checkbox_unchecked.png"];
    [_authButton setImage:image forState:UIControlStateNormal];
    [LinphoneManager.instance lpConfigSetBool:notAskAgain forKey:@"confirmation_dialog_before_sas_call_not_ask_again"];
}

- (void)dismiss {
	[self onCancelClick:nil];
}
@end
