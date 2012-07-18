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
                                                             fullscreen:false];
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
    settingsController.settingsReaderDelegate = self;
    settingsController.settingsStore=[[LinphoneManager instance] settingsStore];
    settingsController.showCreditsFooter = FALSE;
    
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
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [settingsController viewWillAppear:animated];
    }   
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


#pragma mark - IASKSettingsReaderFilterDelegate Functions

- (NSDictionary*)filterPreferenceSpecifier:(NSDictionary *)specifier {
    if (![LinphoneManager isLcReady]) {
        // LinphoneCore not ready: do not filter
        return specifier;
    }
    NSString* identifier = [specifier objectForKey:@"Identifier"];
    if (identifier == nil) {
        identifier = [specifier objectForKey:@"Key"];
    }
    if (!identifier) {
        // child pane maybe
        NSString* title = [specifier objectForKey:@"Title"];
        if ([title isEqualToString:@"Video"]) {
            if (!linphone_core_video_supported([LinphoneManager getLc]))
                return nil;
        }
        return specifier;
    }
    // NSLog(@"Specifier received: %@", identifier);
	if ([identifier isEqualToString:@"silk_24k_preference"]) {
		if (![LinphoneManager isNotIphone3G])
			return nil;
	}
    if ([identifier isEqualToString:@"backgroundmode_preference"]) {
        UIDevice* device = [UIDevice currentDevice];
        if ([device respondsToSelector:@selector(isMultitaskingSupported)]) {
            if ([device isMultitaskingSupported]) {
                return specifier;
            }
        }
        // hide setting if bg mode not supported
        return nil;
    }
    if ([identifier isEqualToString:@"enable_first_login_view_preference"]) {
        // hide first login view preference
        return nil;
    }
	if (![LinphoneManager codecIsSupported:identifier])
		return Nil;
    return specifier;
}


#pragma mark - IASKSettingsDelegate Functions

- (void)settingsViewControllerDidEnd:(IASKAppSettingsViewController *)sender {
}

@end
