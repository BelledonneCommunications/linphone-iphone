/*
linphone
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

#ifndef ENUM_LOOKUP_H
#define ENUM_LOOKUP_H

#include "private.h"

#define MAX_ENUM_LOOKUP_RESULTS 10

typedef struct enum_lookup_res{
	char *sip_address[MAX_ENUM_LOOKUP_RESULTS];
}enum_lookup_res_t;

bool_t is_enum(const char *sipaddress, char **enum_domain);
int enum_lookup(const char *enum_domain, enum_lookup_res_t **res);
void enum_lookup_res_free(enum_lookup_res_t *res);

#endif
