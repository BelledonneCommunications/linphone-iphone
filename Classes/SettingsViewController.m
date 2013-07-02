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
#import "PhoneMainView.h"
#import "UILinphone.h"
#import "UACellBackgroundView.h"

#import "DCRoundSwitch.h"

#import "IASKSpecifierValuesViewController.h"
#import "IASKPSTextFieldSpecifierViewCell.h"
#import "IASKSpecifier.h"
#import "IASKTextField.h"
#include "lpconfig.h"

#ifdef DEBUG
@interface UIDevice (debug)

- (void)_setBatteryLevel:(float)level;
- (void)_setBatteryState:(int)state;

@end
#endif

@interface SettingsViewController (private)

+ (IASKSpecifier*)filterSpecifier:(IASKSpecifier *)specifier;

@end


#pragma mark - IASKSwitchEx Class

@interface IASKSwitchEx : DCRoundSwitch {
    NSString *_key;
}

@property (nonatomic, retain) NSString *key;

@end

@implementation IASKSwitchEx

@synthesize key=_key;

- (void)dealloc {
    [_key release], _key = nil;
	
    [super dealloc];
}

@end


#pragma mark - IASKSpecifierValuesViewControllerEx Class

// Patch IASKSpecifierValuesViewController
@interface IASKSpecifierValuesViewControllerEx: IASKSpecifierValuesViewController

@end

@implementation IASKSpecifierValuesViewControllerEx

- (void)initIASKSpecifierValuesViewControllerEx {
    [self.view setBackgroundColor:[UIColor clearColor]];
}

- (id)init {
    self = [super init];
    if(self != nil) {
        [self initIASKSpecifierValuesViewControllerEx];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if(self != nil) {
        [self initIASKSpecifierValuesViewControllerEx];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if(self != nil) {
        [self initIASKSpecifierValuesViewControllerEx];
    }
    return self;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell * cell = [super tableView:tableView cellForRowAtIndexPath:indexPath];
    
    // Background View
    UACellBackgroundView *selectedBackgroundView = [[[UACellBackgroundView alloc] initWithFrame:CGRectZero] autorelease];
    cell.selectedBackgroundView = selectedBackgroundView;
    [selectedBackgroundView setBackgroundColor:LINPHONE_TABLE_CELL_BACKGROUND_COLOR];
    return cell;
}

@end


#pragma mark - IASKAppSettingsViewControllerEx Class

@interface IASKAppSettingsViewController(PrivateInterface)
- (UITableViewCell*)newCellForIdentifier:(NSString*)identifier;
@end;

@interface IASKAppSettingsViewControllerEx : IASKAppSettingsViewController

@end

@implementation IASKAppSettingsViewControllerEx

- (UITableViewCell*)newCellForIdentifier:(NSString*)identifier {
	UITableViewCell *cell = nil;
	if ([identifier isEqualToString:kIASKPSToggleSwitchSpecifier]) {
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kIASKPSToggleSwitchSpecifier];
		cell.accessoryView = [[[IASKSwitchEx alloc] initWithFrame:CGRectMake(0, 0, 79, 27)] autorelease];
		[((IASKSwitchEx*)cell.accessoryView) addTarget:self action:@selector(toggledValue:) forControlEvents:UIControlEventValueChanged];
        [((IASKSwitchEx*)cell.accessoryView) setOnTintColor:LINPHONE_MAIN_COLOR];
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.textLabel.minimumFontSize = kIASKMinimumFontSize;
        cell.detailTextLabel.minimumFontSize = kIASKMinimumFontSize;
	} else {
        cell = [super newCellForIdentifier:identifier];
    }
    return cell;
}

- (void)toggledValue:(id)sender {
    IASKSwitchEx *toggle    = [[(IASKSwitchEx*)sender retain] autorelease];
    IASKSpecifier *spec   = [_settingsReader specifierForKey:[toggle key]];
    
    if ([toggle isOn]) {
        if ([spec trueValue] != nil) {
            [self.settingsStore setObject:[spec trueValue] forKey:[toggle key]];
        }
        else {
            [self.settingsStore setBool:YES forKey:[toggle key]];
        }
    }
    else {
        if ([spec falseValue] != nil) {
            [self.settingsStore setObject:[spec falseValue] forKey:[toggle key]];
        }
        else {
            [self.settingsStore setBool:NO forKey:[toggle key]];
        }
    }
    // Start notification after animation of DCRoundSwitch
    dispatch_async(dispatch_get_main_queue(), ^{
        [[NSNotificationCenter defaultCenter] postNotificationName:kIASKAppSettingChanged
                                                            object:[toggle key]
                                                          userInfo:[NSDictionary dictionaryWithObject:[self.settingsStore objectForKey:[toggle key]]
                                                                                               forKey:[toggle key]]];
    });
}

- (void)initIASKAppSettingsViewControllerEx {
    [self.view setBackgroundColor:[UIColor clearColor]];
    
    // Force kIASKSpecifierValuesViewControllerIndex
    static int kIASKSpecifierValuesViewControllerIndex = 0;
    _viewList = [[NSMutableArray alloc] init];
    [_viewList addObject:[NSDictionary dictionaryWithObjectsAndKeys:@"IASKSpecifierValuesView", @"ViewName",nil]];
    [_viewList addObject:[NSDictionary dictionaryWithObjectsAndKeys:@"IASKAppSettingsView", @"ViewName",nil]];
    
    NSMutableDictionary *newItemDict = [NSMutableDictionary dictionaryWithCapacity:3];
    [newItemDict addEntriesFromDictionary: [_viewList objectAtIndex:kIASKSpecifierValuesViewControllerIndex]];	// copy the title and explain strings
    
    IASKSpecifierValuesViewController *targetViewController = [[IASKSpecifierValuesViewControllerEx alloc] init];
    // add the new view controller to the dictionary and then to the 'viewList' array
    [newItemDict setObject:targetViewController forKey:@"viewController"];
    [_viewList replaceObjectAtIndex:kIASKSpecifierValuesViewControllerIndex withObject:newItemDict];
    [targetViewController release];
}

- (IASKSettingsReader*)settingsReader {
    IASKSettingsReader *r = [super settingsReader];
    NSMutableArray *dataSource = [NSMutableArray arrayWithArray:[r dataSource]];
    for (int i = 0; i < [dataSource count]; ++i) {
        NSMutableArray *specifiers = [NSMutableArray arrayWithArray:[dataSource objectAtIndex:i]];
        for (int j = 0; j < [specifiers count]; ++j) {
            id sp = [specifiers objectAtIndex:j];
            if ([sp isKindOfClass:[IASKSpecifier class]]) {
                sp = [SettingsViewController filterSpecifier:sp];
            }
            [specifiers replaceObjectAtIndex:j withObject:sp];
        }
        
        [dataSource replaceObjectAtIndex:i withObject:specifiers];
    }
    [r setDataSource:dataSource];
    return r;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    
    [self.tableView setBackgroundColor:[UIColor clearColor]]; // Can't do it in Xib: issue with ios4
    [self.tableView setBackgroundView:nil]; // Can't do it in Xib: issue with ios4
}

- (id)initWithStyle:(UITableViewStyle)style {
    self = [super initWithStyle:style];
    if(self != nil) {
        [self initIASKAppSettingsViewControllerEx];
    }
    return self;
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    
    UIEdgeInsets inset = {0, 0, 10, 0};
    UIScrollView *scrollView = self.tableView;
    [scrollView setContentInset:inset];
    [scrollView setScrollIndicatorInsets:inset];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    
    UIBarButtonItem *buttonItem = [[UIBarButtonItem alloc] initWithTitle:NSLocalizedString(@"About", nil) style:UIBarButtonItemStyleBordered target:self action:@selector(onAboutClick:)];
    self.navigationItem.rightBarButtonItem = buttonItem;
    [buttonItem release];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell * cell = [super tableView:tableView cellForRowAtIndexPath:indexPath];
    
    if([cell isKindOfClass:[IASKPSTextFieldSpecifierViewCell class]]) {
        UITextField *field = ((IASKPSTextFieldSpecifierViewCell*)cell).textField;
        [field setTextColor:LINPHONE_MAIN_COLOR];
    }
    
    cell.detailTextLabel.textColor = LINPHONE_MAIN_COLOR;
    
    // Background View
    UACellBackgroundView *selectedBackgroundView = [[[UACellBackgroundView alloc] initWithFrame:CGRectZero] autorelease];
    cell.selectedBackgroundView = selectedBackgroundView;
    [selectedBackgroundView setBackgroundColor:LINPHONE_TABLE_CELL_BACKGROUND_COLOR];
    return cell;
}

- (IBAction)onAboutClick: (id)sender {
    [[PhoneMainView instance] changeCurrentView:[AboutViewController compositeViewDescription] push:TRUE];
}

@end


#pragma mark - UINavigationBarEx Class

@interface UINavigationBarEx: UINavigationBar {
    
}
@end

@implementation UINavigationBarEx


#pragma mark - Lifecycle Functions

- (void)initUINavigationBarEx {
    [self setTintColor:[LINPHONE_MAIN_COLOR adjustHue:5.0f/180.0f saturation:0.0f brightness:0.0f alpha:0.0f]];
}

- (id)init {
    self = [super init];
    if (self) {
        [self initUINavigationBarEx];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self) {
        [self initUINavigationBarEx];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        [self initUINavigationBarEx];
    }
    return self;
}

- (void)drawRect:(CGRect)rect {
    UIImage *img = [UIImage imageNamed:@"toolsbar_background.png"];
    [img drawInRect:rect];
}

@end


#pragma mark - UINavigationControllerEx Class

@interface UINavigationControllerEx : UINavigationController

@end

@implementation UINavigationControllerEx

- (id)initWithRootViewController:(UIViewController *)rootViewController {
    [UINavigationControllerEx removeBackground:rootViewController.view];
    return [self initWithRootViewController:rootViewController];
}

+ (void)removeBackground:(UIView*)view {
    [view setBackgroundColor:[UIColor clearColor]];
}

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated {
    [UINavigationControllerEx removeBackground:viewController.view];
    UIViewController *oldTopViewController = self.topViewController;
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [oldTopViewController viewWillDisappear:animated];
    }
    [viewController viewWillAppear:animated]; // Force view
    UILabel *labelTitleView = [[UILabel alloc] init];
    labelTitleView.backgroundColor = [UIColor clearColor];
    labelTitleView.textColor = [UIColor colorWithRed:0x41/255.0f green:0x48/255.0f blue:0x4f/255.0f alpha:1.0];
    labelTitleView.shadowColor = [UIColor colorWithWhite:1.0 alpha:0.5];
    labelTitleView.font = [UIFont boldSystemFontOfSize:20];
    labelTitleView.shadowOffset = CGSizeMake(0,1);
    labelTitleView.textAlignment = UITextAlignmentCenter;
    labelTitleView.text = viewController.title;
    [labelTitleView sizeToFit];
    viewController.navigationItem.titleView = labelTitleView;
    [super pushViewController:viewController animated:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [self.topViewController viewDidAppear:animated];
        [oldTopViewController viewDidDisappear:animated];
    }
}

- (UIViewController *)popViewControllerAnimated:(BOOL)animated {
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [self.topViewController viewWillDisappear:animated];
        UIViewController *nextView = nil;
        int count = [self.viewControllers count];
        if(count > 1) {
            nextView = [self.viewControllers objectAtIndex:count - 2];
        }
        [nextView viewWillAppear:animated];
    }
    UIViewController * ret = [super popViewControllerAnimated:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [ret viewDidDisappear:animated];
        [self.topViewController viewDidAppear:animated];
    }
    return ret;
}

- (void)setViewControllers:(NSArray *)viewControllers {
    for(UIViewController *controller in viewControllers) {
        [UINavigationControllerEx removeBackground:controller.view];
    }
    [super setViewControllers:viewControllers];
}

- (void)setViewControllers:(NSArray *)viewControllers animated:(BOOL)animated {
    for(UIViewController *controller in viewControllers) {
        [UINavigationControllerEx removeBackground:controller.view];
    }
    [super setViewControllers:viewControllers animated:animated];
}

@end


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
    [settingsStore release];
	[settingsController release];
    [navigationController release];
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
                                                          landscapeMode:[LinphoneManager runningOnIpad]
                                                           portraitMode:true];
    }
    return compositeDescription;
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
	settingsStore = [[LinphoneCoreSettingsStore alloc] init];
	
    settingsController.showDoneButton = FALSE;
    settingsController.delegate = self;
    settingsController.showCreditsFooter = FALSE;
    settingsController.settingsStore = settingsStore;
    
    [navigationController.view setBackgroundColor:[UIColor clearColor]];
    
    navigationController.view.frame = self.view.frame;
    [navigationController pushViewController:settingsController animated:FALSE];
    [self.view addSubview: navigationController.view];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    [settingsController dismiss:self];
    // Set observer
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                 name:kIASKAppSettingChanged 
                                               object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    
    [settingsStore transformLinphoneCoreToKeys]; // Sync settings with linphone core settings
    settingsController.hiddenKeys = [self findHiddenKeys];
    [settingsController.tableView reloadData];	
    
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(appSettingChanged:) 
                                                 name:kIASKAppSettingChanged
                                               object:nil];
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
    } else if ([@"random_port_preference" compare: notif.object] == NSOrderedSame) {
        BOOL enable = [[notif.userInfo objectForKey:@"random_port_preference"] boolValue];
        NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[settingsController hiddenKeys]];
        if(enable) {
            [hiddenKeys addObject:@"port_preference"];
        } else {
            [hiddenKeys removeObject:@"port_preference"];
        }
        [settingsController setHiddenKeys:hiddenKeys animated:TRUE];
    } else if ([@"backgroundmode_preference" compare: notif.object] == NSOrderedSame) {
        BOOL enable = [[notif.userInfo objectForKey:@"backgroundmode_preference"] boolValue];
        NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[settingsController hiddenKeys]];
        if(!enable) {
            [hiddenKeys addObject:@"start_at_boot_preference"];
        } else {
            [hiddenKeys removeObject:@"start_at_boot_preference"];
        }
        [settingsController setHiddenKeys:hiddenKeys animated:TRUE];
    } else if ([@"stun_preference" compare: notif.object] == NSOrderedSame) {
        NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[settingsController hiddenKeys]];
        NSString *stun_server = [notif.userInfo objectForKey:@"stun_preference"];
        if (stun_server && ([stun_server length] > 0)) {
            [hiddenKeys removeObject:@"ice_preference"];
        } else {
            [hiddenKeys addObject:@"ice_preference"];
        }
        [settingsController setHiddenKeys:hiddenKeys animated:TRUE];
    } else if ([@"debugenable_preference" compare: notif.object] == NSOrderedSame) {
        NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[settingsController hiddenKeys]];
        BOOL debugEnable = [[notif.userInfo objectForKey:@"debugenable_preference"] boolValue];
        if (debugEnable) {
            [hiddenKeys removeObject:@"console_button"];
        } else {
            [hiddenKeys addObject:@"console_button"];
        }
        [settingsController setHiddenKeys:hiddenKeys animated:TRUE];
    }
}


#pragma mark - 

+ (IASKSpecifier*)filterSpecifier:(IASKSpecifier *)specifier {
#ifndef HAVE_SSL
    if ([[specifier key] isEqualToString:@"transport_preference"]) {
        NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[specifier specifierDict]];
        NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
        [titles removeObject:@"TLS"];
        [dict setObject:titles forKey:@"Titles"];
        NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
        [values removeObject:@"tls"];
        [dict setObject:values forKey:@"Values"];
        return [[[IASKSpecifier alloc] initWithSpecifier:dict] autorelease];
    }
#else
    if([LinphoneManager isLcReady]) {
        if ([[specifier key] isEqualToString:@"media_encryption_preference"]) {
            NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[specifier specifierDict]];
            if(!linphone_core_media_encryption_supported([LinphoneManager getLc], LinphoneMediaEncryptionZRTP)) {
                NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
                [titles removeObject:@"ZRTP"];
                [dict setObject:titles forKey:@"Titles"];
                NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
                [values removeObject:@"ZRTP"];
                [dict setObject:values forKey:@"Values"];
            }
            if(!linphone_core_media_encryption_supported([LinphoneManager getLc], LinphoneMediaEncryptionSRTP)) {
                NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
                [titles removeObject:@"SRTP"];
                [dict setObject:titles forKey:@"Titles"];
                NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
                [values removeObject:@"SRTP"];
                [dict setObject:values forKey:@"Values"];
            }
            return [[[IASKSpecifier alloc] initWithSpecifier:dict] autorelease];
        }
    }
#endif //HAVE_SSL
    return specifier;
}

- (NSSet*)findHiddenKeys {
    if(![LinphoneManager isLcReady]) {
        [LinphoneLogger log:LinphoneLoggerWarning format:@"Can't filter settings: Linphone core not read"];
    }
    NSMutableSet *hiddenKeys = [NSMutableSet set];
    
#ifndef DEBUG
    [hiddenKeys addObject:@"release_button"];
    [hiddenKeys addObject:@"clear_cache_button"];
    [hiddenKeys addObject:@"battery_alert_button"];
#endif
    
    [hiddenKeys addObject:@"playback_gain_preference"];
    [hiddenKeys addObject:@"microphone_gain_preference"];
    
    [hiddenKeys addObject:@"network_limit_group"];
    [hiddenKeys addObject:@"upload_bandwidth_preference"];
    [hiddenKeys addObject:@"download_bandwidth_preference"];
    
    [hiddenKeys addObject:@"incoming_call_timeout_preference"];
    [hiddenKeys addObject:@"in_call_timeout_preference"];
    
    [hiddenKeys addObject:@"wifi_only_preference"];
    
    [hiddenKeys addObject:@"quit_button"]; // Hide for the moment
    [hiddenKeys addObject:@"about_button"]; // Hide for the moment
    
    if (!linphone_core_video_supported([LinphoneManager getLc]))
        [hiddenKeys addObject:@"video_menu"];
    
    if (![LinphoneManager isNotIphone3G])
        [hiddenKeys addObject:@"silk_24k_preference"];
    
    UIDevice* device = [UIDevice currentDevice];
    if (![device respondsToSelector:@selector(isMultitaskingSupported)] || ![device isMultitaskingSupported]) {
        [hiddenKeys addObject:@"backgroundmode_preference"];
        [hiddenKeys addObject:@"start_at_boot_preference"];
    } else {
         if(![[LinphoneManager instance] lpConfigBoolForKey:@"backgroundmode_preference"]) {
             [hiddenKeys addObject:@"start_at_boot_preference"];
         }
    }
    
    [hiddenKeys addObject:@"enable_first_login_view_preference"];
    
#ifndef VIDEO_ENABLED
    [hiddenKeys addObject:@"enable_video_preference"];
#endif //VIDEO_ENABLED
    
    if (!linphone_core_video_enabled([LinphoneManager getLc])) {
        [hiddenKeys addObject:@"video_menu"];
    }
    
    
    [hiddenKeys addObjectsFromArray:[[LinphoneManager unsupportedCodecs] allObjects]];
    
    if(lp_config_get_int(linphone_core_get_config([LinphoneManager getLc]),"sip","sip_random_port",0)==1) {
        [hiddenKeys addObject:@"port_preference"];
    }

    if(linphone_core_get_stun_server([LinphoneManager getLc]) == NULL) {
        [hiddenKeys addObject:@"ice_preference"];
    }

    if(![[LinphoneManager instance] lpConfigBoolForKey:@"debugenable_preference"]) {
        [hiddenKeys addObject:@"console_button"];
    }
    
    if(![LinphoneManager runningOnIpad]) {
        [hiddenKeys addObject:@"preview_preference"];
    }
    if([[LinphoneManager instance] lpConfigBoolForKey:@"hide_run_assistant_preference"]) {
		[hiddenKeys addObject:@"wizard_button"];
	}
	
    return hiddenKeys;
}

#pragma mark - IASKSettingsDelegate Functions

- (void)settingsViewControllerDidEnd:(IASKAppSettingsViewController *)sender {
}

- (void)settingsViewController:(IASKAppSettingsViewController*)sender buttonTappedForSpecifier:(IASKSpecifier*)specifier {
    NSString *key = [specifier.specifierDict objectForKey:kIASKKey];
#ifdef DEBUG
    if([key isEqual:@"release_button"]) {
        [[UIApplication sharedApplication].keyWindow.rootViewController  release];
        [[UIApplication sharedApplication].keyWindow setRootViewController:nil];
        [[LinphoneManager instance]	destroyLibLinphone];
        [LinphoneManager instanceRelease];
    } else  if([key isEqual:@"clear_cache_button"]) {
        [[PhoneMainView instance].mainViewController clearCache:[NSArray arrayWithObject:[[PhoneMainView  instance] currentView]]];
    } else  if([key isEqual:@"battery_alert_button"]) {
        [[UIDevice currentDevice] _setBatteryState:UIDeviceBatteryStateUnplugged];
        [[UIDevice currentDevice] _setBatteryLevel:0.01f];
        [[NSNotificationCenter defaultCenter] postNotificationName:UIDeviceBatteryLevelDidChangeNotification object:self];
    }
#endif
    if([key isEqual:@"console_button"]) {
        [[PhoneMainView instance] changeCurrentView:[ConsoleViewController compositeViewDescription] push:TRUE];
    } else if([key isEqual:@"wizard_button"]) {
        WizardViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[WizardViewController compositeViewDescription]], WizardViewController);
        if(controller != nil) {
            [controller reset];
        }
    } else if([key isEqual:@"about_button"]) {
        [[PhoneMainView instance] changeCurrentView:[AboutViewController compositeViewDescription] push:TRUE];
    }
}

@end
