/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
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

typedef NS_ENUM(int, TestState) { TestStateIdle, TestStatePassed, TestStateInProgress, TestStateFailed };

@interface TestItem : NSObject

@property(strong, nonatomic) NSString *suite;
@property(strong, nonatomic) NSString *name;
@property(nonatomic) TestState state;

- (id)initWithName:(NSString *)name fromSuite:(NSString *)suite;
+ (TestItem *)testWithName:(NSString *)name fromSuite:(NSString *)suite;

@end

@interface DetailTableView : UITableViewController

@property(strong, nonatomic) NSString *detailItem;

@end
