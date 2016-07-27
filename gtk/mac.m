/*
linphone, gtk-glade interface.
Copyright (C) 2008  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import "linphone.h"


static int unread_messages_count() {
	LinphoneCore* lc = linphone_gtk_get_core();
	int count = 0;
	const MSList *rooms = linphone_core_get_chat_rooms(lc);
	const MSList *item = rooms;
	while (item) {
		LinphoneChatRoom *room = (LinphoneChatRoom *)item->data;
		if (room) {
			count += linphone_chat_room_get_unread_messages_count(room);
		}
		item = item->next;
	}
	return count;
}

void linphone_gtk_update_badge_count() {
	int count = unread_messages_count();
	NSString* badgeStr = (count > 0) ? [NSString stringWithFormat:@"%d", count] : @"";
	[[NSApp dockTile] setBadgeLabel:badgeStr];
}

#endif
