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

@interface UILinphoneAudioPlayer : UIViewController
@property (weak, nonatomic) IBOutlet UIButton *playButton;
@property (weak, nonatomic) IBOutlet UIButton *stopButton;
@property (weak, nonatomic) IBOutlet UILabel *timeLabel;
@property (weak, nonatomic) IBOutlet UIProgressView *timeProgress;
@property (weak, nonatomic) NSString *file;
@property (weak, nonatomic) NSTimer *refreshTimer;

+ (id)audioPlayerWithFilePath:(NSString *)filePath;
- (void)close;
- (BOOL)isOpened;
- (BOOL)isCreated;
- (void)open;
- (void)pause;
- (void)setFile:(NSString *)fileName;
- (IBAction)onPlay:(id)sender;
- (IBAction)onStop:(id)sender;
- (IBAction)onTapTimeBar:(UITapGestureRecognizer *)sender;
@end
