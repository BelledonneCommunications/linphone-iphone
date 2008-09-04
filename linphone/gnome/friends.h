/***************************************************************************
                          friends.h  -  display of friend's list

                             -------------------
    begin                : Mon Dec 17 2001
    copyright            : (C) 2001 by Simon Morlat
    email                : simon.morlat@linphone.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <interface.h>

struct _FriendList {
	LinphoneCore *lc;
	GtkWidget *friendlist;
};

typedef struct _FriendList FriendList;

void friend_list_init(FriendList *fl,LinphoneCore *lc,GtkWidget *mainwidget);
void friend_list_set_friend_status(FriendList *fl, LinphoneFriend * fid, const gchar *url, const gchar *status, const gchar *img);
