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

- (IBAction)firstButtonTapped {
    [self launchAppWithURL:[NSURL URLWithString:@"sip://"]];
}

- (IBAction)secondButtonTapped {
    [self launchAppWithURL:[NSURL URLWithString:@"sip://"]];
}

- (IBAction)thirdButtonTapped {
    [self launchAppWithURL:[NSURL URLWithString:@"sip://"]];
}

- (IBAction)fourthButtonTapped {
    [self launchAppWithURL:[NSURL URLWithString:@"sip://"]];
}
@end
