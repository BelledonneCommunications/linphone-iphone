/* UIMainBar.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */  

#import <UIKit/UIKit.h>
#import "TPMultiLayoutViewController.h"

@interface UIMainBar : TPMultiLayoutViewController {
}

@property (nonatomic, retain) IBOutlet UIButton* historyButton;
@property (nonatomic, retain) IBOutlet UIButton* contactsButton;
@property (nonatomic, retain) IBOutlet UIButton* dialerButton;
@property (nonatomic, retain) IBOutlet UIButton* settingsButton;
@property (nonatomic, retain) IBOutlet UIButton* chatButton;
@property (nonatomic, retain) IBOutlet UIView *historyNotificationView;
@property (nonatomic, retain) IBOutlet UILabel *historyNotificationLabel;
@property (nonatomic, retain) IBOutlet UIView *chatNotificationView;
@property (nonatomic, retain) IBOutlet UILabel *chatNotificationLabel;

-(IBAction) onHistoryClick: (id) event;
-(IBAction) onContactsClick: (id) event;
-(IBAction) onDialerClick: (id) event;
-(IBAction) onSettingsClick: (id) event;
-(IBAction) onChatClick: (id) event;

@end
