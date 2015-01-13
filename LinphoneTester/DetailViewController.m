//
//  DetailViewController.m
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import "DetailViewController.h"
#import "MasterViewController.h"
#import "LogsViewController.h"
#include "linphone/liblinphone_tester.h"

static NSString* const kAllTestsName = @"Run All tests";

@implementation TestItem

-(id)initWithName:(NSString *)name fromSuite:(NSString *)suite {
    self = [super init];
    if( self ){
        self.name = name;
        self.suite = suite;
        self.state = TestStateIdle;
    }
    return self;
}

+(TestItem *)testWithName:(NSString *)name fromSuite:(NSString *)suite{
    return [[TestItem alloc] initWithName:name fromSuite:suite];
}

-(NSString*)description {
    return [NSString stringWithFormat:@"{suite:'%@', test:'%@', state:%d}", _suite, _name, _state];
}

@end

@interface DetailViewController () {
    NSMutableArray* _tests;
    BOOL in_progress;
}
@property (strong, nonatomic) UIPopoverController *masterPopoverController;
- (void)configureView;
@end

@implementation DetailViewController

#pragma mark - Managing the detail item

- (void)setDetailItem:(id)newDetailItem
{
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

- (void)configureView
{
    const char* suite = [self.detailItem UTF8String];
    if( suite == NULL ) return;
    NSString* nssuite = [NSString stringWithUTF8String:suite];
    int count = liblinphone_tester_nb_tests(suite);
    _tests = [[NSMutableArray alloc] initWithCapacity:count];
    
    [_tests addObject:[TestItem testWithName:kAllTestsName fromSuite:nssuite]];
    
    for (int i=0; i<count; i++) {
        const char* test_name = liblinphone_tester_test_name(suite, i);
        TestItem* item = [[TestItem alloc] initWithName:[NSString stringWithUTF8String:test_name] fromSuite:nssuite];
        [_tests addObject:item];
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    [self configureView];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Tableview

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _tests.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"testCellIdentifier" forIndexPath:indexPath];

    TestItem *test = _tests[indexPath.row];
    cell.textLabel.text = test.name;
    NSString* image = nil;
    switch( test.state ){
        case TestStateIdle: image = nil; break;
        case TestStatePassed: image = @"test_passed"; break;
        case TestStateInProgress: image = @"test_inprogress"; break;
        case TestStateFailed: image = @"test_failed"; break;
    }
    if(image){
        image = [[NSBundle mainBundle] pathForResource:image ofType:@"png"];
        cell.imageView.image = [UIImage imageWithContentsOfFile:image];
	} else {
		[cell.imageView setImage:nil];
	}
    return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return NO;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:FALSE];
    if( !in_progress ){
        if( indexPath.row == 0 ){
          
            // run all tests
            NSMutableArray* paths = [[NSMutableArray alloc] init];
            for (int i = 1; i<_tests.count; i++) {
                [paths addObject:[NSIndexPath indexPathForRow:i inSection:0]];
            }
            [self launchTest:paths];
            
        } else {
            [self launchTest:@[indexPath]];
        }
    }
}


-(void)updateItem:(NSArray*)paths withAnimation:(BOOL)animate {
    [self.tableView reloadRowsAtIndexPaths:paths
                          withRowAnimation:animate?UITableViewRowAnimationFade:UITableViewRowAnimationNone];
}


-(void)launchTest:(NSArray*)paths {
    for (NSIndexPath* path in paths) {
        TestItem *test = _tests[path.row];
        test.state = TestStateInProgress;
    }
    [self updateItem:paths withAnimation:FALSE];
    
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    in_progress = TRUE;
    
    dispatch_async(queue, ^{
        for (NSIndexPath*path in paths) {
            
            TestItem *test = _tests[path.row];
            LSLog(@"Should launch test %@", test);
            
            BOOL fail = liblinphone_tester_run_tests([test.suite UTF8String], [test.name UTF8String]);
            if(fail){
                LSLog(@"Test Failed!");
                test.state = TestStateFailed;
            } else {
                LSLog(@"Test Passed!");
                test.state = TestStatePassed;
            }
            
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                [self updateItem:@[path] withAnimation:TRUE];
                in_progress = FALSE;
            });
        }
    });
}


#pragma mark - Split view

- (void)splitViewController:(UISplitViewController *)splitController willHideViewController:(UIViewController *)viewController withBarButtonItem:(UIBarButtonItem *)barButtonItem forPopoverController:(UIPopoverController *)popoverController
{
    barButtonItem.title = NSLocalizedString(@"Master", @"Master");
    [self.navigationItem setLeftBarButtonItem:barButtonItem animated:YES];
    self.masterPopoverController = popoverController;
}

- (void)splitViewController:(UISplitViewController *)splitController willShowViewController:(UIViewController *)viewController invalidatingBarButtonItem:(UIBarButtonItem *)barButtonItem
{
    // Called when the view is shown again in the split view, invalidating the button and popover controller.
    [self.navigationItem setLeftBarButtonItem:nil animated:YES];
    self.masterPopoverController = nil;
}

@end
