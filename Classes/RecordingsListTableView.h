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

#import "UICheckBoxTableView.h"

@interface RecordingsListTableView : UICheckBoxTableView {
@private
    NSMutableDictionary *recordings;
    //This has sub arrays indexed with the date of the recordings, themselves containings the recordings.
    NSString *writablePath;
    //This is the path to the folder where we write the recordings to. We should probably define it in LinphoneManager though.
}
- (void)loadData;
- (void)removeAllRecordings;
- (void)setSelected:(NSString *)filepath;

@end
