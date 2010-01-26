/*
 *  FirstLoginViewController.h
 *
 * Description: 
 *
 *
 * Belledonne Communications (C) 2009
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */


#import <UIKit/UIKit.h>
#include "linphoneAppDelegate.h"


@interface FirstLoginViewController : UIViewController <UITextFieldDelegate>{
	UIButton* ok;
	UIButton* site;
	UITextField* identity;
	UITextField* passwd;
	UITextField* domain;
	UITextField* axtelPin;
	UIView* activityIndicator;
	
	id<LinphoneManagerDelegate> mainDelegate;

}
-(void) doOk:(id)sender;
-(void) callStateChange:(LinphoneGeneralState*) state;
-(void) authInfoRequested;
@property (nonatomic, retain) IBOutlet UIButton* ok;
@property (nonatomic, retain) IBOutlet UIButton* site;
@property (nonatomic, retain) IBOutlet UITextField* identity;
@property (nonatomic, retain) IBOutlet UITextField* passwd;
@property (nonatomic, retain) IBOutlet UITextField* domain;
@property (nonatomic, retain) IBOutlet UITextField* axtelPin;
@property (nonatomic, retain) IBOutlet UIView* activityIndicator;
@property (nonatomic, retain) id<LinphoneManagerDelegate> mainDelegate;
@end
