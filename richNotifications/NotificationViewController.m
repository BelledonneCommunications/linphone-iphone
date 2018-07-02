//
//  NotificationViewController.m
//  richNotifications
//
//  Created by David Idmansour on 22/06/2018.
//

#import "NotificationViewController.h"
#import "NotificationTableViewCell.h"
#import <UserNotifications/UserNotifications.h>
#import <UserNotificationsUI/UserNotificationsUI.h>

@interface NotificationViewController () <UNNotificationContentExtension>

@end

@implementation NotificationViewController {
    @private
    NSMutableArray *msgs;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    self.tableView.scrollEnabled = TRUE;
    // Do any required interface initialization here.
}

- (void)didReceiveNotification:(UNNotification *)notification {
//    static float initialHeight = -1;
//    if (initialHeight < 0)
//        initialHeight = self.tableView.frame.size.height;
    if (msgs)
        [msgs addObject:[((NSArray *)[[[[notification request] content] userInfo] objectForKey:@"msgs"]) lastObject]];
    else
        msgs = [NSMutableArray arrayWithArray:[[[[notification request] content] userInfo] objectForKey:@"msgs"]];
    [self.tableView reloadData];
    [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForRow:msgs.count - 1 inSection:0] atScrollPosition:UITableViewScrollPositionBottom animated:YES];
//    float height = 0;
//    for (int i = 0 ; i < self->msgs.count ; i++) {
//        height += [self tableView:self.tableView heightForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:0]];
//    }
//    if (height > initialHeight) {
//        CGRect frame = self.tableView.frame;
//        frame.size.height = height;
//        frame.origin = CGPointMake(0, 0);
//        self.tableView.frame = frame;
//        self.tableView.bounds = frame;
//        self.preferredContentSize = CGSizeMake(self.preferredContentSize.width, height);
//    }
}

#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return msgs.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    BOOL isOutgoing = ((NSNumber *)[msgs[indexPath.row] objectForKey:@"isOutgoing"]).boolValue;
    NSString *display = ((NSString *)[msgs[indexPath.row] objectForKey:@"displayNameDate"]);
    NSString *msgText = ((NSString *)[msgs[indexPath.row] objectForKey:@"msg"]);
    NSString *imdm = ((NSString *)[msgs[indexPath.row] objectForKey:@"state"]);
    NSData *imageData = [msgs[indexPath.row] objectForKey:@"fromImageData"];
    NotificationTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"notificationCell" forIndexPath:indexPath];
    
    cell.background.image = cell.bottomBarColor.image = [UIImage imageNamed:isOutgoing ? @"color_A" : @"color_D.png"];
    cell.contactImage.image = [UIImage imageWithData:imageData];
    cell.nameDate.text = display;
    cell.msgText.text = msgText;
    cell.isOutgoing = isOutgoing;
    CGSize size = [cell ViewSizeForMessage:msgText withWidth:self.view.bounds.size.width - 10];
    cell.width = size.width;
    cell.height = size.height;
    cell.nameDate.textColor = [UIColor colorWithPatternImage:cell.background.image];
    cell.msgText.textColor = [UIColor darkGrayColor];
    cell.imdm.hidden = cell.imdmImage.hidden = !isOutgoing;
    
    if ([imdm isEqualToString:@"LinphoneChatMessageStateDelivered"] || [imdm isEqualToString:@"LinphoneChatMessageStateDeliveredToUser"]) {
        cell.imdm.text = NSLocalizedString(@"Delivered", nil);
        cell.imdm.textColor = [UIColor grayColor];
        cell.imdmImage.image = [UIImage imageNamed:@"chat_delivered.png"];
    } else if ([imdm isEqualToString:@"LinphoneChatMessageStateDisplayed"]) {
        cell.imdm.text = NSLocalizedString(@"Read", nil);
        cell.imdm.textColor = [UIColor colorWithRed:(24 / 255.0) green:(167 / 255.0) blue:(175 / 255.0) alpha:1.0];
        cell.imdmImage.image = [UIImage imageNamed:@"chat_read.png"];
    } else if ([imdm isEqualToString:@"LinphoneChatMessageStateNotDelivered"] || [imdm isEqualToString:@"LinphoneChatMessageStateFileTransferError"]) {
        cell.imdm.text = NSLocalizedString(@"Error", nil);
        cell.imdm.textColor = [UIColor redColor];
        cell.imdmImage.image = [UIImage imageNamed:@"chat_error.png"];
    } else
        cell.imdm.hidden = YES;
    
    return cell;
}

#pragma mark - UITableViewDelegate Functions

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    NotificationTableViewCell *cell = [[NotificationTableViewCell alloc] init];
    cell.msgText = [[UITextView alloc] init];
    cell.msgText.text = ((NSString *)[msgs[indexPath.row] objectForKey:@"msg"]);
    cell.msgText.font = [UIFont systemFontOfSize:17];
    return [cell ViewSizeForMessage:cell.msgText.text withWidth:self.view.bounds.size.width - 10].height + 5;
}

@end
