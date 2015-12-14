/*
vcard.h
Copyright (C) 2015  Belledonne Communications SARL

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

#ifndef LINPHONE_VCARD_H
#define LINPHONE_VCARD_H

#include <mediastreamer2/mscommon.h>

#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _LinphoneVCard LinphoneVCard;

LINPHONE_PUBLIC LinphoneVCard* linphone_vcard_new(void);
LINPHONE_PUBLIC void linphone_vcard_free(LinphoneVCard *vcard);
LINPHONE_PUBLIC MSList* linphone_vcard_new_from_vcard4_file(const char *file);

LINPHONE_PUBLIC void linphone_vcard_set_full_name(LinphoneVCard *vcard, const char *name);
LINPHONE_PUBLIC const char* linphone_vcard_get_full_name(const LinphoneVCard *vcard);

LINPHONE_PUBLIC MSList* linphone_vcard_get_sip_addresses(const LinphoneVCard *vcard);

#ifdef __cplusplus
}
#endif

#endif