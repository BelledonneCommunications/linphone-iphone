/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "RecordingsListView.h"
#import "PhoneMainView.h"

@implementation RecordingSelection

static RecordingSelectionMode sSelectionMode = RecordingSelectionModeNone;

+ (void)setSelectionMode:(RecordingSelectionMode)selectionMode {
    sSelectionMode = selectionMode;
}

+ (RecordingSelectionMode)getSelectionMode {
    return sSelectionMode;
}

@end

@implementation RecordingsListView

@synthesize tableController;
@synthesize topBar;

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
    if (compositeDescription == nil) {
        compositeDescription = [[UICompositeViewDescription alloc] init:self.class
                                                              statusBar:StatusBarView.class
                                                                 tabBar:TabBarView.class
                                                               sideMenu:SideMenuView.class
                                                             fullscreen:false
                                                         isLeftFragment:YES
                                                           fragmentWith:ContactDetailsView.class];
    }
    return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
    return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    tableController.tableView.accessibilityIdentifier = @"Recordings table";
    tableController.tableView.tableFooterView = [[UIView alloc] init];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if (tableController.isEditing) {
        tableController.editing = NO;
    }
	[_toggleSelectionButton setImage:[UIImage imageNamed:@"select_all_default.png"] forState:UIControlStateSelected];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
}

- (void) viewWillDisappear:(BOOL)animated {
    self.view = NULL;
    [self.tableController removeAllRecordings];
}

#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)sender {
    NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete selected recordings?", nil)];
    [LinphoneManager.instance setContactsUpdated:TRUE];
    [UIConfirmationDialog ShowWithMessage:msg
                            cancelMessage:nil
                           confirmMessage:nil
                            onCancelClick:^() {
                                [self onEditionChangeClick:nil];
                            }
                      onConfirmationClick:^() {
                          [tableController removeSelectionUsing:nil];
                          [tableController loadData];
                          [self onEditionChangeClick:nil];
                      }];
}

- (IBAction)onEditionChangeClick:(id)sender {
    _backButton.hidden = self.tableController.isEditing;
}

- (IBAction)onBackPressed:(id)sender {
    [PhoneMainView.instance popCurrentView];
}

@end
