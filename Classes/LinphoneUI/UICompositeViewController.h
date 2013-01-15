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
#import "TPMultiLayoutViewController.h"

@interface UICompositeViewDescription: NSObject{
}

@property (retain) NSString *name;
@property (retain) NSString *content;
@property (retain) NSString *stateBar;
@property (assign) BOOL stateBarEnabled;
@property (retain) NSString *tabBar;
@property (assign) BOOL tabBarEnabled;
@property (assign) BOOL fullscreen;
@property (assign) BOOL landscapeMode;
@property (assign) BOOL portraitMode;

- (id)copy;
- (BOOL)equal:(UICompositeViewDescription*) description;
- (id)init:(NSString *)name content:(NSString *)content stateBar:(NSString*)stateBar 
                        stateBarEnabled:(BOOL) stateBarEnabled 
                                 tabBar:(NSString*)tabBar
                          tabBarEnabled:(BOOL) tabBarEnabled
                             fullscreen:(BOOL) fullscreen
                          landscapeMode:(BOOL) landscapeMode
                           portraitMode:(BOOL) portraitMode;

@end

@protocol UICompositeViewDelegate <NSObject>

+ (UICompositeViewDescription*) compositeViewDescription;

@end

@interface UICompositeViewController : TPMultiLayoutViewController {
    @private
    NSMutableDictionary *viewControllerCache;
    UICompositeViewDescription *currentViewDescription;
    UIInterfaceOrientation currentOrientation;
}

@property (retain) CATransition *viewTransition;

@property (nonatomic, retain) IBOutlet UIView* stateBarView;
@property (nonatomic, retain) IBOutlet UIView* contentView;
@property (nonatomic, retain) IBOutlet UIView* tabBarView;

- (void)changeView:(UICompositeViewDescription *)description;
- (void)setFullScreen:(BOOL) enabled;
- (void)setStateBarHidden:(BOOL) hidden;
- (void)setToolBarHidden:(BOOL) hidden;
- (UIViewController *)getCachedController:(NSString*)name;
- (UIViewController *)getCurrentViewController;
- (UIInterfaceOrientation)currentOrientation;
- (void)clearCache:(NSArray*)exclude;

@end
