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

	[_tableController release];

	[super dealloc];
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
//	[[NSNotificationCenter defaultCenter] addObserver:self
//											 selector:@selector(textReceivedEvent:)
//												 name:kLinphoneTextReceived
//											   object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

//	[[NSNotificationCenter defaultCenter] removeObserver:self
//													name:kLinphoneTextReceived
//												  object:nil];
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

@end