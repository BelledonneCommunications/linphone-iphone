/* ConferenceCallDetailView.h
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */     
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
