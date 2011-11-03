/* PhoneViewController.h
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
#import <Foundation/Foundation.h>
#import "linphonecore.h"
#import "UILinphone.h"
#import "CallDelegate.h"


@class IncallViewController;
@class FirstLoginViewController;

@interface PhoneViewController : UIViewController <UITextFieldDelegate,LinphoneUICallDelegate, UIActionSheetCustomDelegate> {

@private
	//UI definition
	UIView* dialerView;
	UITextField* address;
	UILabel* mDisplayName;
	UIEraseButton* erase;
	
	UIView* incallView;
	UIDuration* callDuration;
	UIMuteButton* mute;
	UISpeakerButton* speaker;	
	UILabel* peerLabel;
		
	
	UICallButton* __call;
	UIHangUpButton* hangup;

	UILabel* status;

	//key pad
	UIDigitButton* one;
	UIDigitButton* two;
	UIDigitButton* three;
	UIDigitButton* four;
	UIDigitButton* five;
	UIDigitButton* six;
	UIDigitButton* seven;
	UIDigitButton* eight;
	UIDigitButton* nine;
	UIDigitButton* star;
	UIDigitButton* zero;
	UIDigitButton* hash;

	UIButton* back;
	
	UITabBarController*  myTabBarController;

	UIActionSheet *mIncomingCallActionSheet;
	FirstLoginViewController* myFirstLoginViewController;
	
}
@property (nonatomic, retain) IBOutlet UIView* dialerView;
@property (nonatomic, retain) IBOutlet UITextField* address;
@property (nonatomic, retain) IBOutlet UIButton* __call;
@property (nonatomic, retain) IBOutlet UIButton* hangup;
@property (nonatomic, retain) IBOutlet UILabel* status;
@property (nonatomic, retain) IBOutlet UIEraseButton* erase;

@property (nonatomic, retain) IBOutlet UIView* incallView;
@property (nonatomic, retain) IBOutlet UILabel* callDuration;
@property (nonatomic, retain) IBOutlet UIButton* mute;
@property (nonatomic, retain) IBOutlet UIButton* speaker;	
@property (nonatomic, retain) IBOutlet UILabel* peerLabel;


@property (nonatomic, retain) IBOutlet UIButton* one;
@property (nonatomic, retain) IBOutlet UIButton* two;
@property (nonatomic, retain) IBOutlet UIButton* three;
@property (nonatomic, retain) IBOutlet UIButton* four;
@property (nonatomic, retain) IBOutlet UIButton* five;
@property (nonatomic, retain) IBOutlet UIButton* six;
@property (nonatomic, retain) IBOutlet UIButton* seven;
@property (nonatomic, retain) IBOutlet UIButton* eight;
@property (nonatomic, retain) IBOutlet UIButton* nine;
@property (nonatomic, retain) IBOutlet UIButton* star;
@property (nonatomic, retain) IBOutlet UIButton* zero;
@property (nonatomic, retain) IBOutlet UIButton* hash;

@property (nonatomic, retain) IBOutlet UIButton* back;



// method to handle keypad event
- (IBAction)doKeyPad:(id)sender;


@property (nonatomic, retain) IBOutlet UITabBarController*  myTabBarController;
@end
