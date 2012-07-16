/* HistoryDetailsViewController.h
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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
#import "linphonecore.h"

#import <AddressBook/AddressBook.h>
#import "UICompositeViewController.h"

@interface HistoryDetailsViewController : UIViewController<UICompositeViewDelegate> {
    LinphoneCallLog *callLog;
    UIImageView *avatarImage;
    UILabel *addressLabel;
    UILabel *dateLabel;
    UILabel *dateHeaderLabel;
    UILabel *durationLabel;
    UILabel *durationHeaderLabel;
    UILabel *typeLabel;
    UILabel *typeHeaderLabel;
    UIButton *addressButton;
    UIButton *addContactButton;
    ABRecordRef contact;
}
@property (nonatomic, retain) IBOutlet  UIImageView *avatarImage;
@property (nonatomic, retain) IBOutlet UILabel *addressLabel;
@property (nonatomic, retain) IBOutlet UILabel *dateLabel;
@property (nonatomic, retain) IBOutlet UILabel *dateHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel *durationLabel;
@property (nonatomic, retain) IBOutlet UILabel *durationHeaderLabel;
@property (nonatomic, retain) IBOutlet UILabel *typeLabel;
@property (nonatomic, retain) IBOutlet UILabel *typeHeaderLabel;
@property (nonatomic, retain) IBOutlet UIButton *addressButton;
@property (nonatomic, retain) IBOutlet UIButton *addContactButton;
@property (nonatomic, assign) LinphoneCallLog *callLog;

- (IBAction)onBackClick:(id)event;
- (IBAction)onContactClick:(id)event;
- (IBAction)onAddContactClick:(id)event;
- (IBAction)onAddressClick:(id)event;

@end
