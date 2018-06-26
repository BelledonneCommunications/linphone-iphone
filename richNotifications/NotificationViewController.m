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
    NSArray *msgs;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any required interface initialization here.
}

- (void)didReceiveNotification:(UNNotification *)notification {
    msgs = [[[[notification request] content] userInfo] objectForKey:@"msgs"];
    [self.tableView reloadData];
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
    NSData *imageData = [msgs[indexPath.row] objectForKey:@"fromImageData"];
    printf("Message %s de %s : %s\n", isOutgoing ? "sortant" : "entrant",
           display.UTF8String,
           msgText.UTF8String);
    printf("Taille de l'image de profil : %d\n", (unsigned int)imageData.length);
    NotificationTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"notificationCell" forIndexPath:indexPath];
    cell.contactImage.image = [UIImage imageWithData:imageData];
    cell.nameDate.text = display;
    cell.msgText.text = msgText;
    return cell;
}

#pragma mark - UITableViewDelegate Functions

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 100;
}

@end
