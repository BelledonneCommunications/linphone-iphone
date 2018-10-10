//
//  RecordingsListView.m
//  linphone
//
//  Created by benjamin_verdier on 25/07/2018.
//

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
