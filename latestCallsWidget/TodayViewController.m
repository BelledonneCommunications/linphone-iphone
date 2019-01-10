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

- (void)loadData {
    NSUserDefaults *mySharedDefaults = [[NSUserDefaults alloc] initWithSuiteName: @"group.belledonne-communications.linphone.widget"];
    [_imgs removeAllObjects];
    [_logIds removeAllObjects];
    [_displayNames removeAllObjects];
    NSMutableArray *logs = [NSMutableArray arrayWithArray:[mySharedDefaults objectForKey:@"logs"]];
    for (NSDictionary *dict in logs) {
        [_logIds addObject:[dict objectForKey:@"id"]];
        [_displayNames addObject:[dict objectForKey:@"display"]];
        [_imgs addObject:[dict objectForKey:@"img"]?:[NSNull null]];
    }
}

- (void)draw {
    int i = 0;
    for (i = 0 ; i < 4 && i < _logIds.count ; i++) {
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[0];
        UILabel *name = stack.subviews[1];
        if (_imgs[i] != [NSNull null]) {
            [button setImage:[UIImage imageWithData:_imgs[i]] forState:UIControlStateNormal];
        }
        [stack setAlpha:1];
        button.enabled = YES;
        [name setText:_displayNames[i]];
    }
    
    while (i < 4) {
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[0];
        [stack setAlpha:0];
        button.enabled = NO;
        i++;
    }
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view from its nib.
    for (UIStackView *stack in _stackViews) {
        UIButton *button = stack.subviews[0];
        // making rounded image
        UIImageView *imageView = button.imageView;
        imageView.layer.cornerRadius = imageView.frame.size.height / 2;
        imageView.clipsToBounds = YES;
    }
    _logIds = [NSMutableArray array];
    _imgs = [NSMutableArray array];
    _displayNames = [NSMutableArray array];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
    [_imgs removeAllObjects];
}

- (void)widgetPerformUpdateWithCompletionHandler:(void (^)(NCUpdateResult))completionHandler {
    // Perform any setup necessary in order to update the view.
    
    // If an error is encountered, use NCUpdateResultFailed
    // If there's no update required, use NCUpdateResultNoData
    // If there's an update, use NCUpdateResultNewData
    [self loadData];
    [self draw];
    completionHandler(NCUpdateResultNewData);
}

- (void)launchAppWithURL:(NSURL *) url {
    [self.extensionContext openURL:url
                 completionHandler:nil];
}

- (void)launchOnHistoryDetailsWithId:(NSString *)logId {
    [self launchAppWithURL:[NSURL URLWithString:[@"linphone-widget://call_log/show?" stringByAppendingString:logId]]];
}

- (IBAction)firstButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[0]];
}

- (IBAction)secondButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[1]];
}

- (IBAction)thirdButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[2]];
}

- (IBAction)fourthButtonTapped {
    [self launchOnHistoryDetailsWithId:_logIds[3]];
}
@end
