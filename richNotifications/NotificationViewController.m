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
    if (msgs)
        [msgs addObject:[((NSArray *)[[[[notification request] content] userInfo] objectForKey:@"msgs"]) lastObject]];
    else
        msgs = [NSMutableArray arrayWithArray:[[[[notification request] content] userInfo] objectForKey:@"msgs"]];
    [self.tableView reloadData];
    [self.tableView scrollToRowAtIndexPath:[NSIndexPath indexPathForItem:msgs.count - 1
                                                               inSection:0]
                          atScrollPosition:UITableViewScrollPositionBottom
                                  animated:YES];
    NSLog(@"Content length : %f", self.tableView.contentSize.height);
    NSLog(@"Number of rows : %d", (unsigned int)[self tableView:self.tableView numberOfRowsInSection:0]);
    [self.view.superview bringSubviewToFront:self.tableView];
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
    printf("%s : %s : %s\n", isOutgoing ? "sortant" : "entrant",
           display.UTF8String,
           msgText.UTF8String);
    printf("Taille de l'image de profil : %d\n", (unsigned int)imageData.length);
    NotificationTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"notificationCell" forIndexPath:indexPath];
    cell.contactImage.image = [UIImage imageWithData:imageData];
    cell.nameDate.text = display;
    cell.msgText.text = msgText;
    if (!isOutgoing)
        cell.imdm.hidden = YES;
    if ([imdm isEqualToString:@"LinphoneChatMessageStateDelivered"])
        cell.imdm.text = NSLocalizedString(@"Delivered", nil);
    else if ([imdm isEqualToString:@"LinphoneChatMessageStateDisplayed"])
        cell.imdm.text = NSLocalizedString(@"Read", nil);
    else
        cell.imdm.text = imdm;
    return cell;
}

#pragma mark - UITableViewDelegate Functions

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 100;
}

@end
