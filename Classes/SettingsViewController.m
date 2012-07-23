/* SettingsViewController.m
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

#import "SettingsViewController.h"
#import "LinphoneManager.h"

@implementation SettingsViewController

@synthesize settingsController;
@synthesize navigationController;

#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"SettingsViewController" bundle:[NSBundle mainBundle]];
}


- (void)dealloc {
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if(compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:@"Settings" 
                                                                content:@"SettingsViewController" 
                                                               stateBar:nil 
                                                        stateBarEnabled:false 
                                                                 tabBar: @"UIMainBar" 
                                                          tabBarEnabled:true 
                                                             fullscreen:false
                                                          landscapeMode:false
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - 

+ (void)removeBackground:(UIView*)view {
    if([view isKindOfClass:[UITableView class]]) {
          [view setBackgroundColor:[UIColor clearColor]];  
    }
    for(UIView *subview in [view subviews]) {
        [SettingsViewController removeBackground:subview];
    }
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    settingsController.delegate = self;
    settingsController.showCreditsFooter = FALSE;
    settingsController.hiddenKeys = [self findHiddenKeys];
    settingsController.settingsStore = [[LinphoneManager instance] settingsStore];
    
    navigationController.view.frame = self.view.frame;
    [SettingsViewController removeBackground:navigationController.view];
    [SettingsViewController removeBackground:settingsController.view];
    [self.view addSubview: navigationController.view];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [settingsController viewWillDisappear:animated];
    }
    [settingsController dismiss:self];
    // Set observer
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kIASKAppSettingChanged 
                                               object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [settingsController viewWillAppear:animated];
    }   
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(appSettingChanged:) 
                                                 name:kIASKAppSettingChanged
                                               object:nil];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [settingsController viewDidAppear:animated];
    }   
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [settingsController viewDidDisappear:animated];
    }  
}


#pragma mark - Event Functions

- (void)appSettingChanged:(NSNotification*) notif {
    if([@"enable_video_preference" compare: notif.object] == NSOrderedSame) {
        BOOL enable = [[notif.userInfo objectForKey:@"enable_video_preference"] boolValue];
        NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[settingsController hiddenKeys]];
        if(!enable) {
            [hiddenKeys addObject:@"video_menu"];
        } else {
            [hiddenKeys removeObject:@"video_menu"];
        }
        [settingsController setHiddenKeys:hiddenKeys animated:TRUE];
    }else if([@"random_port_preference" compare: notif.object] == NSOrderedSame) {
        BOOL enable = [[notif.userInfo objectForKey:@"random_port_preference"] boolValue];
        NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[settingsController hiddenKeys]];
        if(enable) {
            [hiddenKeys addObject:@"port_preference"];
        } else {
            [hiddenKeys removeObject:@"port_preference"];
        }
        [settingsController setHiddenKeys:hiddenKeys animated:TRUE];
    }
}


#pragma mark - 

- (NSSet*)findHiddenKeys {
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger log:LinphoneLoggerWarning format:@"Can't filter settings: Linphone core not read"];
    }
    NSMutableSet *hiddenKeys = [NSMutableSet set];
    
    if (!linphone_core_video_supported([LinphoneManager getLc]))
        [hiddenKeys addObject:@"video_menu"];
    
    if (![LinphoneManager isNotIphone3G])
        [hiddenKeys addObject:@"silk_24k_preference"];
    
    UIDevice* device = [UIDevice currentDevice];
    if (![device respondsToSelector:@selector(isMultitaskingSupported)] || ![device isMultitaskingSupported]) {
        [hiddenKeys addObject:@"backgroundmode_preference"];
    }
    
    [hiddenKeys addObject:@"enable_first_login_view_preference"];
    
    if (!linphone_core_video_enabled([LinphoneManager getLc])) {
        [hiddenKeys addObject:@"video_menu"];
    }
    
    [hiddenKeys addObjectsFromArray:[[LinphoneManager unsupportedCodecs] allObjects]];
    
    if([[[[LinphoneManager instance] settingsStore] objectForKey:@"random_port_preference"] boolValue]) {
        [hiddenKeys addObject:@"port_preference"];
    }

    return hiddenKeys;
}


#pragma mark - IASKSettingsDelegate Functions

- (void)settingsViewControllerDidEnd:(IASKAppSettingsViewController *)sender {
}

@end
