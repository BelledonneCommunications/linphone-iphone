/***************************************************************************
                          presence.h  -  code for the presence box
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

#ifndef PRESENCE_H
#define PRESENCE_H

enum { 	PRESENCE_MODE_REACHABLE=LINPHONE_STATUS_ONLINE,
				PRESENCE_MODE_BUSY=LINPHONE_STATUS_BUSY,
				PRESENCE_MODE_AWAY=LINPHONE_STATUS_AWAY,
				PRESENCE_MODE_NOT_DISTURB=LINPHONE_STATUS_NOT_DISTURB,
				PRESENCE_MODE_MOVED=LINPHONE_STATUS_MOVED,
				PRESENCE_MODE_ALT_SERVICE=LINPHONE_STATUS_ALT_SERVICE
				};
				

typedef struct _PresenceBox
{
	LinphoneCore *lc;
	GtkWidget *minutesaway;
	GtkWidget *contact_field;
	gint toggled_button; /* indicate which button is toggled*/
} PresenceBox;


void presence_box_init(PresenceBox *p, GtkWidget *main_window,LinphoneCore *lc);
void presence_box_changed(PresenceBox *p);
void presence_box_apply(PresenceBox *p);

#endif
