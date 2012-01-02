//
//  ConferenceCallDetailView.h
//  linphone
//
//  Created by Pierre-Eric Pelloux-Prayer on 25/11/11.
//  Copyright (c) 2011 Belledonne Communications. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "LinphoneUI/UIMuteButton.h"
#import "LinphoneUI/UISpeakerButton.h"

@interface ConferenceCallDetailView : UIViewController<UITableViewDelegate, UITableViewDataSource> {

    UIMuteButton* mute;
    UISpeakerButton* speaker;
    UIButton* back;
    UIButton* hangup;
    UITableView* table;
    
    UITableViewCell* conferenceDetailCell;
}

@property (nonatomic, retain) IBOutlet UIButton* mute;
@property (nonatomic, retain) IBOutlet UIButton* speaker;
@property (nonatomic, retain) IBOutlet UIButton* addCall;
@property (nonatomic, retain) IBOutlet UIButton* back;
@property (nonatomic, retain) IBOutlet UIButton* hangup;
@property (nonatomic, retain) IBOutlet UITableView* table;

@property (nonatomic, assign) IBOutlet UITableViewCell* conferenceDetailCell;

-(void) updateCallQuality;
@end
