/* UIMutedMicroButton.m
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

#import "UIMutedMicroButton.h"

#import "LinphoneManager.h"

@implementation UIMutedMicroButton

- (void)onOn {
	linphone_core_enable_mic(LC, false);
}

- (void)onOff {
	linphone_core_enable_mic(LC, true);
}

- (bool)onUpdate {
	return (linphone_core_get_current_call(LC) || linphone_core_is_in_conference(LC)) && !linphone_core_mic_enabled(LC);
}

@end
