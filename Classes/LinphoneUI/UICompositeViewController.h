/* UICompositeViewController.h
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
#import <QuartzCore/QuartzCore.h>

#import "LinphoneManager.h"

@interface UICompositeViewDescription: NSObject{
@public
    NSString *content;
    NSString *stateBar;
    BOOL stateBarEnabled;
    NSString *tabBar;
    BOOL tabBarEnabled;
    BOOL fullscreen;
}
- (id)copy;
- (id)init:(NSString *)content stateBar:(NSString*)stateBar 
                        stateBarEnabled:(BOOL) stateBarEnabled 
                                 tabBar:(NSString*)tabBar
                          tabBarEnabled:(BOOL) tabBarEnabled
                             fullscreen:(BOOL) fullscreen;

@end

@protocol UICompositeViewDelegate <NSObject>

+ (UICompositeViewDescription*) compositeViewDescription;

@end

@interface UICompositeViewController : UIViewController {
    @private
    UIView *stateBarView;
    UIViewController *stateBarViewController;
    UIView *contentView;
    UIViewController *contentViewController;
    UIView *tabBarView;
    UIViewController *tabBarViewController;
    
    NSMutableDictionary *viewControllerCache;
    
    UICompositeViewDescription *currentViewDescription;
    CATransition *viewTransition;
}

@property (strong) CATransition *viewTransition;

@property (nonatomic, retain) IBOutlet UIView* stateBarView;
@property (nonatomic, retain) IBOutlet UIView* contentView;
@property (nonatomic, retain) IBOutlet UIView* tabBarView;

- (void) changeView:(UICompositeViewDescription *)description;
- (void) setFullScreen:(BOOL) enabled;
- (void) setToolBarHidden:(BOOL) hidden;

- (UIViewController *) getCurrentViewController;

@end
