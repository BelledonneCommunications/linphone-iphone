//
//  TodayViewController.m
//  latestChatroomsWidget
//
//  Created by David Idmansour on 18/06/2018.
//

#import "TodayViewController.h"
#import <NotificationCenter/NotificationCenter.h>
#import "linphone/linphonecore.h"

@interface TodayViewController () <NCWidgetProviding>

@end

@implementation TodayViewController

- (void)loadData {    
    NSUserDefaults *mySharedDefaults = [[NSUserDefaults alloc] initWithSuiteName: @"group.belledonne-communications.linphone.widget"];
    [_chatrooms removeAllObjects];
    [_imgs removeAllObjects];
    [_addresses removeAllObjects];
    [_displayNames removeAllObjects];
    [_isConf removeAllObjects];
    _imgs = [NSMutableDictionary dictionaryWithDictionary:[mySharedDefaults objectForKey:@"imageData"]];
    _chatrooms = [NSMutableArray arrayWithArray:[mySharedDefaults objectForKey:@"chatrooms"]];
    for (NSDictionary *dict in _chatrooms) {
        [_addresses addObject:[dict objectForKey:@"address"]];
        [_displayNames addObject:[dict objectForKey:@"display"]];
        [_isConf addObject:[NSNumber numberWithBool:(((NSNumber *)[dict objectForKey:@"nbParticipants"]).intValue > 1)]];
    }
}

- (void)draw {
    int i = 0;
    for (i = 0 ; i < 4 && i < _addresses.count ; i++) {
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[0];
        UILabel *name = stack.subviews[1];
        printf("%s\n", ((NSString *)_addresses[i]).UTF8String);
        if ([self.imgs.allKeys containsObject:_addresses[i]]) {
            NSData *imgData = [_imgs objectForKey:_addresses[i]];
            [button setImage:[UIImage imageWithData:imgData] forState:UIControlStateNormal];
            NSLog(@"Size of Image(bytes):%d", (int)[imgData length]);
        } else if (((NSNumber *)_isConf[i]).boolValue) {
            [button setImage:[UIImage imageNamed:@"chat_group_avatar.png"] forState:UIControlStateNormal];
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
    
    _chatrooms = [NSMutableArray array];
    _addresses = [NSMutableArray array];
    _displayNames = [NSMutableArray array];
    _imgs = [NSMutableDictionary dictionary];
    _isConf = [NSMutableArray array];
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

- (void)launchOnChatConversationViewWithId:(NSString *)chatId {
    [self launchAppWithURL:[NSURL URLWithString:[@"linphone-widget://chatroom/show?" stringByAppendingString:chatId]]];
}

- (IBAction)firstButtonTapped {
    [self launchOnChatConversationViewWithId:_addresses[0]];
}

- (IBAction)secondButtonTapped {
    [self launchOnChatConversationViewWithId:_addresses[1]];
}

- (IBAction)thirdButtonTapped {
    [self launchOnChatConversationViewWithId:_addresses[2]];
}

- (IBAction)fourthButtonTapped {
    [self launchOnChatConversationViewWithId:_addresses[3]];
}
@end
