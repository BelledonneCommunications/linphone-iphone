/* DialerViewController.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */       

#import <UIKit/UIKit.h>

#import "UICompositeViewController.h"

#import "UIEraseButton.h"
#import "UICamSwitch.h"
#import "UICallButton.h"
#import "UITransferButton.h"
#import "UIDigitButton.h"

@interface DialerViewController : UIViewController <UITextFieldDelegate, UICompositeViewDelegate> {
}

- (void)setAddress:(NSString*)address;
- (void)call:(NSString*)address displayName:(NSString *)displayName;
- (void)call:(NSString*)address;

@property (nonatomic, assign) BOOL transferMode;

@property (nonatomic, retain) IBOutlet UITextField* addressField;
@property (nonatomic, retain) IBOutlet UIButton* addContactButton;
@property (nonatomic, retain) IBOutlet UICallButton* callButton;
@property (nonatomic, retain) IBOutlet UICallButton* addCallButton;
@property (nonatomic, retain) IBOutlet UITransferButton* transferButton;
@property (nonatomic, retain) IBOutlet UIButton* backButton;
@property (nonatomic, retain) IBOutlet UIEraseButton* eraseButton;

@property (nonatomic, retain) IBOutlet UIDigitButton* oneButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* twoButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* threeButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* fourButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* fiveButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* sixButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* sevenButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* eightButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* nineButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* starButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* zeroButton;
@property (nonatomic, retain) IBOutlet UIDigitButton* sharpButton;
@property (nonatomic, retain) IBOutlet UIView* backgroundView;
@property (nonatomic, retain) IBOutlet UIView* videoPreview;
@property (nonatomic, retain) IBOutlet UICamSwitch* videoCameraSwitch;

- (IBAction)onAddContactClick: (id) event;
- (IBAction)onBackClick: (id) event;
- (IBAction)onAddressChange: (id)sender;

@end
