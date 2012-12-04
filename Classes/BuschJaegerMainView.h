/* BuschJaegerMainView.h
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import <UIKit/UIKit.h>

#import "BuschJaegerCallView.h"
#import "BuschJaegerSettingsView.h"
#import "BuschJaegerManualSettingsView.h"
#import "BuschJaegerWelcomeView.h"
#import "BuschJaegerHistoryView.h"
#import "BuschJaegerHistoryDetailsView.h"

@interface UINavigationControllerEx : UINavigationController

@end

@interface BuschJaegerMainView : UIViewController {
@private
    int loadCount;
    NSTimer *historyTimer;
    NSOperationQueue *historyQueue;
}

@property (nonatomic, retain) IBOutlet UINavigationControllerEx *navigationController;
@property (nonatomic, retain) IBOutlet BuschJaegerCallView *callView;
@property (nonatomic, retain) IBOutlet BuschJaegerSettingsView *settingsView;
@property (nonatomic, retain) IBOutlet BuschJaegerManualSettingsView *manualSettingsView;
@property (nonatomic, retain) IBOutlet BuschJaegerWelcomeView *welcomeView;
@property (nonatomic, retain) IBOutlet BuschJaegerHistoryView *historyView;
@property (nonatomic, retain) IBOutlet BuschJaegerHistoryDetailsView *historyDetailsView;

- (void)updateIconBadge:(id)info;

+ (BuschJaegerMainView*) instance;

@end
