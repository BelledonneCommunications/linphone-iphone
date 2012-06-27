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

#import "UILinphone.h"

@interface DialerViewController : UIViewController <UITextFieldDelegate>{

@private
	//Buttons
	UITextField*    addressField;
    UIButton*       addContactButton;
    UIButton*       cancelButton;
	UIEraseButton*  eraseButton;
	UICallButton*   callButton;
    UICallButton*   addCallButton;

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
	UIDigitButton*  hashButton;
}

- (void)setAddress:(NSString*) address;

@property (nonatomic, retain) IBOutlet UITextField* addressField;
@property (nonatomic, retain) IBOutlet UIButton* addContactButton;
@property (nonatomic, retain) IBOutlet UICallButton* callButton;
@property (nonatomic, retain) IBOutlet UICallButton* addCallButton;
@property (nonatomic, retain) IBOutlet UIButton* cancelButton;
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
@property (nonatomic, retain) IBOutlet UIButton* hashButton;

- (IBAction)onAddContactClick: (id) event;
- (IBAction)onCancelClick: (id) event;
- (IBAction)onAddCallClick: (id) event;
- (IBAction)onAddressChange: (id)sender;

@end
