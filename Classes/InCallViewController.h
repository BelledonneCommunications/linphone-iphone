/* InCallViewController.h
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.                                 
 *                                                                      
 *  This program is distributed in the hope that it will be useful,     
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of      
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */              

#import <UIKit/UIKit.h>

#import "VideoZoomHandler.h"
#import "UICamSwitch.h"
#import "CallDelegate.h"

#import "UICompositeViewController.h"
#import "InCallTableViewController.h"

@class VideoViewController;

@interface UIPreviewView : UIView

@end

@interface InCallViewController : UIViewController <CallActionSheetDelegate, UIGestureRecognizerDelegate, UICompositeViewDelegate> {
    
    InCallTableViewController* callTableController;
    UITableView* callTableView;
    
    UIView* videoGroup;
    UIView* videoView;
    UIView* videoPreview;
#ifdef TEST_VIDEO_VIEW_CHANGE
    UIView* testVideoView;
#endif
    UICamSwitch* videoCameraSwitch;
    
    UIActivityIndicatorView* videoWaitingForFirstImage;
    
    NSTimer* hideControlsTimer;
    
    BOOL videoShown;
    VideoZoomHandler* videoZoomHandler;
    
    UIActionSheet* visibleActionSheet;
}

@property (nonatomic, retain) IBOutlet InCallTableViewController* callTableController;
@property (nonatomic, retain) IBOutlet UITableView* callTableView;

@property (nonatomic, retain) IBOutlet UIView* videoGroup;
@property (nonatomic, retain) IBOutlet UIView* videoView;
#ifdef TEST_VIDEO_VIEW_CHANGE
@property (nonatomic, retain) IBOutlet UIView* testVideoView;
#endif
@property (nonatomic, retain) IBOutlet UIView* videoPreview;
@property (nonatomic, retain) IBOutlet UICamSwitch* videoCameraSwitch;
@property (nonatomic, retain) IBOutlet UIActivityIndicatorView* videoWaitingForFirstImage;

@end
