/*
p2pproxy Copyright (C) 2007  Jehan Monnier ()

p2pproxy.h - sip proxy.

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
#ifndef P2PPROXY_LAUNCHER_H_
#define P2PPROXY_LAUNCHER_H_

#include <stdio.h>
#ifdef SWIG
%module P2pProxylauncher
%javaconst(1);
%include "p2pproxy.h"
#endif /*SWIG*/

#define P2PPROXY_ACCOUNTMGT_USER_EXIST 1
#define P2PPROXY_ACCOUNTMGT_USER_NOT_EXIST 0 
#define P2PPROXY_RESOURCEMGT_SERVER_NOT_FOUND  3
/* state code*/
#define P2PPROXY_CONNECTED 2
#define P2PPROXY_NOT_CONNECTED 1
/* status code*/
#define P2PPROXY_NO_ERROR 0
/*error codes*/
#define P2PPROXY_ERROR -1
#define P2PPROXY_ERROR_APPLICATION_NOT_STARTED  -2
#define P2PPROXY_ERROR_APPLICATION_ALREADY_STARTED  -3
#define P2PPROXY_ERROR_ACCOUNTMGT_USER_ALREADY_EXIST  -4
#define P2PPROXY_ERROR_ACCOUNTMGT_BAD_SIP_URI  -5
#define P2PPROXY_ERROR_RESOURCEMGT_SERVER_NOT_FOUND  -6

#ifndef SWIG
/**
 *  start p2pproxy application
 *  blocking call
 *  @param argc number of argument
 *  @param argv arguments
 * 	@return status code
 * 
 */
int p2pproxy_application_start(int argc, char **argv);

/**
 *  return status
 * 	@return P2PPROXY_CONNECTED, P2PPROXY_NOT_ERROR
 * 
 */
int p2pproxy_application_get_state(void);

/**
 *  stop p2pproxy application
 * 
 */
int p2pproxy_application_stop(void);

/**
 * return the status string corresponding to the status code
 */
/*const char* p2pproxy_status_string(int status_code);*/

/************************/
/***account management***/
/************************/

/**
* create an account with the given name (must be unique)
* @param user_name user sip uri (sip:joe@p2p.linphone.org)
*  
* @return  P2PPROXY_NO_ERROR, P2PPROXY_ERROR_APPLICATIONNOTSTARTED, P2PPROXY_ERROR_ACCOUNTMGT_USERALREADYEXIST, P2PPROXY_ERROR_ACCOUNTMGT_BADSIPURI
*/
int p2pproxy_accountmgt_createAccount(const char* user_name);
/**
* check if a user name has been already created
* @param user_name user sip uri (sip:joe@p2p.linphone.org)
* @return P2PPROXY_ACCOUNTMGT_USEREXIST, P2PPROXY_ACCOUNTMGT_USERNOTEXIST , P2PPROXY_ERROR_APPLICATIONNOTSTARTED
*/
int p2pproxy_accountmgt_isValidAccount(const char* user_name);

/**
* delete an account with the given name 
* @param user_name  user sip uri (sip:joe@p2p.linphone.org)
* @return P2PPROXY_NO_ERROR, P2PPROXY_ERROR_APPLICATIONNOTSTARTED
*/
int p2pproxy_accountmgt_deleteAccount(const char* user_name);

/****************************/
/***resource management******/
/****************************/
/**
 * Structure to store resource list, must be instanciated by 
 * p2pproxy_resourcemgt_new_resource_list and deleted by p2pproxy_resourcemgt_delete_resource_list.
 * 
 */
struct p2pproxy_resourcemgt_resource_list {
	char[]* resource_uri; /* uri list*/
	unsigned char size;   /*number of element in the list*/
} p2pproxy_resourcemgt_resource_list_t;

/**
 * Instanciate a p2pproxy_resourcemgt_resource_list
 */
p2pproxy_resourcemgt_resource_list_t* p2pproxy_resourcemgt_new_resource_list();
/**
 * delete a p2pproxy_resourcemgt_resource_list 
 */
void p2pproxy_resourcemgt_delete_resource_list(p2pproxy_resourcemgt_resource_list_t* resource_list);

/**
* access a proxy registrar sip addreess for a given domaine name 
* @param [out] proxy_uri buffer allocated by the user
* @param [in] size buffer size
* @param [in] domaine name
* @return status code P2PPROXY_NO_ERROR, P2PPROXY_ERROR_RESOURCELOCATOR_SERVER_NOT_FOUND
*/
int p2pproxy_resourcemgt_lookup_sip_proxy(char* proxy_uri,size_t size, char* domaine) ;
/**
* access a media ressource addresses for a given domaine name 
* @param [out] p2pproxy_resourcemgt_resource_list_t  allocated by the user
* @param [in] domaine name
* @return status code P2PPROXY_NO_ERROR, P2PPROXY_ERROR_RESOURCELOCATOR_SERVER_NOT_FOUND
*/
int p2pproxy_resourcemgt_lookup_media_resource(p2pproxy_resourcemgt_resource_list_t* resource_list, char* domaine) ;
/*
 * notify the library at a given proxy is no longuer reachable 
* @param [in] proxy sip uri
* @return status code P2PPROXY_NO_ERROR
*/
int p2pproxy_resourcemgt_revoke_sip_proxy(char* proxy_uri);

/*
 * notify the library at a given Media resoure is no longuer reachable 
* @param [in] media resource uri (udp://hostname:port)
* @return status code P2PPROXY_NO_ERROR
*/
int p2pproxy_resourcemgt_revoke_media_resource(char* resource_uri);

#endif /*SWIG*/

#endif /*P2PPROXY_LAUNCHER_H_*/
