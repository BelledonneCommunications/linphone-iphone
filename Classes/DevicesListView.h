//
//  DevicesListView.h
//  linphone
//
//  Created by Danmei Chen on 06/11/2018.
//
#import <UIKit/UIKit.h>
#import "UICompositeView.h"

@interface DevicesMenuEntry : NSObject {
@public
    LinphoneParticipant *participant;
    NSInteger numberOfDevices;
};
@end

@interface DevicesListView : UIViewController <UICompositeViewDelegate, UITableViewDelegate, UITableViewDataSource>

@property (weak, nonatomic) IBOutlet UILabel *addressLabel;
@property (weak, nonatomic) IBOutlet UITableView *tableView;

@property(nonatomic) LinphoneChatRoom *room;
@property bctbx_list_t *devices;
@property NSMutableArray *devicesMenuEntries;
@property BOOL isOneToOne;

- (IBAction)onBackClick:(id)sender;

@end
