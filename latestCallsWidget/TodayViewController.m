//
//  TodayViewController.m
//  latestCallsWidget
//
//  Created by David Idmansour on 06/06/2018.
//

#import "TodayViewController.h"
#import <NotificationCenter/NotificationCenter.h>

@interface TodayViewController () <NCWidgetProviding>

@end

@implementation TodayViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)widgetPerformUpdateWithCompletionHandler:(void (^)(NCUpdateResult))completionHandler {
    // Perform any setup necessary in order to update the view.
    
    // If an error is encountered, use NCUpdateResultFailed
    // If there's no update required, use NCUpdateResultNoData
    // If there's an update, use NCUpdateResultNewData

    completionHandler(NCUpdateResultNewData);
}

- (void)launchAppWithURL:(NSURL *) url {
    [self.extensionContext openURL:url
                 completionHandler:nil];
}

- (void)launchOnHistoryDetailsWithId:(NSString *)logId {
    [self launchAppWithURL:[NSURL URLWithString:[@"sip://call_log/show?" stringByAppendingString:logId]]];
}

- (IBAction)firstButtonTapped {
    [self launchOnHistoryDetailsWithId:@""];
}

- (IBAction)secondButtonTapped {
    [self launchOnHistoryDetailsWithId:@""];
}

- (IBAction)thirdButtonTapped {
    [self launchOnHistoryDetailsWithId:@""];
}

- (IBAction)fourthButtonTapped {
    [self launchOnHistoryDetailsWithId:@""];
}
@end
