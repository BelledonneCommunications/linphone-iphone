/* PhoneMainView.h
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
#import "LinphoneManager.h"

@interface PhoneMainView : UIViewController {
    UIView *statusBarView;
    UIView *contentView;
    UIView *tabBarView;
    PhoneView currentView;
    
    @private
    NSMutableDictionary *viewDescriptions;
    NSArray *views;
    
    UIViewController *statusBarController;
    
    UIViewController *callTabBarController;
    UIViewController *mainTabBarController;
    UIViewController *incomingCallTabBarController;
    
}
@property (nonatomic, retain) IBOutlet UIView* statusBarView;
@property (nonatomic, retain) IBOutlet UIView* contentView;
@property (nonatomic, retain) IBOutlet UIView* tabBarView;

@property (nonatomic, retain) IBOutlet UIViewController* statusBarController;

@property (nonatomic, retain) IBOutlet UIViewController* callTabBarController;
@property (nonatomic, retain) IBOutlet UIViewController* mainTabBarController;
@property (nonatomic, retain) IBOutlet UIViewController* incomingCallTabBarController;

-(void) changeView: (NSNotification*) notif;
@end
