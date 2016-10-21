/* HistoryViewController.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "HistoryListView.h"
#import "PhoneMainView.h"
#import "LinphoneUI/UIHistoryCell.h"

@implementation HistoryListView

typedef enum _HistoryView { History_All, History_Missed, History_MAX } HistoryView;

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:HistoryDetailsView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	if ([_tableController isEditing]) {
		[_tableController setEditing:FALSE animated:FALSE];
	}
	[self changeView:History_All];
	[self onEditionChangeClick:nil];

	// Reset missed call
	linphone_core_reset_missed_calls_count(LC);
	// Fake event
	[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
}

- (void) viewWillDisappear:(BOOL)animated {
	self.view = NULL;
}

#pragma mark -

- (void)changeView:(HistoryView)view {
	CGRect frame = _selectedButtonImage.frame;
	if (view == History_All) {
		frame.origin.x = _allButton.frame.origin.x;
		_allButton.selected = TRUE;
		[_tableController setMissedFilter:FALSE];
		_missedButton.selected = FALSE;
	} else {
		frame.origin.x = _missedButton.frame.origin.x;
		_missedButton.selected = TRUE;
		[_tableController setMissedFilter:TRUE];
		_allButton.selected = FALSE;
	}
	_selectedButtonImage.frame = frame;
}

#pragma m ~ark - Action Functions

- (IBAction)onAllClick:(id)event {
	[self changeView:History_All];
}

- (IBAction)onMissedClick:(id)event {
	[self changeView:History_Missed];
}

- (IBAction)onDeleteClick:(id)event {
	NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete selected logs?", nil)];
	[UIConfirmationDialog ShowWithMessage:msg
		cancelMessage:nil
		confirmMessage:nil
		onCancelClick:^() {
		  [self onEditionChangeClick:nil];
		}
		onConfirmationClick:^() {
		  [_tableController removeSelectionUsing:nil];
		  [_tableController loadData];
		  [self onEditionChangeClick:nil];
		}];
}

- (IBAction)onEditionChangeClick:(id)sender {
	_allButton.hidden = _missedButton.hidden = _selectedButtonImage.hidden = self.tableController.isEditing;
}

@end
