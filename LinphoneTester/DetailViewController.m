//
//  DetailViewController.m
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import "DetailViewController.h"
#include "linphone/liblinphone_tester.h"

@interface DetailViewController () {
    NSMutableArray* _tests;
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
    }

    if (self.masterPopoverController != nil) {
        [self.masterPopoverController dismissPopoverAnimated:YES];
    }        
}

- (void)configureView
{
    const char* suite = [self.detailItem UTF8String];
    int count = liblinphone_tester_nb_tests(suite);
    _tests = [[NSMutableArray alloc] initWithCapacity:count];
    for (int i=0; i<count; i++) {
        const char* test_name = liblinphone_tester_test_name(suite, i);
        [_tests addObject:[NSString stringWithUTF8String:test_name]];
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

    NSString *test = _tests[indexPath.row];
    cell.textLabel.text = test;
    return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return NO;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSString *object = _tests[indexPath.row];
    NSLog(@"Should launch test %@ from suite %@", object, self.detailItem);
    BOOL fail = liblinphone_tester_run_tests([self.detailItem UTF8String], [object UTF8String]);
    if(fail){
        UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"Test failed" message:@"" delegate:nil cancelButtonTitle:@"OK :-(" otherButtonTitles: nil];
        [alert show];
    }
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
