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


/* status code*/
#define P2PPROXY_NO_ERROR 0
/*error codes*/
#define P2PPROXY_ERROR -1
#define P2PPROXY_ERROR_APPLICATION_NOT_STARTED  -2
#define P2PPROXY_ERROR_APPLICATION_ALREADY_STARTED  -3
#define P2PPROXY_ERROR_ACCOUNTMGT_USER_ALREADY_EXIST  -4
#define P2PPROXY_ERROR_ACCOUNTMGT_BAD_SIP_URI  -5
#define P2PPROXY_ERROR_RESOURCELOCATOR_SERVER_NOT_FOUND  -6

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

/***************************/
/***resource location******/
/***************************/
/**
* access a proxy registrar sip addreess 
* @param buffer allocated by the user
* @param size buffer size
* @return status code P2PPROXY_NO_ERROR, P2PPROXY_ERROR_RESOURCELOCATOR_SERVER_NOT_FOUND
*/
int p2pproxy_resourcelocation_get_sip_proxyregistrar_uri(char* string_buffer,size_t size) ;

#endif /*SWIG*/

#endif /*P2PPROXY_LAUNCHER_H_*/
