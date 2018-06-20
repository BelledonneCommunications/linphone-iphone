//
//  TodayViewController.m
//  latestChatroomsWidget
//
//  Created by David Idmansour on 18/06/2018.
//

#import "TodayViewController.h"
#import <NotificationCenter/NotificationCenter.h>

@interface TodayViewController () <NCWidgetProviding>

@end

@implementation TodayViewController

- (void)loadData {    
    NSUserDefaults *mySharedDefaults = [[NSUserDefaults alloc] initWithSuiteName: @"group.belledonne-communications.linphone.widget"];
    [_imgs removeAllObjects];
    [_addresses removeAllObjects];
    [_localAddress removeAllObjects];
    [_displayNames removeAllObjects];
    [_isConf removeAllObjects];
    NSMutableArray *chatrooms = [NSMutableArray arrayWithArray:[mySharedDefaults objectForKey:@"chatrooms"]];
    for (NSDictionary *dict in chatrooms) {
        [_addresses addObject:[dict objectForKey:@"peer"]];
        [_localAddress addObject:[dict objectForKey:@"local"]];
        [_displayNames addObject:[dict objectForKey:@"display"]];
        [_isConf addObject:(NSNumber *)[dict objectForKey:@"nbParticipants"]];
        [_imgs addObject:[dict objectForKey:@"img"]?:[NSNull null]];
    }
}

- (void)draw {
    int i = 0;
    for (i = 0 ; i < 4 && i < _addresses.count ; i++) {
        UIStackView *stack = _stackViews[i];
        UIButton *button = stack.subviews[0];
        UILabel *name = stack.subviews[1];
        if (_imgs[i] != [NSNull null]) {
            [button setImage:[UIImage imageWithData:_imgs[i]] forState:UIControlStateNormal];
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
    
    _addresses = [NSMutableArray array];
    _localAddress = [NSMutableArray array];
    _displayNames = [NSMutableArray array];
    _imgs = [NSMutableArray array];
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

- (void)launchOnChatConversationViewWithPeerAddress:(NSString *)peerAddress andLocalAddress:(NSString *)localAddress {
    [self launchAppWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"linphone-widget://chatroom/show?peer=%@&local=%@", peerAddress, localAddress]]];
}

- (IBAction)firstButtonTapped {
    [self launchOnChatConversationViewWithPeerAddress:_addresses[0] andLocalAddress:_localAddress[0]];
}

- (IBAction)secondButtonTapped {
    [self launchOnChatConversationViewWithPeerAddress:_addresses[1] andLocalAddress:_localAddress[1]];
}

- (IBAction)thirdButtonTapped {
    [self launchOnChatConversationViewWithPeerAddress:_addresses[2] andLocalAddress:_localAddress[2]];
}

- (IBAction)fourthButtonTapped {
    [self launchOnChatConversationViewWithPeerAddress:_addresses[3] andLocalAddress:_localAddress[3]];
}
@end
