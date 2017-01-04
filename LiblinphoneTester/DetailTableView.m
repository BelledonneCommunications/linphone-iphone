//
//  DetailViewController.m
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import "DetailTableView.h"
#import "MasterView.h"
#include "linphone/liblinphone_tester.h"
#import "Log.h"

static NSString *const kAllTestsName = @"Run All tests";

@implementation TestItem

- (id)initWithName:(NSString *)name fromSuite:(NSString *)suite {
	self = [super init];
	if (self) {
		self.name = name;
		self.suite = suite;
		self.state = TestStateIdle;
	}
	return self;
}

+ (TestItem *)testWithName:(NSString *)name fromSuite:(NSString *)suite {
	return [[TestItem alloc] initWithName:name fromSuite:suite];
}

- (NSString *)description {
	return [NSString stringWithFormat:@"{suite:'%@', test:'%@', state:%d}", _suite, _name, _state];
}

@end

@interface DetailTableView () {
	NSMutableArray *_tests;
	BOOL in_progress;
}
@property(strong, nonatomic) UIPopoverController *masterPopoverController;
- (void)configureView;
@end

@implementation DetailTableView

#pragma mark - Managing the detail item

- (void)setDetailItem:(id)newDetailItem {
	if (_detailItem != newDetailItem) {
		_detailItem = newDetailItem;

		// Update the view.
		[self configureView];
		[self.tableView reloadData];
	}

	if (self.masterPopoverController != nil) {
		[self.masterPopoverController dismissPopoverAnimated:YES];
	}
}

- (void)addTestsFromSuite:(NSString *)suite {
	int count = bc_tester_nb_tests([suite UTF8String]);

	for (int i = 0; i < count; i++) {
		const char *test_name = bc_tester_test_name([suite UTF8String], i);
		if (test_name) {
			NSString *testName = [NSString stringWithUTF8String:test_name];
			TestItem *item = [[TestItem alloc] initWithName:testName fromSuite:suite];
			[_tests addObject:item];
		}
	}
}

- (void)configureView {
	_tests = [[NSMutableArray alloc] initWithCapacity:0];
	if (self.detailItem == nil) {
		return;
	}

	if ([self.detailItem isEqualToString:@"All"]) {
		// dont sort tests if we use all suites at once
		for (int i = 0; i < bc_tester_nb_suites(); i++) {
			const char *suite = bc_tester_suite_name(i);
			if (suite) {
				[self addTestsFromSuite:[NSString stringWithUTF8String:suite]];
			}
		}
	} else {
		[self addTestsFromSuite:self.detailItem];
		[_tests sortUsingComparator:^(TestItem *obj1, TestItem *obj2) {
		  return [obj1.name compare:obj2.name];
		}];
	}
	// suite name not used for this one
	[_tests insertObject:[TestItem testWithName:kAllTestsName fromSuite:self.detailItem] atIndex:0];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
	[self configureView];
}

- (void)didReceiveMemoryWarning {
	[super didReceiveMemoryWarning];
	// Dispose of any resources that can be recreated.
}

#pragma mark - Tableview

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return _tests.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"testCellIdentifier";
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId forIndexPath:indexPath];
	if (cell == nil) {
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:kCellId];
	}

	TestItem *test = _tests[indexPath.row];
	cell.textLabel.text = [NSString stringWithFormat:@"%@/%@", test.suite, test.name];
	NSString *image = nil;
	switch (test.state) {
		case TestStateIdle:
			image = nil;
			break;
		case TestStatePassed:
			image = @"test_passed";
			break;
		case TestStateInProgress:
			image = @"test_inprogress";
			break;
		case TestStateFailed:
			image = @"test_failed";
			break;
	}
	if (image) {
		image = [[NSBundle mainBundle] pathForResource:image ofType:@"png"];
		cell.imageView.image = [UIImage imageWithContentsOfFile:image];
	} else {
		[cell.imageView setImage:nil];
	}
	return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath {
	return NO;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:FALSE];
	if (indexPath.row == 0) {
		// we should run all tests
		NSMutableArray *paths = [[NSMutableArray alloc] init];
		for (int i = 1; i < _tests.count; i++) {
			[paths addObject:[NSIndexPath indexPathForRow:i inSection:0]];
		}
		[self launchTest:paths];
	} else {
		[self launchTest:@[ indexPath ]];
	}
}

- (void)updateItem:(NSArray *)paths withAnimation:(BOOL)animate {
	[self.tableView reloadRowsAtIndexPaths:paths
						  withRowAnimation:animate ? UITableViewRowAnimationFade : UITableViewRowAnimationNone];
}

- (void)launchTest:(NSArray *)paths {
	if (in_progress) {
		LOGE(@"Test already in progress");
		return;
	}

	in_progress = TRUE;
	for (NSIndexPath *index in paths) {
		TestItem *test = _tests[index.row];
		test.state = TestStateInProgress;
	}
	[self updateItem:paths withAnimation:FALSE];

	dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);

	dispatch_async(queue, ^{
	  for (NSIndexPath *index in paths) {
		  TestItem *test = _tests[index.row];
		  LOGI(@"Should launch test %@", test);
		  NSString *testSuite = test.suite;
		  if ([test.suite isEqualToString:@"All"]) {
			  testSuite = nil;
		  }
		  NSString *testName = test.name;
		  if ([test.name isEqualToString:kAllTestsName]) {
			  testName = nil;
		  }
		  BOOL fail = NO;
		  @try {
			  fail = bc_tester_run_tests([testSuite UTF8String], [testName UTF8String], NULL);
		  } @catch (NSException *e) {
			  fail = YES;
		  }
		  if (fail) {
			  LOGW(@"Test Failed!");
			  test.state = TestStateFailed;
		  } else {
			  LOGI(@"Test Passed!");
			  test.state = TestStatePassed;
		  }
		  [self.tableView scrollToRowAtIndexPath:index atScrollPosition:UITableViewScrollPositionMiddle animated:YES];
		  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
			[self updateItem:paths withAnimation:TRUE];
		  });
	  }
	  in_progress = FALSE;
	});
}

#pragma mark - Split view

- (void)splitViewController:(UISplitViewController *)splitController
	 willHideViewController:(UIViewController *)viewController
		  withBarButtonItem:(UIBarButtonItem *)barButtonItem
	   forPopoverController:(UIPopoverController *)popoverController {
	barButtonItem.title = NSLocalizedString(@"Master", @"Master");
	[self.navigationItem setLeftBarButtonItem:barButtonItem animated:YES];
	self.masterPopoverController = popoverController;
}

- (void)splitViewController:(UISplitViewController *)splitController
	 willShowViewController:(UIViewController *)viewController
  invalidatingBarButtonItem:(UIBarButtonItem *)barButtonItem {
	// Called when the view is shown again in the split view, invalidating the button and popover controller.
	[self.navigationItem setLeftBarButtonItem:nil animated:YES];
	self.masterPopoverController = nil;
}

@end
