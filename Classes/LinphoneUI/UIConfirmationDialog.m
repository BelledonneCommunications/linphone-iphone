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

+ (UIConfirmationDialog *)ShowWithMessage:(NSString *)message
							cancelMessage:(NSString *)cancel
						   confirmMessage:(NSString *)confirm
							onCancelClick:(UIConfirmationBlock)onCancel
					  onConfirmationClick:(UIConfirmationBlock)onConfirm {
	UIConfirmationDialog *dialog =
		[[UIConfirmationDialog alloc] initWithNibName:NSStringFromClass(self.class) bundle:NSBundle.mainBundle];

	[PhoneMainView.instance.mainViewController.view addSubview:dialog.view];
	[PhoneMainView.instance.mainViewController addChildViewController:dialog];

	dialog->onCancelCb = onCancel;
	dialog->onConfirmCb = onConfirm;

	[dialog.titleLabel setText:message];
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

- (void)dismiss {
	[self onCancelClick:nil];
}
@end
