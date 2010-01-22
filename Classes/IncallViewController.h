/* IncallViewController.h
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
#import "linphonecore.h"
#import "PhoneViewController.h"
#import <AddressBookUI/ABPeoplePickerNavigationController.h>


@interface IncallViewController : UIViewController <ABPeoplePickerNavigationControllerDelegate> {
	LinphoneCore* myLinphoneCore;
	id<PhoneViewControllerDelegate> phoneviewDelegate;
	NSTimer *durationRefreasher;
	
	
	UIView* controlSubView;
	UIView* padSubView;
	
	UILabel* peerName;
	UILabel* peerNumber;
	UILabel* callDuration;
	UILabel* status;
	UIButton* end;
	UIButton* dialer;
	UIButton* mute;
	UIButton* speaker;
	UIButton* contacts;
	
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
	
	UIButton* close;
	
	bool isMuted;
	bool isSpeaker;
	
	ABPeoplePickerNavigationController* myPeoplePickerController;
}

-(void) setLinphoneCore:(LinphoneCore*) lc;
-(void) startCall;

-(void)displayStatus:(NSString*) message;

- (IBAction)doAction:(id)sender;

@property (nonatomic, retain) IBOutlet UIView* controlSubView;
@property (nonatomic, retain) IBOutlet UIView* padSubView;

@property (nonatomic, retain) IBOutlet UILabel* peerName;
@property (nonatomic, retain) IBOutlet UILabel* peerNumber;
@property (nonatomic, retain) IBOutlet UILabel* callDuration;
@property (nonatomic, retain) IBOutlet UILabel* status;
@property (nonatomic, retain) IBOutlet UIButton* end;
@property (nonatomic, retain) IBOutlet UIButton* dialer;
@property (nonatomic, retain) IBOutlet UIButton* mute;
@property (nonatomic, retain) IBOutlet UIButton* speaker;
@property (nonatomic, retain) IBOutlet UIButton* contacts;


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
@property (nonatomic, retain) IBOutlet UIButton* close;

@property (nonatomic, retain) id<PhoneViewControllerDelegate> phoneviewDelegate;
@end
