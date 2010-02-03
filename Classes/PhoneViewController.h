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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */                          
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "linphonecore.h"

@protocol PhoneViewControllerDelegate

-(void)setPhoneNumber:(NSString*)number;
-(void)dismissIncallView;
-(void)displayStatus:(NSString*) message;
@end
@class IncallViewController;

@interface PhoneViewController : UIViewController <UITextFieldDelegate,PhoneViewControllerDelegate> {

@private
	//UI definition
	UITextField* address;
	UIButton* call;
	UIButton* hangup;

	UILabel* status;

	//key pad
	UIButton* one;
	UIButton* two;
	UIButton* three;
	UIButton* four;
	UIButton* five;
	UIButton* six;
	UIButton* seven;
	UIButton* eight;
	UIButton* nine;
	UIButton* star;
	UIButton* zero;
	UIButton* hash;

	UIButton* back;
	/*
	 * lib linphone main context
	 */
	LinphoneCore* mCore;
	IncallViewController *myIncallViewController;
	
}
@property (nonatomic, retain) IBOutlet UITextField* address;
@property (nonatomic, retain) IBOutlet UIButton* call;
@property (nonatomic, retain) IBOutlet UIButton* hangup;
@property (nonatomic, retain) IBOutlet UILabel* status;

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



/*
 * Handle call state change from linphone
 */
-(void) callStateChange:(LinphoneGeneralState*) state;

-(void) setLinphoneCore:(LinphoneCore*) lc;

/********************
 * UI method handlers
 ********************/
-(void)doKeyZeroLongPress;

//method to handle cal/hangup events
- (IBAction)doAction:(id)sender;

// method to handle keypad event
- (IBAction)doKeyPad:(id)sender;

- (IBAction)doKeyPadUp:(id)sender;

-(void) dismissAlertDialog:(UIAlertView*)alertView;


@end
