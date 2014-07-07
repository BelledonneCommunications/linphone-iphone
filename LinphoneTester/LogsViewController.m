//
//  LogsViewController.m
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 01/06/2014.
//
//

#import "LogsViewController.h"
#import "MasterViewController.h"

@interface LogsViewController () {
    NSString* txt;
}

@end

@implementation LogsViewController



- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void)viewDidAppear:(BOOL)animated {
    self.tview.textContainer.lineBreakMode = NSLineBreakByClipping;
    self.tview.text = [lastLogs componentsJoinedByString:@"\n"];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(updateLogs:)
                                                 name:kLogsUpdateNotification
                                               object:nil];
}

- (void)viewDidDisappear:(BOOL)animated {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (IBAction)clearLogs:(id)sender {
    
    self.tview.text = nil;
}

- (void)updateLogs:(NSNotification*)notif {
    NSArray* newLogs = [notif.userInfo objectForKey:@"newlogs"];
    dispatch_async(dispatch_get_main_queue(), ^{
        self.tview.text = [self.tview.text stringByAppendingString:[newLogs componentsJoinedByString:@"\n"] ];
    });
}

@end
