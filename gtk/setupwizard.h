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

#ifndef SETUP_WIZARD_H
#define SETUP_WIZARD_H

#include "linphone/sipsetup.h"

LINPHONE_PUBLIC void linphone_gtk_show_assistant(void);
LINPHONE_PUBLIC void linphone_gtk_assistant_prepare(GtkWidget *assistant);
LINPHONE_PUBLIC void linphone_gtk_assistant_closed(GtkWidget *w);

LINPHONE_PUBLIC void linphone_gtk_external_account_configuration_changed(GtkEntry* entry);
LINPHONE_PUBLIC void linphone_gtk_account_configuration_changed(GtkEntry *entry, GtkAssistant *assistant);
LINPHONE_PUBLIC void linphone_gtk_account_creation_username_changed(GtkEntry *entry);
LINPHONE_PUBLIC void linphone_gtk_account_creation_password_changed(GtkEntry *entry);
LINPHONE_PUBLIC void linphone_gtk_account_creation_email_changed(GtkEntry *entry);

#endif
