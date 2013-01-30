/* BuschJaegerWelcomeView.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import "BuschJaegerWelcomeView.h"
#import "BuschJaegerMainView.h"

@implementation BuschJaegerWelcomeView

@synthesize settingsButton;
@synthesize stationTableController;
@synthesize historyTableController;
@synthesize waitView;
@synthesize historyLeftSwipeGestureRecognizer;

#pragma mark - Lifecycle Functions

- (void)dealloc {
    [settingsButton release];
    [stationTableController release];
    [historyTableController release];
    [historyLeftSwipeGestureRecognizer release];
    
    // Remove all observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [super dealloc];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Swipe history gesture for iphone devices
    if(![LinphoneManager runningOnIpad]) {
        historyLeftSwipeGestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(doHistorySwipe:)];
        [historyLeftSwipeGestureRecognizer setDirection:UISwipeGestureRecognizerDirectionLeft];
        [historyLeftSwipeGestureRecognizer setDelegate:self];
        [self.view addGestureRecognizer:historyLeftSwipeGestureRecognizer];
    }
    
    [historyTableController.view setBackgroundColor:[UIColor clearColor]];
    [stationTableController.view setBackgroundColor:[UIColor clearColor]];
    
    [waitView setHidden:TRUE];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    // Set observer
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(configurationUpdateEvent:)
                                                 name:kLinphoneConfigurationUpdate
                                               object:nil];
    [self updateConfiguration:[LinphoneManager instance].configuration];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(applicationWillEnterForeground:)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
    // Wait a bit for the gateway update
    [self performSelector:@selector(reloadHistory) withObject:self afterDelay:2.0];
}

- (void)viewWillDisappear:(BOOL)animated{
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:UIApplicationWillEnterForegroundNotification
                                                  object:nil];
    
    // Remove observer
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:kLinphoneConfigurationUpdate
                                                  object:nil];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation {
    if ([LinphoneManager runningOnIpad]) {
        return YES;
    } else {
        return interfaceOrientation == UIInterfaceOrientationPortrait;
    }
}

- (NSUInteger)supportedInterfaceOrientations {
    if ([LinphoneManager runningOnIpad]) {
        return UIInterfaceOrientationMaskAll;
    } else {
        return UIInterfaceOrientationMaskPortrait;
    }
}


#pragma mark - Event Functions

- (void)applicationWillEnterForeground:(NSNotification*)notif {
    [self reloadHistory];
}

- (void)configurationUpdateEvent: (NSNotification*) notif {
    BuschJaegerConfiguration *configuration = [notif.userInfo objectForKey:@"configuration"];
    [self updateConfiguration:configuration];
}

- (void)updateConfiguration:(BuschJaegerConfiguration *)configuration {
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"ID" ascending:YES];
    NSArray *sortDescriptors = [NSArray arrayWithObjects:sortDescriptor, nil];
    [stationTableController setStations:[configuration.outdoorStations sortedArrayUsingDescriptors:sortDescriptors]];
}


#pragma mark -

- (void)reloadHistory {
    [self view]; // Force view load
    if([LinphoneManager runningOnIpad] && [LinphoneManager instance].configuration.valid) {
        if([[LinphoneManager instance].configuration loadHistory:self]) {
            [waitView setHidden:FALSE];
        } else {
            [waitView setHidden:TRUE];
        }
    }
}

- (void)update {
    NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"date" ascending:NO];
    NSArray *sortDescriptors = [NSArray arrayWithObjects:sortDescriptor, nil];
    
    [historyTableController setHistory:[[LinphoneManager instance].configuration.history sortedArrayUsingDescriptors:sortDescriptors]];
}

- (IBAction)settingsClick:(id)sender {
    [[BuschJaegerMainView instance].navigationController  pushViewController:[BuschJaegerMainView instance].settingsView animated:FALSE];
}


#pragma mark - Actions Functions

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

- (IBAction)doHistorySwipe:(UISwipeGestureRecognizer *)sender {
    if([[LinphoneManager instance].configuration valid]) {
        [[BuschJaegerMainView instance].historyView reload];
        [[BuschJaegerMainView instance].navigationController  pushViewController:[BuschJaegerMainView instance].historyView animated:FALSE];
    }
}


#pragma mark - BuschJaegerConfigurationDelegate Functions

- (void)buschJaegerConfigurationSuccess {
    [[UIApplication sharedApplication] setApplicationIconBadgeNumber:0];
    [waitView setHidden:TRUE];
    [self update];
}

- (void)buschJaegerConfigurationError:(NSString *)error {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"History provisioning error",nil)
                                                        message:[NSString stringWithFormat:NSLocalizedString(@"Connection issue: %@", nil), error]
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    [waitView setHidden:TRUE];
    [self update];
}


@end
