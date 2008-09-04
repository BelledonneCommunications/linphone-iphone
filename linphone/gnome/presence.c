/***************************************************************************
                          presence.c  -  code for the presence box
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
#include "linphone.h"
#include "callbacks.h"
#include "support.h"


void presence_box_init(PresenceBox *p, GtkWidget *main_window,LinphoneCore *lc)
{
	GtkWidget *r;
	p->lc=lc;
	p->contact_field=lookup_widget(main_window,"contact_field");
	p->minutesaway=lookup_widget(main_window,"minutesaway");
	r=lookup_widget(main_window,"presence_reachable");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(r),TRUE);
	r=lookup_widget(main_window,"presence_frame");
}

void presence_box_changed(PresenceBox *p)
{
	presence_box_apply(p);
}

#define get_presence_box() (&(uiobj)->main_window.presencebox)


void
on_reachable                           (GtkToggleButton *togglebutton,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	if (!gtk_toggle_button_get_active(togglebutton)) return;
	//gtk_widget_set_sensitive(p->contact_field,FALSE);
	p->toggled_button=PRESENCE_MODE_REACHABLE;
	//gtk_widget_set_sensitive(p->minutesaway,FALSE);
	//gtk_widget_set_sensitive(p->contact_field,FALSE);
	presence_box_changed(p);
}


void
on_busy                                (GtkToggleButton *togglebutton,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	if (!gtk_toggle_button_get_active(togglebutton)) return;
	//gtk_widget_set_sensitive(p->contact_field,FALSE);
	p->toggled_button=PRESENCE_MODE_BUSY;
	//gtk_widget_set_sensitive(p->minutesaway,TRUE);
	presence_box_changed(p);
}


void
on_minutesaway_changed                 (GtkEditable *editable,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	presence_box_changed(p);
}


void
on_away                                (GtkToggleButton *togglebutton,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	if (!gtk_toggle_button_get_active(togglebutton)) return;
	//gtk_widget_set_sensitive(p->contact_field,FALSE);
	//gtk_widget_set_sensitive(p->minutesaway,TRUE);
	p->toggled_button=PRESENCE_MODE_AWAY;
	presence_box_changed(p);
}


void
on_do_not_disturb                      (GtkToggleButton *togglebutton,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	if (!gtk_toggle_button_get_active(togglebutton)) return;
	//gtk_widget_set_sensitive(p->contact_field,FALSE);
	//gtk_widget_set_sensitive(p->minutesaway,FALSE);
	p->toggled_button=PRESENCE_MODE_NOT_DISTURB;
	presence_box_changed(p);
}


void
on_moved_tmply                         (GtkToggleButton *togglebutton,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	if (!gtk_toggle_button_get_active(togglebutton)) return;
  //gtk_widget_set_sensitive(p->contact_field,TRUE);
  //gtk_widget_set_sensitive(p->minutesaway,FALSE);
  p->toggled_button=PRESENCE_MODE_MOVED;
  presence_box_changed(p);
}


void
on_alt_serv                            (GtkToggleButton *togglebutton,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	if (!gtk_toggle_button_get_active(togglebutton)) return;
  //gtk_widget_set_sensitive(p->contact_field,TRUE);
  //gtk_widget_set_sensitive(p->minutesaway,FALSE);
  p->toggled_button=PRESENCE_MODE_ALT_SERVICE;
  presence_box_changed(p);
}


void
on_contact_field_changed               (GtkEditable *entry,gpointer         user_data)
{
	PresenceBox *p=get_presence_box();
	  presence_box_changed(p);
}

void presence_box_apply(PresenceBox *p)
{
	gchar *tmp,*contact=NULL;
	int minutes_away=-1;
	g_message("presence_box_apply");
	/* retrieve the minutes away */
	tmp = gtk_editable_get_chars (GTK_EDITABLE(p->minutesaway),0,-1);
	if (tmp!=NULL && strlen(tmp)>0)
	{
		minutes_away = atoi(tmp);
		g_free(tmp);
	}
	/* retrieve the alternate contact url */
	tmp = gtk_editable_get_chars (GTK_EDITABLE(p->contact_field),0,-1);
	if (tmp!=NULL && strlen(tmp)>0)
	{
		contact=tmp;
	}
	/* set presence mode */
	linphone_core_set_presence_info(p->lc,minutes_away,contact,p->toggled_button);
	if (tmp!=NULL) g_free(tmp);
}
