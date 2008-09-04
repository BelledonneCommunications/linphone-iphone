/***************************************************************************
                          addressbook.h  -  
                             -------------------
    begin                : Wed Jan 30 2002
    copyright            : (C) 2002 by Simon Morlat
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


#ifndef ADDRESSBOOK_H
#define ADDRESSBOOK_H

GtkWidget *create_and_fill_address_book();
void show_address_book();
GtkWidget * contact_new(LinphoneFriend *lf, GtkWidget *ab);
GtkWidget * contact_edit(LinphoneFriend *lf, GtkWidget *ab);
GtkWidget * subscriber_edit(LinphoneFriend *lf);

#endif
