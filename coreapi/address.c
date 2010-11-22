/*
linphone
Copyright (C) 2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphonecore.h"
#include "lpconfig.h"
#include "private.h"

/**
 * @addtogroup linphone_address
 * @{
**/

/**
 * Constructs a LinphoneAddress object by parsing the user supplied address,
 * given as a string.
**/
LinphoneAddress * linphone_address_new(const char *addr){
	SalAddress *saddr=sal_address_new(addr);
	if (saddr==NULL) ms_error("Cannot create LinphoneAddress, bad uri [%s]",addr);
	return saddr;
}

/**
 * Clones a LinphoneAddress object.
**/
LinphoneAddress * linphone_address_clone(const LinphoneAddress *addr){
	return sal_address_clone(addr);
}

/**
 * Returns the address scheme, normally "sip".
**/
const char *linphone_address_get_scheme(const LinphoneAddress *u){
	return sal_address_get_scheme(u);
}

/**
 * Returns the display name.
**/
const char *linphone_address_get_display_name(const LinphoneAddress* u){
	return sal_address_get_display_name(u);
}

/**
 * Returns the username.
**/
const char *linphone_address_get_username(const LinphoneAddress *u){
	return sal_address_get_username(u);
}

/**
 * Returns the domain name.
**/
const char *linphone_address_get_domain(const LinphoneAddress *u){
	return sal_address_get_domain(u);
}

/**
 * Sets the display name.
**/
void linphone_address_set_display_name(LinphoneAddress *u, const char *display_name){
	sal_address_set_display_name(u,display_name);
}

/**
 * Sets the username.
**/
void linphone_address_set_username(LinphoneAddress *uri, const char *username){
	sal_address_set_username(uri,username);
}

/**
 * Sets the domain.
**/
void linphone_address_set_domain(LinphoneAddress *uri, const char *host){
	sal_address_set_domain(uri,host);
}

/**
 * Sets the port number.
**/
void linphone_address_set_port(LinphoneAddress *uri, const char *port){
	sal_address_set_port(uri,port);
}

/**
 * Sets the port number.
**/
void linphone_address_set_port_int(LinphoneAddress *uri, int port){
	sal_address_set_port_int(uri,port);
}

/**
 * Removes address's tags and uri headers so that it is displayable to the user.
**/
void linphone_address_clean(LinphoneAddress *uri){
	sal_address_clean(uri);
}

/**
 * Returns the address as a string.
 * The returned char * must be freed by the application. Use ms_free().
**/
char *linphone_address_as_string(const LinphoneAddress *u){
	return sal_address_as_string(u);
}

/**
 * Returns the SIP uri only as a string, that is display name is removed.
 * The returned char * must be freed by the application. Use ms_free().
**/
char *linphone_address_as_string_uri_only(const LinphoneAddress *u){
	return sal_address_as_string_uri_only(u);
}

static bool_t strings_equals(const char *s1, const char *s2){
	if (s1==NULL && s2==NULL) return TRUE;
	if (s1!=NULL && s2!=NULL && strcmp(s1,s2)==0) return TRUE;
	return FALSE;
}

/**
 * Compare two LinphoneAddress ignoring tags and headers, basically just domain, username, and port.
 * Returns TRUE if they are equal.
**/
bool_t linphone_address_weak_equal(const LinphoneAddress *a1, const LinphoneAddress *a2){
	const char *u1,*u2;
	const char *h1,*h2;
	int p1,p2;
	u1=linphone_address_get_username(a1);
	u2=linphone_address_get_username(a2);
	p1=linphone_address_get_port_int(a1);
	p2=linphone_address_get_port_int(a2);
	h1=linphone_address_get_domain(a1);
	h2=linphone_address_get_domain(a2);
	return strings_equals(u1,u2) && strings_equals(h1,h2) && p1==p2;
}

/**
 * Destroys a LinphoneAddress object.
**/
void linphone_address_destroy(LinphoneAddress *u){
	sal_address_destroy(u);
}

int linphone_address_get_port_int(const LinphoneAddress *u) {
	return sal_address_get_port_int(u);
}

const char* linphone_address_get_port(const LinphoneAddress *u) {
	return sal_address_get_port(u);
}

/** @} */
