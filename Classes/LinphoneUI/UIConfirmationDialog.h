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

+ (void)ShowWithMessage:(NSString *)message
		  onCancelClick:(UIConfirmationBlock)onCancel
	onConfirmationClick:(UIConfirmationBlock)onConfirm;

@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *cancelButton;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *confirmationButton;
@property(weak, nonatomic) IBOutlet UILabel *titleLabel;
- (IBAction)onCancelClick:(id)sender;
- (IBAction)onConfirmationClick:(id)sender;

@end
