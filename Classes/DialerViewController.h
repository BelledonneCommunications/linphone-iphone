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
#import "UICallButton.h"
#import "UITransferButton.h"
#import "UIDigitButton.h"

@interface DialerViewController : UIViewController <UITextFieldDelegate, UICompositeViewDelegate> {
@private
	//Buttons
	UITextField*    addressField;
    UIButton*       addContactButton;
    UIButton*       backButton;
	UIEraseButton*  eraseButton;
	UICallButton*   callButton;
    UICallButton*   addCallButton;
    UITransferButton*   transferButton;

	//Key pad
	UIDigitButton*  oneButton;
	UIDigitButton*  twoButton;
	UIDigitButton*  threeButton;
	UIDigitButton*  fourButton;
	UIDigitButton*  fiveButton;
	UIDigitButton*  sixButton;
	UIDigitButton*  sevenButton;
	UIDigitButton*  eightButton;
	UIDigitButton*  nineButton;
	UIDigitButton*  starButton;
	UIDigitButton*  zeroButton;
	UIDigitButton*  sharpButton;
    
    BOOL transferMode;
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

@property (nonatomic, retain) IBOutlet UIButton* oneButton;
@property (nonatomic, retain) IBOutlet UIButton* twoButton;
@property (nonatomic, retain) IBOutlet UIButton* threeButton;
@property (nonatomic, retain) IBOutlet UIButton* fourButton;
@property (nonatomic, retain) IBOutlet UIButton* fiveButton;
@property (nonatomic, retain) IBOutlet UIButton* sixButton;
@property (nonatomic, retain) IBOutlet UIButton* sevenButton;
@property (nonatomic, retain) IBOutlet UIButton* eightButton;
@property (nonatomic, retain) IBOutlet UIButton* nineButton;
@property (nonatomic, retain) IBOutlet UIButton* starButton;
@property (nonatomic, retain) IBOutlet UIButton* zeroButton;
@property (nonatomic, retain) IBOutlet UIButton* sharpButton;

- (IBAction)onAddContactClick: (id) event;
- (IBAction)onBackClick: (id) event;
- (IBAction)onAddressChange: (id)sender;

@end
