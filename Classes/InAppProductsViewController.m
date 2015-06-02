//
//  InAppProductsViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 15/04/15.
//
//

#import "InAppProductsViewController.h"
#import "InAppProductsCell.h"

@implementation InAppProductsViewController

#pragma mark - Lifecycle Functions

- (id)init {
	return [super initWithNibName:@"InAppProductsViewController" bundle:[NSBundle mainBundle]];
}


- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];


}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	for (NSString* notification in [NSArray arrayWithObjects:kIAPReady, kIAPRestoreSucceeded, kIAPPurchaseSucceeded, kIAPReceiptSucceeded, kIAPPurchaseTrying, nil]) {
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(onIAPPurchaseNotification:)
													 name:notification
												   object:nil];
	}
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	for (NSString* notification in [NSArray arrayWithObjects:kIAPReady, kIAPRestoreSucceeded, kIAPPurchaseSucceeded, kIAPReceiptSucceeded, kIAPPurchaseTrying, nil]) {
		[[NSNotificationCenter defaultCenter] removeObserver:self
														name:notification
													  object:nil];
	}
}

- (void)onIAPPurchaseNotification:(NSNotification*)notif {
	InAppProductsManager *iapm = [[LinphoneManager instance] iapManager];
	[[_tableController tableView] reloadData];
	[_waitView setHidden:([[iapm productsAvailable] count] != 0 && ![notif.name isEqualToString:kIAPPurchaseTrying])];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if(compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:@"InAppProducts"
																content:@"InAppProductsViewController"
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

- (IBAction)onRestoreClicked:(UIButton *)sender {
	[[[LinphoneManager instance] iapManager] restore];
}
@end