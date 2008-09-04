/*
applet.h - ome utils functions that cannot be set in interface.c.

Copyright (C) 2000  Simon MORLAT (simon.morlat@free.fr)

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef GUI_UTILS_H
#define GUI_UTILS_H

#include "linphone.h"

/* set audio levels on the main window*/
void set_levels(LinphoneGnomeUI *ui,gint reclev, gint playlev, gint ringlev);

/* display an alternate url (used in 380 response) */
void alt_ressource_display(LinphoneGnomeUI *ui, const gchar *url);

#endif

