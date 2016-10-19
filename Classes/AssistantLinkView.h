//
//  AssistantLinkView.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 29/08/16.
//
//

#import "PhoneMainView.h"
#import <UIKit/UIKit.h>

@interface AssistantLinkView : UIViewController <UITextFieldDelegate, UICompositeViewDelegate>
@property(weak, nonatomic) IBOutlet UIView *linkAccountView;
@property(weak, nonatomic) IBOutlet UIView *activateSMSView;

@property(weak, nonatomic) IBOutlet UIButton *countryButton;
@property(weak, nonatomic) IBOutlet UITextField *countryCodeField;
@property(weak, nonatomic) IBOutlet UITextField *activationCodeField;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *linkAccountButton;
@property(weak, nonatomic) IBOutlet UIRoundBorderedButton *checkValidationButton;
@property(weak, nonatomic) IBOutlet UIView *waitView;
@property(weak, nonatomic) IBOutlet UITextField *phoneField;
@property (weak, nonatomic) IBOutlet UILabel *linkSMSText;
@property BOOL firstTime;

- (IBAction)onLinkAccount:(id)sender;
- (IBAction)onCheckValidationButton:(id)sender;
- (IBAction)onCountryClick:(id)sender;
- (IBAction)onDialerClick:(id)sender;
- (IBAction)onPhoneNumberDisclosureClick:(id)sender;

@end
