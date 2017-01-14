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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "linphone/core.h"
#include "linphone/lpconfig.h"
#include "private.h"

LinphoneAddress * _linphone_address_new(const char *addr){
	SalAddress *saddr=sal_address_new(addr);
	if (saddr==NULL)
		ms_error("Cannot create LinphoneAddress, bad uri [%s]",addr);
	return saddr;
}

LinphoneAddress * linphone_address_new(const char *addr) {
	return _linphone_address_new(addr);
}

LinphoneAddress * linphone_address_clone(const LinphoneAddress *addr){
	return sal_address_clone(addr);
}

LinphoneAddress * linphone_address_ref(LinphoneAddress *addr){
	return sal_address_ref(addr);
}

void linphone_address_unref(LinphoneAddress *addr){
	sal_address_unref(addr);
}

const char *linphone_address_get_scheme(const LinphoneAddress *u){
	return sal_address_get_scheme(u);
}

const char *linphone_address_get_display_name(const LinphoneAddress* u){
	return sal_address_get_display_name(u);
}

const char *linphone_address_get_username(const LinphoneAddress *u){
	return sal_address_get_username(u);
}

const char *linphone_address_get_domain(const LinphoneAddress *u){
	return sal_address_get_domain(u);
}

int linphone_address_get_port(const LinphoneAddress *u) {
	return sal_address_get_port(u);
}

int linphone_address_set_display_name(LinphoneAddress *u, const char *display_name){
	sal_address_set_display_name(u,display_name);
	return 0;
}

int linphone_address_set_username(LinphoneAddress *uri, const char *username){
	sal_address_set_username(uri,username);
	return 0;
}

int linphone_address_set_domain(LinphoneAddress *uri, const char *host){
	sal_address_set_domain(uri,host);
	return 0;
}

int linphone_address_set_port(LinphoneAddress *uri, int port){
	sal_address_set_port(uri,port);
	return 0;
}

int linphone_address_set_transport(LinphoneAddress *uri, LinphoneTransportType tp){
	sal_address_set_transport(uri,(SalTransport)tp);
	return 0;
}

LinphoneTransportType linphone_address_get_transport(const LinphoneAddress *uri){
	return (LinphoneTransportType)sal_address_get_transport(uri);
}

void linphone_address_set_method_param(LinphoneAddress *addr, const char *method) {
	sal_address_set_method_param(addr, method);
}

const char *linphone_address_get_method_param(const LinphoneAddress *addr) {
	return sal_address_get_method_param(addr);
}

void linphone_address_clean(LinphoneAddress *uri){
	sal_address_clean(uri);
}

char *linphone_address_as_string(const LinphoneAddress *u){
	return sal_address_as_string(u);
}

char *linphone_address_as_string_uri_only(const LinphoneAddress *u){
	return sal_address_as_string_uri_only(u);
}

bool_t linphone_address_is_secure(const LinphoneAddress *uri){
	return sal_address_is_secure(uri);
}

bool_t linphone_address_get_secure(const LinphoneAddress *uri){
	return sal_address_is_secure(uri);
}

void linphone_address_set_secure(LinphoneAddress *addr, bool_t enabled){
	sal_address_set_secure(addr, enabled);
}

bool_t linphone_address_is_sip(const LinphoneAddress *uri){
	return sal_address_is_sip(uri);
}

static bool_t strings_equals(const char *s1, const char *s2){
	if (s1==NULL && s2==NULL) return TRUE;
	if (s1!=NULL && s2!=NULL && strcmp(s1,s2)==0) return TRUE;
	return FALSE;
}

bool_t linphone_address_weak_equal(const LinphoneAddress *a1, const LinphoneAddress *a2){
	const char *u1,*u2;
	const char *h1,*h2;
	int p1,p2;
	u1=linphone_address_get_username(a1);
	u2=linphone_address_get_username(a2);
	p1=linphone_address_get_port(a1);
	p2=linphone_address_get_port(a2);
	h1=linphone_address_get_domain(a1);
	h2=linphone_address_get_domain(a2);
	return strings_equals(u1,u2) && strings_equals(h1,h2) && p1==p2;
}

bool_t linphone_address_equal(const LinphoneAddress *a1, const LinphoneAddress *a2) {
	char *s1;
	char *s2;
	bool_t res;
	if ((a1 == NULL) && (a2 == NULL)) return TRUE;
	if ((a1 == NULL) || (a2 == NULL)) return FALSE;
	s1 = linphone_address_as_string(a1);
	s2 = linphone_address_as_string(a2);
	res = (strcmp(s1, s2) == 0) ? TRUE : FALSE;
	ms_free(s1);
	ms_free(s2);
	return res;
}

void linphone_address_destroy(LinphoneAddress *u){
	linphone_address_unref(u);
}

void linphone_address_set_password(LinphoneAddress *addr, const char *passwd){
	sal_address_set_password(addr,passwd);
}

const char *linphone_address_get_password(const LinphoneAddress *addr){
	return sal_address_get_password(addr);
}

void linphone_address_set_header(LinphoneAddress *addr, const char *header_name, const char *header_value){
	sal_address_set_header(addr,header_name,header_value);
}

bool_t linphone_address_has_param(const LinphoneAddress *addr, const char *name) {
	return sal_address_has_param(addr, name);
}

const char * linphone_address_get_param(const LinphoneAddress *addr, const char *name) {
	return sal_address_get_param(addr, name);
}

void linphone_address_set_param(LinphoneAddress *addr, const char *name, const char *value) {
	sal_address_set_param(addr, name, value);
}

void linphone_address_set_params(LinphoneAddress *addr, const char *params) {
	sal_address_set_params(addr, params);
}

void linphone_address_set_uri_param(LinphoneAddress *addr, const char *name, const char *value) {
	sal_address_set_uri_param(addr, name, value);
}

void linphone_address_set_uri_params(LinphoneAddress *addr, const char *params) {
	sal_address_set_uri_params(addr, params);
}

bool_t linphone_address_has_uri_param(const LinphoneAddress *addr, const char *name) {
	return sal_address_has_uri_param(addr, name);
}

const char * linphone_address_get_uri_param(const LinphoneAddress *addr, const char *name) {
	return sal_address_get_uri_param(addr, name);
}

LinphoneAddress * linphone_core_create_address(LinphoneCore *lc, const char *address) {
	return linphone_address_new(address);
}

/** @} */
