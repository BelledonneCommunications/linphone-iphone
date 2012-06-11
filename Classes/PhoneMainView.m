/* PhoneMainView.m
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

#import "PhoneMainView.h"
#import "CallHistoryTableViewController.h"
typedef enum _TabBar {
    TabBar_Main,
    TabBar_END
} TabBar;


@interface ViewsDescription: NSObject{
    @public
    UIViewController *content;
    UIViewController *tabBar;
    bool statusEnabled;
}
@end
@implementation ViewsDescription
@end

@implementation PhoneMainView

@synthesize statusBarView;
@synthesize contentView;
@synthesize tabBarView;

@synthesize callTabBar;
@synthesize mainTabBar;
@synthesize incomingTabBar;
@synthesize addCallTabBar;

- (void) changeView: (NSNotification*) notif {   
    PhoneView view = [[notif.userInfo objectForKey: @"PhoneView"] intValue];
    ViewsDescription *description = [viewDescriptions objectForKey:[NSNumber numberWithInt: view]];
    for (UIView *view in contentView.subviews) {
        [view removeFromSuperview];
    }
    for (UIView *view in tabBarView.subviews) {
        [view removeFromSuperview];
    }
    
    [contentView addSubview: description->content.view];
    
 CGRect contentFrame = contentView.frame;
    if(description->statusEnabled) {
        statusBarView.hidden = false;
        contentFrame.origin.y = statusBarView.frame.size.height + statusBarView.frame.origin.y;
    } else {
        statusBarView.hidden = true;
        contentFrame.origin.y = 0;
    }
    
  
    // Resize tabbar
    CGRect tabFrame = tabBarView.frame;
    tabFrame.origin.y += tabFrame.size.height;
    tabFrame.origin.x += tabFrame.size.width;
    tabFrame.size.height = description->tabBar.view.frame.size.height;
    tabFrame.size.width = description->tabBar.view.frame.size.width;
    tabFrame.origin.y -= tabFrame.size.height;
    tabFrame.origin.x -= tabFrame.size.width;
    tabBarView.frame = tabFrame;
    for (UIView *view in description->tabBar.view.subviews) {
        if(view.tag == -1) {
            contentFrame.size.height = tabFrame.origin.y - contentFrame.origin.y + view.frame.origin.y;
        }
    }
    
    
   // contentView.frame = contentFrame;
    [tabBarView addSubview: description->tabBar.view];
}

-(void)viewDidLoad {
    [super viewDidLoad];
    UIView *dumb;
    
    // Init view descriptions
    viewDescriptions = [[NSMutableDictionary alloc] init];
    
    // Load Bars
    dumb = mainTabBar.view;
    
    // Main View
    PhoneViewController* myPhoneViewController = [[PhoneViewController alloc]  
        initWithNibName:@"PhoneViewController" 
        bundle:[NSBundle mainBundle]];
    [myPhoneViewController loadView];
    ViewsDescription *mainViewDescription = [ViewsDescription alloc];
    mainViewDescription->content = myPhoneViewController;
    mainViewDescription->tabBar = mainTabBar;
    mainViewDescription->statusEnabled = true;
    [viewDescriptions setObject:mainViewDescription forKey:[NSNumber numberWithInt: PhoneView_Main]];
    
    // Call History View
    CallHistoryTableViewController* myCallHistoryTableViewController = [[CallHistoryTableViewController alloc]
        initWithNibName:@"CallHistoryTableViewController" 
        bundle:[NSBundle mainBundle]];
    [myCallHistoryTableViewController loadView];
    ViewsDescription *callHistoryDescription = [ViewsDescription alloc];
    callHistoryDescription->content = myCallHistoryTableViewController;
    callHistoryDescription->tabBar = mainTabBar;
    callHistoryDescription->statusEnabled = true;
    [viewDescriptions setObject:callHistoryDescription forKey:[NSNumber numberWithInt: PhoneView_CallHistory]];
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(changeView:) name:@"LinphoneMainViewChange" object:nil];
    
    // Change to default view
    NSDictionary* dict = [NSDictionary dictionaryWithObject: [NSNumber numberWithInt:PhoneView_Main] forKey:@"PhoneView"];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"LinphoneMainViewChange" object:self userInfo:dict];
}
     
- (void)dealloc {
    [super dealloc];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}
@end