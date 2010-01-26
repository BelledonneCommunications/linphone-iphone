/*
 *  FavoriteEditViewController.h
 *
 * Description: 
 *
 *
 * Belledonne Communications (C) 2010
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#import <UIKit/UIKit.h>
#include "linphonecore.h"


@interface FavoriteEditViewController : UIViewController {
	UIButton* ok;
	UITextField* number;
	UILabel* name;
	NSString* initialPhoneNumber;
	NSString* initialName;
	int			favoriteIndex;
	LinphoneCore* myLinphoneCore;
	
	
}
- (IBAction)doOk:(id)sender;
-(void) setLinphoneCore:(LinphoneCore*) lc;

@property (nonatomic, retain) IBOutlet UIButton* ok;
@property (nonatomic, retain) IBOutlet UITextField* number;
@property (nonatomic, retain) IBOutlet UILabel* name;
@property (nonatomic, retain) NSString* initialPhoneNumber;
@property (nonatomic, retain) NSString* initialName;

@property  int favoriteIndex;

@end

