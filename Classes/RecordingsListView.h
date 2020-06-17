/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "RecordingsListTableView.h"
#import "UIInterfaceStyleButton.h"

typedef enum _RecordingSelectionMode { RecordingSelectionModeNone, RecordingSelectionModeEdit } RecordingSelectionMode;

@interface RecordingSelection : NSObject <UISearchBarDelegate> {
}

+ (void)setSelectionMode:(RecordingSelectionMode)selectionMode;
+ (RecordingSelectionMode)getSelectionMode;

@end

@interface RecordingsListView : UIViewController <UICompositeViewDelegate>

@property(strong, nonatomic) IBOutlet RecordingsListTableView *tableController;
@property(strong, nonatomic) IBOutlet UIView *topBar;
@property(weak, nonatomic) IBOutlet UIButton *deleteButton;
@property (strong, nonatomic) IBOutlet UIButton *backButton;
@property (weak, nonatomic) IBOutlet UIInterfaceStyleButton *toggleSelectionButton;

- (IBAction)onDeleteClick:(id)sender;
- (IBAction)onEditionChangeClick:(id)sender;
- (IBAction)onBackPressed:(id)sender;

@end
