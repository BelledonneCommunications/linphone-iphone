//
//  TodayViewController.m
//  latestCallsWidget
//
//  Created by David Idmansour on 06/06/2018.
//

#import "TodayViewController.h"
#import <NotificationCenter/NotificationCenter.h>
#import "linphone/linphonecore.h"
#ifdef __IPHONE_9_0
#import <Contacts/Contacts.h>
#endif

@interface TodayViewController () <NCWidgetProviding>

@end

@implementation TodayViewController

- (void)loadData {
    NSDateFormatter *formatter = [[NSDateFormatter alloc] init];
    [formatter setDateFormat:@"yyyy-MM-dd HH:mm:ss"];
    
    NSUserDefaults *mySharedDefaults = [[NSUserDefaults alloc] initWithSuiteName: @"group.belledonne-communications.linphone.widget"];
    NSMutableArray *dates = [NSMutableArray array];
    [_imgs removeAllObjects];
    _imgs = nil;
    _imgs = [NSMutableDictionary dictionaryWithDictionary:[mySharedDefaults objectForKey:@"imageData"]];
    NSDictionary *logsTmp = [mySharedDefaults objectForKey:@"logs"];
    [_logs removeAllObjects];
    for (NSString *dateStr in logsTmp.allKeys) {
        NSDictionary *log = [logsTmp objectForKey:dateStr];
        NSDate *date = [formatter dateFromString:dateStr];
        [dates addObject:date];
        [_logs setObject:log forKey:date];
    }
    [_sortedDates removeAllObjects];
    _sortedDates = nil;
    _sortedDates = [[NSMutableArray alloc]
                    initWithArray:[dates sortedArrayUsingComparator:^NSComparisonResult(NSDate *d1, NSDate *d2) {
        return [d2 compare:d1]; // reverse order
    }]];
}

- (UIImage *)resizeImage:(UIImage *)image
{
    float actualHeight = image.size.height;
    float actualWidth = image.size.width;
    float maxHeight = 200.0;
    float maxWidth = 200.0;
    float imgRatio = actualWidth/actualHeight;
    float maxRatio = maxWidth/maxHeight;
    float compressionQuality = 1;
    if (actualHeight > maxHeight || actualWidth > maxWidth)
    {
        if(imgRatio < maxRatio) {
            imgRatio = maxHeight / actualHeight;
            actualWidth = imgRatio * actualWidth;
            actualHeight = maxHeight;
        } else if(imgRatio > maxRatio) {
            imgRatio = maxWidth / actualWidth;
            actualHeight = imgRatio * actualHeight;
            actualWidth = maxWidth;
        } else {
            actualHeight = maxHeight;
            actualWidth = maxWidth;
        }
    }
    CGRect rect = CGRectMake(0.0, 0.0, actualWidth, actualHeight);
    UIGraphicsBeginImageContext(rect.size);
    [image drawInRect:rect];
    UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
    NSData *imageData = UIImageJPEGRepresentation(img, compressionQuality);
    UIGraphicsEndImageContext();
    return [UIImage imageWithData:imageData];
}

- (void)draw {
    [_contactsToDisplay removeAllObjects];
    [_logIds removeAllObjects];
    int i = 0, j = 0;
    while (i < _stackViews.count && j < _sortedDates.count) {
        NSDate *date = _sortedDates[j++];
        NSString *address = [[_logs objectForKey:date] objectForKey:@"address"];
        LinphoneAddress *adr = linphone_address_new([address UTF8String]);
        NSString *logId = [[_logs objectForKey:date] objectForKey:@"id"];
        address = [[NSString stringWithUTF8String:linphone_address_as_string_uri_only(adr)] substringFromIndex:4];
        if ([_contactsToDisplay containsObject:address])
            continue;
        [_contactsToDisplay addObject:address];
        [_logIds addObject:logId];
        NSString *displayName = [NSString stringWithUTF8String:(linphone_address_get_display_name(adr))?:linphone_address_get_username(adr)];
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[0];
        UILabel *name = stack.subviews[1];
        if ([self.imgs.allKeys containsObject:address]) {
            NSData *imgData = [_imgs objectForKey:address];
//            UIImage *image = [UIImage imageWithData:imgData];
//            image = [self resizeImage:image];
//            NSData *data = UIImageJPEGRepresentation(image, 0);
            [button setImage:[UIImage imageWithData:imgData] forState:UIControlStateNormal];
            NSLog(@"Size of Image(bytes):%d", (int)[imgData length]);
        }
        [stack setAlpha:1];
        button.enabled = YES;
        [name setText:displayName];
        i++;
    }
    while (i < _stackViews.count) {
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[1];
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
    _logs = [NSMutableDictionary dictionary];
    _sortedDates = [NSMutableArray array];
    _contactsToDisplay = [NSMutableArray array];
    _imgs = [NSMutableDictionary dictionary];
    _updateNeeded = YES;
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
    [_imgs removeAllObjects];
    [_logs removeAllObjects];
    [_sortedDates removeAllObjects];
    [_contactsToDisplay removeAllObjects];
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
