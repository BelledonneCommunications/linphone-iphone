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

+ (void)ShowWithMessage:(NSString *)message
		  onCancelClick:(UIConfirmationBlock)onCancel
	onConfirmationClick:(UIConfirmationBlock)onConfirm {
	UIConfirmationDialog *dialog =
		[[UIConfirmationDialog alloc] initWithNibName:NSStringFromClass(self.class) bundle:NSBundle.mainBundle];

	[PhoneMainView.instance.mainViewController.view addSubview:dialog.view];
	[PhoneMainView.instance.mainViewController addChildViewController:dialog];

	dialog->onCancelCb = onCancel;
	dialog->onConfirmCb = onConfirm;

	[dialog.titleLabel setText:message];
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
@end
