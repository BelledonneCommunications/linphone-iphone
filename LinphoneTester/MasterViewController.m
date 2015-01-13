//
//  MasterViewController.m
//  LinphoneTester
//
//  Created by guillaume on 28/05/2014.
//
//

#import "MasterViewController.h"
#import "LogsViewController.h"
#import "DetailViewController.h"

#include "linphone/liblinphone_tester.h"

@interface MasterViewController () {
    NSMutableArray *_objects;
    NSString* bundlePath;
    NSString* documentPath;
}
@end

@implementation MasterViewController

- (void)awakeFromNib
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        self.clearsSelectionOnViewWillAppear = NO;
        self.preferredContentSize = CGSizeMake(320.0, 600.0);
    }
    [super awakeFromNib];
}

NSMutableArray* lastLogs = nil;
NSMutableArray* logsBuffer = nil;
static int const kLastLogsCapacity = 5000;
static int const kLogsBufferCapacity = 10;
NSString* const  kLogsUpdateNotification = @"kLogsUpdateNotification";

void LSLog(NSString* fmt, ...){
    va_list args;
    va_start(args, fmt);
    linphone_log_function(ORTP_MESSAGE, [fmt UTF8String], args);
    va_end(args);
}

static void linphone_log_function(OrtpLogLevel lev, const char *fmt, va_list args) {
    NSString* log = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:fmt] arguments:args];
    NSLog(@"%@",log);
}

- (void)setupLogging {
    lastLogs = [[NSMutableArray alloc] initWithCapacity:kLastLogsCapacity];
    logsBuffer = [NSMutableArray arrayWithCapacity:kLogsBufferCapacity];

    linphone_core_set_log_handler(linphone_log_function);
    linphone_core_set_log_level(ORTP_DEBUG|ORTP_ERROR|ORTP_FATAL|ORTP_WARNING|ORTP_MESSAGE|ORTP_TRACE);
    
}


- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
    self.detailViewController = (DetailViewController *)[[self.splitViewController.viewControllers lastObject] topViewController];
    
    [self setupLogging];
    
    bundlePath = [[NSBundle mainBundle] bundlePath];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    documentPath = [paths objectAtIndex:0];
    liblinphone_tester_init();
    liblinphone_tester_set_fileprefix([bundlePath UTF8String]);
    liblinphone_tester_set_writable_dir_prefix( ms_strdup([documentPath UTF8String]) );

    LSLog(@"Bundle path: %@", bundlePath);
    LSLog(@"Document path: %@", documentPath);

    int count = liblinphone_tester_nb_test_suites();
    _objects = [[NSMutableArray alloc] initWithCapacity:count];
    for (int i=0; i<count; i++) {
        const char* suite = liblinphone_tester_test_suite_name(i);
        [_objects addObject:[NSString stringWithUTF8String:suite]];
    }

}

- (void)displayLogs {
    LSLog(@"Should display logs");
    [self.navigationController performSegueWithIdentifier:@"viewLogs" sender:self];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark - Table View

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _objects.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"Cell" forIndexPath:indexPath];

    NSString *suite = _objects[indexPath.row];
    cell.textLabel.text = suite;
    return cell;
}

- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return NO;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        [_objects removeObjectAtIndex:indexPath.row];
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view.
    }
}

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
        NSString *object = _objects[indexPath.row];
        self.detailViewController.detailItem = object;
    }
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"showDetail"]) {
        NSIndexPath *indexPath = [self.tableView indexPathForSelectedRow];
        NSString *object = _objects[indexPath.row];
        [[segue destinationViewController] setDetailItem:object];
    }
}


@end
