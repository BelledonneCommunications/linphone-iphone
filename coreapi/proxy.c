/*
linphone
Copyright (C) 2000  Simon MORLAT (simon.morlat@linphone.org)
*/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "linphonecore.h"
#include "sipsetup.h"
#include "lpconfig.h"
#include "private.h"
#include "mediastreamer2/mediastream.h"

#include <ctype.h>


void linphone_proxy_config_write_all_to_config_file(LinphoneCore *lc){
	MSList *elem;
	int i;
	if (!linphone_core_ready(lc)) return;
	
	for(elem=lc->sip_conf.proxies,i=0;elem!=NULL;elem=ms_list_next(elem),i++){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		linphone_proxy_config_write_to_config_file(lc->config,cfg,i);
	}
	/*to ensure removed configs are erased:*/
	linphone_proxy_config_write_to_config_file(lc->config,NULL,i);
	lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy(lc,NULL));
}

static void linphone_proxy_config_init(LinphoneCore* lc,LinphoneProxyConfig *obj){
	memset(obj,0,sizeof(LinphoneProxyConfig));
	obj->magic=linphone_proxy_config_magic;
	obj->expires=LP_CONFIG_DEFAULT_INT((lc?lc->config:NULL),"reg_expires",3600);
	obj->dial_prefix=ms_strdup(LP_CONFIG_DEFAULT_STRING((lc?lc->config:NULL),"dial_prefix",'\0'));
	obj->dial_escape_plus=LP_CONFIG_DEFAULT_INT((lc?lc->config:NULL),"dial_escape_plus",0);
}

/**
 * @addtogroup proxies
 * @{
**/

/**
 * @deprecated, use #linphone_core_create_proxy_config instead
 *Creates an empty proxy config.
**/
LinphoneProxyConfig *linphone_proxy_config_new() {
	return linphone_core_create_proxy_config(NULL);
}
LinphoneProxyConfig * linphone_core_create_proxy_config(LinphoneCore *lc) {
	LinphoneProxyConfig *obj=NULL;
	obj=ms_new(LinphoneProxyConfig,1);
	linphone_proxy_config_init(lc,obj);
	return obj;
}



/**
 * Destroys a proxy config.
 * 
 * @note: LinphoneProxyConfig that have been removed from LinphoneCore with
 * linphone_core_remove_proxy_config() must not be freed.
**/
void linphone_proxy_config_destroy(LinphoneProxyConfig *obj){
	if (obj->reg_proxy!=NULL) ms_free(obj->reg_proxy);
	if (obj->reg_identity!=NULL) ms_free(obj->reg_identity);
	if (obj->reg_route!=NULL) ms_free(obj->reg_route);
	if (obj->ssctx!=NULL) sip_setup_context_free(obj->ssctx);
	if (obj->realm!=NULL) ms_free(obj->realm);
	if (obj->type!=NULL) ms_free(obj->type);
	if (obj->dial_prefix!=NULL) ms_free(obj->dial_prefix);
	if (obj->op) sal_op_release(obj->op);
	if (obj->publish_op) sal_op_release(obj->publish_op);
}

/**
 * Returns a boolean indicating that the user is sucessfully registered on the proxy.
**/
bool_t linphone_proxy_config_is_registered(const LinphoneProxyConfig *obj){
	return obj->state == LinphoneRegistrationOk;
}

/**
 * Sets the proxy address
 *
 * Examples of valid sip proxy address are:
 * - IP address: sip:87.98.157.38
 * - IP address with port: sip:87.98.157.38:5062
 * - hostnames : sip:sip.example.net
**/
int linphone_proxy_config_set_server_addr(LinphoneProxyConfig *obj, const char *server_addr){
	LinphoneAddress *addr=NULL;
	char *modified=NULL;
	
	if (obj->reg_proxy!=NULL) ms_free(obj->reg_proxy);
	obj->reg_proxy=NULL;
	
	if (server_addr!=NULL && strlen(server_addr)>0){
		if (strstr(server_addr,"sip:")==NULL){
			modified=ms_strdup_printf("sip:%s",server_addr);
			addr=linphone_address_new(modified);
			ms_free(modified);
		}
		if (addr==NULL)
			addr=linphone_address_new(server_addr);
		if (addr){
			obj->reg_proxy=linphone_address_as_string(addr);
			linphone_address_destroy(addr);
		}else{
			ms_warning("Could not parse %s",server_addr);
			return -1;
		}
	}
	return 0;
}

/**
 * Sets the user identity as a SIP address.
 *
 * This identity is normally formed with display name, username and domain, such 
 * as:
 * Alice <sip:alice@example.net>
 * The REGISTER messages will have from and to set to this identity.
 *
**/
int linphone_proxy_config_set_identity(LinphoneProxyConfig *obj, const char *identity){
	LinphoneAddress *addr;
	if (identity!=NULL && strlen(identity)>0){
		addr=linphone_address_new(identity);
		if (!addr || linphone_address_get_username(addr)==NULL){
			ms_warning("Invalid sip identity: %s",identity);
			if (addr)
				linphone_address_destroy(addr);
			return -1;
		}else{
			if (obj->reg_identity!=NULL) {
				ms_free(obj->reg_identity);
				obj->reg_identity=NULL;
			}
			obj->reg_identity=ms_strdup(identity);
			if (obj->realm){
				ms_free(obj->realm);
			}
			obj->realm=ms_strdup(linphone_address_get_domain(addr));
			linphone_address_destroy(addr);
			return 0;
		}
	}
	return -1;
}

const char *linphone_proxy_config_get_domain(const LinphoneProxyConfig *cfg){
	return cfg->realm;
}

/**
 * Sets a SIP route.
 * When a route is set, all outgoing calls will go to the route's destination if this proxy
 * is the default one (see linphone_core_set_default_proxy() ).
**/
int linphone_proxy_config_set_route(LinphoneProxyConfig *obj, const char *route)
{
	if (obj->reg_route!=NULL){
		ms_free(obj->reg_route);
		obj->reg_route=NULL;
	}
	if (route!=NULL){
		SalAddress *addr;
		char *tmp;
		/*try to prepend 'sip:' */
		if (strstr(route,"sip:")==NULL){
			tmp=ms_strdup_printf("sip:%s",route);
		}else tmp=ms_strdup(route);
		addr=sal_address_new(tmp);
		if (addr!=NULL){
			sal_address_destroy(addr);
		}else{
			ms_free(tmp);
			tmp=NULL;
		}
		obj->reg_route=tmp;
	}
	return 0;
}

bool_t linphone_proxy_config_check(LinphoneCore *lc, LinphoneProxyConfig *obj){
	if (obj->reg_proxy==NULL){
		if (lc->vtable.display_warning)
			lc->vtable.display_warning(lc,_("The sip proxy address you entered is invalid, it must start with \"sip:\""
						" followed by a hostname."));
		return FALSE;
	}
	if (obj->reg_identity==NULL){
		if (lc->vtable.display_warning)
			lc->vtable.display_warning(lc,_("The sip identity you entered is invalid.\nIt should look like "
					"sip:username@proxydomain, such as sip:alice@example.net"));
		return FALSE;
	}
	return TRUE;
}

/**
 * Indicates whether a REGISTER request must be sent to the proxy.
**/
void linphone_proxy_config_enableregister(LinphoneProxyConfig *obj, bool_t val){
	obj->reg_sendregister=val;
}

/**
 * Sets the registration expiration time in seconds.
**/
void linphone_proxy_config_expires(LinphoneProxyConfig *obj, int val){
	if (val<0) val=600;
	obj->expires=val;
}

void linphone_proxy_config_enable_publish(LinphoneProxyConfig *obj, bool_t val){
	obj->publish=val;
}
/**
 * Starts editing a proxy configuration.
 *
 * Because proxy configuration must be consistent, applications MUST
 * call linphone_proxy_config_edit() before doing any attempts to modify
 * proxy configuration (such as identity, proxy address and so on).
 * Once the modifications are done, then the application must call
 * linphone_proxy_config_done() to commit the changes.
**/
void linphone_proxy_config_edit(LinphoneProxyConfig *obj){
	if (obj->reg_sendregister){
		/* unregister */
		if (obj->state == LinphoneRegistrationOk) {
			sal_unregister(obj->op);
		}
	}
}

void linphone_proxy_config_apply(LinphoneProxyConfig *obj,LinphoneCore *lc)
{
	obj->lc=lc;
	linphone_proxy_config_done(obj);
}
#ifndef USE_BELLESIP
static char *guess_contact_for_register(LinphoneProxyConfig *obj){
	LinphoneAddress *proxy=linphone_address_new(obj->reg_proxy);
	char *ret=NULL;
	const char *host;
	if (proxy==NULL) return NULL;
	host=linphone_address_get_domain (proxy);
	if (host!=NULL){
		char localip[LINPHONE_IPADDR_SIZE];
		char *tmp;
		LCSipTransports tr;
		LinphoneAddress *contact;
		
		linphone_core_get_local_ip(obj->lc,host,localip);
		contact=linphone_address_new(obj->reg_identity);
		linphone_address_set_domain (contact,localip);
		linphone_address_set_port_int(contact,linphone_core_get_sip_port(obj->lc));
		linphone_address_set_display_name(contact,NULL);
		
		linphone_core_get_sip_transports(obj->lc,&tr);
		if (tr.udp_port <= 0) {
			if (tr.tcp_port>0) {
				sal_address_set_param(contact,"transport","tcp");
			} else if (tr.tls_port>0) {
				sal_address_set_param(contact,"transport","tls");
			}
		}
		tmp=linphone_address_as_string_uri_only(contact);
		if (obj->contact_params)
			ret=ms_strdup_printf("<%s;%s>",tmp,obj->contact_params);
		else ret=ms_strdup_printf("<%s>",tmp);
		linphone_address_destroy(contact);
		ms_free(tmp);
	}
	linphone_address_destroy (proxy);
	return ret;
}
#endif
static void linphone_proxy_config_register(LinphoneProxyConfig *obj){
	if (obj->reg_sendregister){
#ifndef USE_BELLESIP
		char *contact;
#endif
		if (obj->op)
			sal_op_release(obj->op);
		obj->op=sal_op_new(obj->lc->sal);
#ifndef USE_BELLESIP /*contact is automatically guessed by belle-sip*/
		contact=guess_contact_for_register(obj);
		sal_op_set_contact(obj->op,contact);
		ms_free(contact);
#endif
		sal_op_set_user_pointer(obj->op,obj);
		if (sal_register(obj->op,obj->reg_proxy,obj->reg_identity,obj->expires)==0) {
			linphone_proxy_config_set_state(obj,LinphoneRegistrationProgress,"Registration in progress");
		} else {
			linphone_proxy_config_set_state(obj,LinphoneRegistrationFailed,"Registration failed");
		}
	}
}

/**
 * Refresh a proxy registration.
 * This is useful if for example you resuming from suspend, thus IP address may have changed.
**/
void linphone_proxy_config_refresh_register(LinphoneProxyConfig *obj){
	if (obj->reg_sendregister && obj->op){
		if (sal_register_refresh(obj->op,obj->expires) == 0) {
			linphone_proxy_config_set_state(obj,LinphoneRegistrationProgress, "Refresh registration");
		}
	}
}


/**
 * Sets a dialing prefix to be automatically prepended when inviting a number with 
 * linphone_core_invite();
 * This dialing prefix shall usually be the country code of the country where the user is living.
 *
**/
void linphone_proxy_config_set_dial_prefix(LinphoneProxyConfig *cfg, const char *prefix){
	if (cfg->dial_prefix!=NULL){
		ms_free(cfg->dial_prefix);
		cfg->dial_prefix=NULL;
	}
	if (prefix && prefix[0]!='\0') cfg->dial_prefix=ms_strdup(prefix);
}

/**
 * Returns dialing prefix.
 *
 * 
**/
const char *linphone_proxy_config_get_dial_prefix(const LinphoneProxyConfig *cfg){
	return cfg->dial_prefix;
}

/**
 * Sets whether liblinphone should replace "+" by international calling prefix in dialed numbers (passed to
 * #linphone_core_invite ).
 *
**/
void linphone_proxy_config_set_dial_escape_plus(LinphoneProxyConfig *cfg, bool_t val){
	cfg->dial_escape_plus=val;
}

/**
 * Returns whether liblinphone should replace "+" by "00" in dialed numbers (passed to
 * #linphone_core_invite ).
 *
**/
bool_t linphone_proxy_config_get_dial_escape_plus(const LinphoneProxyConfig *cfg){
	return cfg->dial_escape_plus;
}
/*
 * http://en.wikipedia.org/wiki/Telephone_numbering_plan
 * http://en.wikipedia.org/wiki/Telephone_numbers_in_Europe
 */
typedef struct dial_plan{
	const char *country;
	const char* iso_country_code; /* ISO 3166-1 alpha-2 code, ex: FR for France*/
	char  ccc[8]; /*country calling code*/
	int nnl; /*maximum national number length*/
	const char * icp; /*international call prefix, ex: 00 in europe*/
	
}dial_plan_t;

/* TODO: fill with information for all countries over the world*/

static dial_plan_t const dial_plans[]={
	{"Afghanistan"                  ,"AF"		, "93"      , 9		, "00"  },
	{"Albania"                      ,"AL"		, "355"     , 9		, "00"  },
	{"Algeria"                      ,"DZ"		, "213"     , 9		, "00"  },
	{"American Samoa"               ,"AS"		, "1"       , 10	, "011"	},
	{"Andorra"                      ,"AD"		, "376"     , 6		, "00"  },
	{"Angola"                       ,"AO"		, "244"     , 9		, "00"  },
	{"Anguilla"                     ,"AI"		, "1"       , 10	, "011" },
	{"Antigua and Barbuda"          ,"AG"		, "1"       , 10	, "011"	},
	{"Argentina"                    ,"AR"		, "54"      , 10	, "00"  },
	{"Armenia"                      ,"AM"		, "374"     , 8		, "00"  },
	{"Aruba"                        ,"AW"		, "297"     , 7		, "011"	},
	{"Australia"                    ,"AU"		, "61"      , 9	    , "0011"},
	{"Austria"                      ,"AT"		, "43"      , 10	, "00"  },
	{"Azerbaijan"                   ,"AZ"       , "994"     , 9		, "00"  },
	{"Bahamas"                      ,"BS"		, "1"       , 10    , "011"	},
	{"Bahrain"                      ,"BH"		, "973"     , 8     , "00"  },
	{"Bangladesh"                   ,"BD"		, "880"     , 10    , "00"  },
	{"Barbados"                     ,"BB"		, "1"       , 10    , "011"	},
    {"Belarus"                      ,"BY"		, "375"     , 9     , "00"  },
	{"Belgium"                      ,"BE"		, "32"      , 9     , "00"  },
	{"Belize"                       ,"BZ"		, "501"     , 7     , "00"  },
	{"Benin"                        ,"BJ"		, "229"     , 8     , "00"	},
	{"Bermuda"                      ,"BM"		, "1"       , 10    , "011" },
	{"Bhutan"                       ,"BT"		, "975"     , 8     , "00"  },
	{"Bolivia"                      ,"BO"		, "591"     , 8     , "00"	},
	{"Bosnia and Herzegovina"       ,"BA"		, "387"     , 8     , "00"  },
	{"Botswana"                     ,"BW"		, "267"     , 8     , "00"  },
    {"Brazil"                       ,"BR"		, "55"      , 10	, "00"  },
	{"Brunei Darussalam"            ,"BN"		, "673"     , 7		, "00"	},
	{"Bulgaria"                     ,"BG"		, "359"     , 9		, "00"  },
	{"Burkina Faso"                 ,"BF"		, "226"     , 8		, "00"  },
	{"Burundi"                      ,"BI"		, "257"     , 8     , "011" },
    {"Cambodia"                     ,"KH"		, "855"     , 9		, "00"  },
	{"Cameroon"                     ,"CM"		, "237"     , 8		, "00"  },
	{"Canada"                       ,"CA"		, "1"       , 10	, "011" },
	{"Cape Verde"                   ,"CV"		, "238"     , 7		, "00"	},
	{"Cayman Islands"               ,"KY"		, "1"       , 10	, "011" },
	{"Central African Republic"     ,"CF"		, "236"     , 8     , "00"  },
	{"Chad"                         ,"TD"		, "235"     , 8		, "00"	},
	{"Chile"                        ,"CL"		, "56"      , 9	    , "00"  },
	{"China"                        ,"CN"		, "86"      , 11	, "00"  },
    {"Colombia"                     ,"CO"       , "57"      , 10	, "00"  },
    {"Comoros"                      ,"KM"		, "269"     , 7     , "00"	},
    {"Congo"                        ,"CG"		, "242"     , 9		, "00"	},
    {"Congo Democratic Republic"	,"CD"		, "243"     , 9		, "00"  },
    {"Cook Islands"                 ,"CK"		, "682"     , 5		, "00"  },
    {"Costa Rica"                   ,"CR"		, "506"     , 8     , "00"	},
    {"C�te d'Ivoire"	            ,"AD"		, "225"     , 8     , "00"  },
    {"Croatia"                      ,"HR"		, "385"     , 9		, "00"  },
    {"Cuba"                         ,"CU"		, "53"      , 8     , "119" },
    {"Cyprus"                       ,"CY"		, "357"     , 8     , "00"	},
    {"Czech Republic"               ,"CZ"		, "420"     , 9     , "00"  },
    {"Denmark"                      ,"DK"		, "45"      , 8		, "00"  },
    {"Djibouti"                     ,"DJ"		, "253"     , 8		, "00"	},
    {"Dominica"                     ,"DM"		, "1"       , 10	, "011" },
    {"Dominican Republic"	        ,"DO"		, "1"       , 10	, "011" },
    {"Ecuador"                      ,"EC"       , "593"     , 9		, "00"  },
    {"Egypt"                        ,"EG"		, "20"      , 10	, "00"	},
    {"El Salvador"                  ,"SV"		, "503"     , 8		, "00"	},
    {"Equatorial Guinea"            ,"GQ"		, "240"     , 9		, "00"  },
    {"Eritrea"                      ,"ER"		, "291"     , 7		, "00"  },
    {"Estonia"                      ,"EE"		, "372"     , 8     , "00"	},
    {"Ethiopia"                     ,"ET"		, "251"     , 9     , "00"  },
    {"Falkland Islands"	            ,"FK"		, "500"     , 5		, "00"  },
    {"Faroe Islands"	            ,"FO"		, "298"     , 6     , "00"  },
    {"Fiji"                         ,"FJ"		, "679"     , 7     , "00"	},
    {"Finland"                      ,"FI"		, "358"     , 9     , "00"  },
    {"France"                       ,"FR"		, "33"      , 9		, "00"	},
    {"French Guiana"				,"GF"		, "594"     , 9		, "00"	},
    {"French Polynesia"             ,"PF"		, "689"     , 6	    , "00"  },
    {"Gabon"                        ,"GA"		, "241"     , 8     , "00"  },
    {"Gambia"                       ,"GM"       , "220"     , 7		, "00"  },
    {"Georgia"                      ,"GE"		, "995"     , 9     , "00"	},
    {"Germany"                      ,"DE"		, "49"      , 11	, "00"	},
    {"Ghana"                        ,"GH"		, "233"     , 9		, "00"  },
    {"Gibraltar"                    ,"GI"		, "350"     , 8		, "00"  },
    {"Greece"                       ,"GR"		, "30"      ,10     , "00"	},
    {"Greenland"                    ,"GL"		, "299"     , 6		, "00"  },
    {"Grenada"                      ,"GD"		, "1"       , 10	, "011" },
    {"Guadeloupe"                   ,"GP"		, "590"     , 9     , "00"  },
    {"Guam"                         ,"GU"		, "1"       , 10	, "011"	},
    {"Guatemala"                    ,"GT"		, "502"     , 8     , "00"  },
    {"Guinea"                       ,"GN"		, "224"     , 8		, "00"  },
    {"Guinea-Bissau"				,"GW"		, "245"     , 7		, "00"	},
    {"Guyana"                       ,"GY"		, "592"     , 7	    , "001" },
    {"Haiti"                        ,"HT"		, "509"     , 8     , "00"  },
    {"Honduras"                     ,"HN"       , "504"     , 8		, "00"  },
    {"Hong Kong"                    ,"HK"		, "852"     , 8     , "001"	},
    {"Hungary"                      ,"HU"		, "36"      , 9     , "00"  },
    {"Iceland"                      ,"IS"		, "354"     , 9     , "00"  },
    {"India"                        ,"IN"		, "91"      , 10    , "00"  },
    {"Indonesia"                    ,"ID"		, "62"      , 10	, "001"	},
    {"Iran"                         ,"IR"		, "98"      , 10	, "00"	},
    {"Iraq"                         ,"IQ"		, "964"     , 10	, "00"  },
    {"Ireland"                      ,"IE"		, "353"     , 9		, "00"  },
    {"Israel"                       ,"IL"		, "972"     , 9     , "00"	},
    {"Italy"                        ,"IT"		, "39"      , 10	, "00"  },
    {"Jamaica"                      ,"JM"		, "1"       , 10	, "011" },
    {"Japan"                        ,"JP"		, "81"      , 10	, "010" },
    {"Jordan"                       ,"JO"		, "962"     , 9     , "00"	},
    {"Kazakhstan"                   ,"KZ"		, "7"       , 10    , "00"  },
    {"Kenya"                        ,"KE"		, "254"     , 9		, "000" },
    {"Kiribati"                     ,"KI"		, "686"     , 5		, "00"	},
    {"Korea, North"                 ,"KP"		, "850"     , 12	, "99"  },
    {"Korea, South"                 ,"KR"       , "82"      , 12	, "001" },
    {"Kuwait"                       ,"KW"		, "965"     , 8     , "00"	},
    {"Kyrgyzstan"                   ,"KG"		, "996"     , 9     , "00"  },
    {"Laos"                         ,"LA"		, "856"     , 10    , "00"  },
    {"Latvia"                       ,"LV"		, "371"     , 8     , "00"	},
    {"Lebanon"                      ,"LB"		, "961"     , 7     , "00"	},
    {"Lesotho"                      ,"LS"		, "266"     , 8		, "00"	},
    {"Liberia"                      ,"LR"		, "231"     , 8		, "00"  },
    {"Libya"                        ,"LY"		, "218"     , 8		, "00"  },
    {"Liechtenstein"                ,"LI"		, "423"     , 7     , "00"	},
    {"Lithuania"                    ,"LT"		, "370"     , 8		, "00"  },
    {"Luxembourg"                   ,"LU"		, "352"     , 9		, "00"  },
    {"Macau"                        ,"MO"		, "853"     , 8     , "00"  },
    {"Macedonia"                    ,"MK"		, "389"     , 8     , "00"	},
    {"Madagascar"                   ,"MG"		, "261"     , 9     , "00"  },
    {"Malawi"                       ,"MW"		, "265"     , 9		, "00"  },
    {"Malaysia"                     ,"MY"		, "60"      , 9		, "00"	},
    {"Maldives"                     ,"MV"		, "960"     , 7	    , "00"  },
    {"Mali"                         ,"ML"		, "223"     , 8     , "00"  },
    {"Malta"                        ,"MT"       , "356"     , 8		, "00"  },
    {"Marshall Islands"				,"MH"		, "692"     , 7     , "011"	},
    {"Martinique"                   ,"MQ"		, "596"     , 9     , "00"  },
    {"Mauritania"                   ,"MR"		, "222"     , 8     , "00"  },
    {"Mauritius"                    ,"MU"		, "230"     , 7     , "00"	},
    {"Mayotte Island"               ,"YT"		, "262"     , 9     , "00"	},
    {"Mexico"                       ,"MX"		, "52"      , 10	, "00"	},
    {"Micronesia"                   ,"FM"		, "691"     , 7		, "011" },
    {"Moldova"                      ,"MD"		, "373"     , 8		, "00"  },
    {"Monaco"                       ,"MC"		, "377"     , 8     , "00"	},
    {"Mongolia"                     ,"MN"		, "976"     , 8     , "001" },
    {"Montenegro"                   ,"ME"		, "382"     , 8		, "00"  },
    {"Montserrat"                   ,"MS"		, "664"     , 10	, "011" },
    {"Morocco"                      ,"MA"		, "212"     , 9     , "00"	},
    {"Mozambique"                   ,"MZ"		, "258"     , 9     , "00"  },
    {"Myanmar"                      ,"MM"		, "95"      , 8		, "00"  },
    {"Namibia"                      ,"NA"		, "264"     , 9		, "00"	},
    {"Nauru"                        ,"NR"		, "674"     , 7	    , "00"  },
    {"Nepal"                        ,"NP"		, "43"      , 10	, "00"  },
    {"Netherlands"                  ,"NL"       , "31"      , 9		, "00"  },
    {"New Caledonia"				,"NC"		, "687"     , 6     , "00"	},
    {"New Zealand"                  ,"NZ"		, "64"      , 10	, "00"  },
    {"Nicaragua"                    ,"NI"		, "505"     , 8     , "00"  },
    {"Niger"                        ,"NE"		, "227"     , 8     , "00"	},
    {"Nigeria"                      ,"NG"		, "234"     , 10	, "009"	},
    {"Niue"                         ,"NU"		, "683"     , 4		, "00"	},
    {"Norfolk Island"	            ,"NF"		, "672"     , 5		, "00"  },
    {"Northern Mariana Islands"	    ,"MP"		, "1"       , 10	, "011" },
    {"Norway"                       ,"NO"		, "47"      , 8     , "00"	},
    {"Oman"                         ,"OM"		, "968"     , 8		, "00"  },
    {"Pakistan"                     ,"PK"		, "92"      , 10	, "00"  },
    {"Palau"                        ,"PW"		, "680"     , 7     , "011" },
    {"Palestine"                    ,"PS"		, "970"     , 9     , "00"	},
    {"Panama"                       ,"PA"		, "507"     , 8     , "00"  },
    {"Papua New Guinea"	            ,"PG"		, "675"     , 8		, "00"  },
    {"Paraguay"                     ,"PY"		, "595"     , 9		, "00"	},
    {"Peru"                         ,"PE"		, "51"      , 9	    , "00"  },
    {"Philippines"                  ,"PH"		, "63"      , 10	, "00"  },
    {"Poland"                       ,"PL"       , "48"      , 9		, "00"  },
    {"Portugal"                     ,"PT"		, "351"     , 9     , "00"	},
    {"Puerto Rico"                  ,"PR"		, "1"       , 10	, "011" },
    {"Qatar"                        ,"QA"		, "974"     , 8     , "00"  },
    {"R�union Island"				,"RE"		, "262"     , 9     , "011"	},
    {"Romania"                      ,"RO"		, "40"      , 9     , "00"	},
    {"Russian Federation"           ,"RU"		, "7"       , 10	, "8"	},
    {"Rwanda"                       ,"RW"		, "250"     , 9		, "00"  },
    {"Saint Helena"                 ,"SH"		, "290"     , 4		, "00"  },
    {"Saint Kitts and Nevis"		,"KN"		, "1"       , 10	, "011"	},
    {"Saint Lucia"                  ,"LC"		, "1"       , 10	, "011" },
    {"Saint Pierre and Miquelon"    ,"PM"		, "508"     , 6		, "00"  },
    {"Saint Vincent and the Grenadines","VC"	, "1"       , 10	, "011" },
    {"Samoa"                        ,"WS"		, "685"     , 7     , "0"	},
    {"San Marino"                   ,"SM"		, "378"     , 10	, "00"  },
    {"S�o Tom� and Pr�ncipe"        ,"ST"		, "239"     , 7		, "00"  },
    {"Saudi Arabia"                 ,"SA"		, "966"     , 9		, "00"	},
    {"Senegal"                      ,"SN"		, "221"     , 9	    , "00"  },
    {"Serbia"                       ,"RS"		, "381"     , 9     , "00"  },
    {"Seychelles"                   ,"SC"       , "248"     , 7		, "00"  },
    {"Sierra Leone"                 ,"SL"		, "232"     , 8     , "00"	},
    {"Singapore"                    ,"SG"		, "65"      , 8     , "001" },
    {"Slovakia"                     ,"SK"		, "421"     , 9     , "00"  },
    {"Slovenia"                     ,"SI"		, "386"     , 8     , "00"	},
    {"Solomon Islands"              ,"SB"		, "677"     , 7     , "00"	},
    {"Somalia"                      ,"SO"		, "252"     , 8		, "00"	},
    {"South Africa"                 ,"ZA"		, "27"      , 9		, "00"  },
    {"Spain"                        ,"ES"		, "34"      , 9		, "00"  },
    {"Sri Lanka"                    ,"LK"		, "94"      , 9     , "00"	},
    {"Sudan"                        ,"SD"		, "249"     , 9		, "00"  },
    {"Suriname"                     ,"SR"		, "597"     , 7		, "00"  },
    {"Swaziland"                    ,"SZ"		, "268"     , 8     , "00"  },
    {"Sweden"                       ,"SE"		, "1"       , 9     , "00"	},
    {"Switzerland"                  ,"XK"		, "41"      , 9		, "00"	},
    {"Syria"                        ,"SY"		, "963"     , 9		, "00"  },
    {"Taiwan"                       ,"TW"		, "886"     , 9		, "810"	},
    {"Tajikistan"                   ,"TJ"		, "992"     , 9	    , "002" },
    {"Tanzania"                     ,"TZ"		, "255"     , 9     , "000" },
    {"Thailand"                     ,"TH"       , "66"      , 9		, "001" },
    {"Togo"                         ,"TG"		, "228"     , 8     , "00"	},
    {"Tokelau"                      ,"TK"		, "690"     , 4     , "00"  },
    {"Tonga"                        ,"TO"		, "676"     , 5     , "00"  },
    {"Trinidad and Tobago"			,"TT"		, "1"       , 10    , "011"	},
    {"Tunisia"                      ,"TN"		, "216"     , 8     , "00"	},
    {"Turkey"                       ,"TR"		, "90"      , 10	, "00"	},
    {"Turkmenistan"                 ,"TM"		, "993"     , 8		, "00"  },
    {"Turks and Caicos Islands"	    ,"TC"		, "1"       , 7		, "0"   },
    {"Tuvalu"                       ,"TV"		, "688"     , 5     , "00"	},
    {"Uganda"                       ,"UG"		, "256"     , 9     , "000" },
    {"Ukraine"                      ,"UA"		, "380"     , 9		, "00"  },
    {"United Arab Emirates"	        ,"AE"		, "971"     , 9     , "00"  },
    {"United Kingdom"               ,"UK"		, "44"      , 10	, "00"	},
    {"United States"                ,"US"		, "1"       , 10	, "011" },
    {"Uruguay"                      ,"UY"		, "598"     , 8		, "00"  },
    {"Uzbekistan"                   ,"UZ"		, "998"     , 9		, "8"	},
    {"Vanuatu"                      ,"VU"		, "678"     , 7	    , "00"  },
    {"Venezuela"                    ,"VE"		, "58"      , 10	, "00"  },
    {"Vietnam"                      ,"VN"		, "84"      , 9     , "00"  },
    {"Wallis and Futuna"	        ,"WF"		, "681"     , 5		, "00"  },
    {"Yemen"                        ,"YE"		, "967"     , 9     , "00"  },
    {"Zambia"                       ,"ZM"		, "260"     , 9     , "00"	},
    {"Zimbabwe"                     ,"ZW"		, "263"     , 9     , "00"  },
	{NULL                           ,NULL       ,  ""       , 0     , NULL	}
};
static dial_plan_t most_common_dialplan={ "generic" ,"", "", 10, "00"};

int linphone_dial_plan_lookup_ccc_from_e164(const char* e164) {
	dial_plan_t* dial_plan;
	dial_plan_t* elected_dial_plan=NULL;
	unsigned int found;
	unsigned int i=0;
	if (e164[1]=='1') {
		/*USA case*/
		return 1;
	}
	do {
		found=0;
		i++;
		for (dial_plan=(dial_plan_t*)dial_plans; dial_plan->country!=NULL; dial_plan++) {
			if (strncmp(dial_plan->ccc,&e164[1],i) == 0) {
				elected_dial_plan=dial_plan;
				found++;
			}
		}
	} while ((found>1 || found==0) && i < sizeof(dial_plan->ccc));
	if (found==1) {
		return atoi(elected_dial_plan->ccc);
	} else {
		return -1; /*not found */
	}

}
int linphone_dial_plan_lookup_ccc_from_iso(const char* iso) {
	dial_plan_t* dial_plan;
	for (dial_plan=(dial_plan_t*)dial_plans; dial_plan->country!=NULL; dial_plan++) {
		if (strcmp(iso, dial_plan->iso_country_code)==0) {
			return atoi(dial_plan->ccc);
		}
	}
	return -1;
}

static void lookup_dial_plan(const char *ccc, dial_plan_t *plan){
	int i;
	for(i=0;dial_plans[i].country!=NULL;++i){
		if (strcmp(ccc,dial_plans[i].ccc)==0){
			*plan=dial_plans[i];
			return;
		}
	}
	/*else return a generic "most common" dial plan*/
	*plan=most_common_dialplan;
	strcpy(plan->ccc,ccc);
}

static bool_t is_a_phone_number(const char *username){
	const char *p;
	for(p=username;*p!='\0';++p){
		if (isdigit(*p) || 
		    *p==' ' ||
		    *p=='.' ||
		    *p=='-' ||
		    *p==')' ||
			*p=='(' ||
			*p=='/' ||
			*p=='+') continue;
		else return FALSE;
	}
	return TRUE;
}

static char *flatten_number(const char *number){
	char *result=ms_malloc0(strlen(number)+1);
	char *w=result;
	const char *r;
	for(r=number;*r!='\0';++r){
		if (*r=='+' || isdigit(*r)){
			*w++=*r;
		}
	}
	*w++='\0';
	return result;
}

static void replace_plus(const char *src, char *dest, size_t destlen, const char *icp){
	int i=0;
	
	if (icp && src[0]=='+' && (destlen>(i=strlen(icp))) ){
		src++;
		strcpy(dest,icp);
	}
	
	for(;(i<destlen-1) && *src!='\0';++i){
		dest[i]=*src;
		src++;
	}
	dest[i]='\0';
}


int linphone_proxy_config_normalize_number(LinphoneProxyConfig *proxy, const char *username, char *result, size_t result_len){
	int numlen;
	if (is_a_phone_number(username)){
		char *flatten;
		flatten=flatten_number(username);
		ms_message("Flattened number is '%s'",flatten);
		
		if (proxy->dial_prefix==NULL || proxy->dial_prefix[0]=='\0'){
			/*no prefix configured, nothing else to do*/
			strncpy(result,flatten,result_len);
			ms_free(flatten);
			return 0;
		}else{
			dial_plan_t dialplan;
			lookup_dial_plan(proxy->dial_prefix,&dialplan);
			ms_message("Using dialplan '%s'",dialplan.country);
			if (flatten[0]=='+' || strstr(flatten,dialplan.icp)==flatten){
				/* the number has international prefix or +, so nothing to do*/
				ms_message("Prefix already present.");
				/*eventually replace the plus*/
				replace_plus(flatten,result,result_len,proxy->dial_escape_plus ? dialplan.icp : NULL);
				ms_free(flatten);
				return 0;
			}else{
				int i=0;
				int skip;
				numlen=strlen(flatten);
				/*keep at most national number significant digits */
				skip=numlen-dialplan.nnl;
				if (skip<0) skip=0;
				/*first prepend internation calling prefix or +*/
				if (proxy->dial_escape_plus){
					strncpy(result,dialplan.icp,result_len);
					i+=strlen(dialplan.icp);
				}else{
					strncpy(result,"+",result_len);
					i+=1;
				}
				/*add prefix*/
				if (result_len-i>strlen(dialplan.ccc)){
					strcpy(result+i,dialplan.ccc);
					i+=strlen(dialplan.ccc);
				}
				/*add user digits */
				strncpy(result+i,flatten+skip,result_len-i-1);
				ms_free(flatten);
			}
		}
	}else strncpy(result,username,result_len);
	return 0;
}

/**
 * Commits modification made to the proxy configuration.
**/
int linphone_proxy_config_done(LinphoneProxyConfig *obj)
{
	if (!linphone_proxy_config_check(obj->lc,obj)) return -1;
	obj->commit=TRUE;
	linphone_proxy_config_write_all_to_config_file(obj->lc);
	return 0;
}

void linphone_proxy_config_set_realm(LinphoneProxyConfig *cfg, const char *realm)
{
	if (cfg->realm!=NULL) {
		ms_free(cfg->realm);
		cfg->realm=NULL;
	}
	if (realm!=NULL) cfg->realm=ms_strdup(realm);
}

int linphone_proxy_config_send_publish(LinphoneProxyConfig *proxy,
			       LinphoneOnlineStatus presence_mode){
	int err;
	SalOp *op=sal_op_new(proxy->lc->sal);
	err=sal_publish(op,linphone_proxy_config_get_identity(proxy),
	    linphone_proxy_config_get_identity(proxy),linphone_online_status_to_sal(presence_mode));
	if (proxy->publish_op!=NULL)
		sal_op_release(proxy->publish_op);
	proxy->publish_op=op;
	return err;
}

/**
 * Returns the route set for this proxy configuration.
**/
const char *linphone_proxy_config_get_route(const LinphoneProxyConfig *obj){
	return obj->reg_route;
}

/**
 * Returns the SIP identity that belongs to this proxy configuration.
 *
 * The SIP identity is a SIP address (Display Name <sip:username@@domain> )
**/
const char *linphone_proxy_config_get_identity(const LinphoneProxyConfig *obj){
	return obj->reg_identity;
}

/**
 * Returns TRUE if PUBLISH request is enabled for this proxy.
**/
bool_t linphone_proxy_config_publish_enabled(const LinphoneProxyConfig *obj){
	return obj->publish;
}

/**
 * Returns the proxy's SIP address.
**/
const char *linphone_proxy_config_get_addr(const LinphoneProxyConfig *obj){
	return obj->reg_proxy;
}

/**
 * Returns the duration of registration.
**/
int linphone_proxy_config_get_expires(const LinphoneProxyConfig *obj){
	return obj->expires;
}

/**
 * Returns TRUE if registration to the proxy is enabled.
**/
bool_t linphone_proxy_config_register_enabled(const LinphoneProxyConfig *obj){
	return obj->reg_sendregister;
}

/**
 * Set optional contact parameters that will be added to the contact information sent in the registration.
 * @param obj the proxy config object
 * @param contact_params a string contaning the additional parameters in text form, like "myparam=something;myparam2=something_else"
 *
 * The main use case for this function is provide the proxy additional information regarding the user agent, like for example unique identifier or apple push id.
 * As an example, the contact address in the SIP register sent will look like <sip:joe@15.128.128.93:50421;apple-push-id=43143-DFE23F-2323-FA2232>.
**/
void linphone_proxy_config_set_contact_parameters(LinphoneProxyConfig *obj, const char *contact_params){
	if (obj->contact_params) {
		ms_free(obj->contact_params);
		obj->contact_params=NULL;
	}
	if (contact_params){
		obj->contact_params=ms_strdup(contact_params);
	}
}

/**
 * Returns previously set contact parameters.
**/
const char *linphone_proxy_config_get_contact_parameters(const LinphoneProxyConfig *obj){
	return obj->contact_params;
}

struct _LinphoneCore * linphone_proxy_config_get_core(const LinphoneProxyConfig *obj){
	return obj->lc;
}

/**
 * Add a proxy configuration.
 * This will start registration on the proxy, if registration is enabled.
**/
int linphone_core_add_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	if (!linphone_proxy_config_check(lc,cfg)) {
		return -1;
	}
	if (ms_list_find(lc->sip_conf.proxies,cfg)!=NULL){
		ms_warning("ProxyConfig already entered, ignored.");
		return 0;
	}
	lc->sip_conf.proxies=ms_list_append(lc->sip_conf.proxies,(void *)cfg);
	linphone_proxy_config_apply(cfg,lc);
	return 0;
}

/**
 * Removes a proxy configuration.
 *
 * LinphoneCore will then automatically unregister and place the proxy configuration
 * on a deleted list. For that reason, a removed proxy does NOT need to be freed.
**/
void linphone_core_remove_proxy_config(LinphoneCore *lc, LinphoneProxyConfig *cfg){
	/* check this proxy config is in the list before doing more*/
	if (ms_list_find(lc->sip_conf.proxies,cfg)==NULL){
		ms_error("linphone_core_remove_proxy_config: LinphoneProxyConfig %p is not known by LinphoneCore (programming error?)",cfg);
		return;
	}
	lc->sip_conf.proxies=ms_list_remove(lc->sip_conf.proxies,(void *)cfg);
	/* add to the list of destroyed proxies, so that the possible unREGISTER request can succeed authentication */
	lc->sip_conf.deleted_proxies=ms_list_append(lc->sip_conf.deleted_proxies,(void *)cfg);
	cfg->deletion_date=ms_time(NULL);
	if (cfg->state==LinphoneRegistrationOk){
		/* this will unREGISTER */
		linphone_proxy_config_edit(cfg);
	}
	if (lc->default_proxy==cfg){
		lc->default_proxy=NULL;
	}
	linphone_proxy_config_write_all_to_config_file(lc);
}
/**
 * Erase all proxies from config.
 *
 * @ingroup proxy
**/
void linphone_core_clear_proxy_config(LinphoneCore *lc){
	MSList* list=ms_list_copy(linphone_core_get_proxy_config_list((const LinphoneCore*)lc));
	MSList* copy=list;
	for(;list!=NULL;list=list->next){
		linphone_core_remove_proxy_config(lc,(LinphoneProxyConfig *)list->data);
	}
	ms_list_free(copy);
	linphone_proxy_config_write_all_to_config_file(lc);
}
/**
 * Sets the default proxy.
 *
 * This default proxy must be part of the list of already entered LinphoneProxyConfig.
 * Toggling it as default will make LinphoneCore use the identity associated with
 * the proxy configuration in all incoming and outgoing calls.
**/
void linphone_core_set_default_proxy(LinphoneCore *lc, LinphoneProxyConfig *config){
	/* check if this proxy is in our list */
	if (config!=NULL){
		if (ms_list_find(lc->sip_conf.proxies,config)==NULL){
			ms_warning("Bad proxy address: it is not in the list !");
			lc->default_proxy=NULL;
			return ;
		}
	}
	lc->default_proxy=config;
	if (linphone_core_ready(lc))
		lp_config_set_int(lc->config,"sip","default_proxy",linphone_core_get_default_proxy(lc,NULL));
}	

void linphone_core_set_default_proxy_index(LinphoneCore *lc, int index){
	if (index<0) linphone_core_set_default_proxy(lc,NULL);
	else linphone_core_set_default_proxy(lc,ms_list_nth_data(lc->sip_conf.proxies,index));
}

/**
 * Returns the default proxy configuration, that is the one used to determine the current identity.
**/
int linphone_core_get_default_proxy(LinphoneCore *lc, LinphoneProxyConfig **config){
	int pos=-1;
	if (config!=NULL) *config=lc->default_proxy;
	if (lc->default_proxy!=NULL){
		pos=ms_list_position(lc->sip_conf.proxies,ms_list_find(lc->sip_conf.proxies,(void *)lc->default_proxy));
	}
	return pos;
}

/**
 * Returns an unmodifiable list of entered proxy configurations.
**/
const MSList *linphone_core_get_proxy_config_list(const LinphoneCore *lc){
	return lc->sip_conf.proxies;
}

void linphone_proxy_config_write_to_config_file(LpConfig *config, LinphoneProxyConfig *obj, int index)
{
	char key[50];

	sprintf(key,"proxy_%i",index);
	lp_config_clean_section(config,key);
	if (obj==NULL){
		return;
	}
	if (obj->type!=NULL){
		lp_config_set_string(config,key,"type",obj->type);
	}
	if (obj->reg_proxy!=NULL){
		lp_config_set_string(config,key,"reg_proxy",obj->reg_proxy);
	}
	if (obj->reg_route!=NULL){
		lp_config_set_string(config,key,"reg_route",obj->reg_route);
	}
	if (obj->reg_identity!=NULL){
		lp_config_set_string(config,key,"reg_identity",obj->reg_identity);
	}
	if (obj->contact_params!=NULL){
		lp_config_set_string(config,key,"contact_parameters",obj->contact_params);
	}
	lp_config_set_int(config,key,"reg_expires",obj->expires);
	lp_config_set_int(config,key,"reg_sendregister",obj->reg_sendregister);
	lp_config_set_int(config,key,"publish",obj->publish);
	lp_config_set_int(config,key,"dial_escape_plus",obj->dial_escape_plus);
	lp_config_set_string(config,key,"dial_prefix",obj->dial_prefix);
}



LinphoneProxyConfig *linphone_proxy_config_new_from_config_file(LpConfig *config, int index)
{
	const char *tmp;
	const char *identity;
	const char *proxy;
	LinphoneProxyConfig *cfg;
	char key[50];
	
	sprintf(key,"proxy_%i",index);

	if (!lp_config_has_section(config,key)){
		return NULL;
	}

	cfg=linphone_proxy_config_new();

	identity=lp_config_get_string(config,key,"reg_identity",NULL);	
	proxy=lp_config_get_string(config,key,"reg_proxy",NULL);
	
	linphone_proxy_config_set_identity(cfg,identity);
	linphone_proxy_config_set_server_addr(cfg,proxy);
	
	tmp=lp_config_get_string(config,key,"reg_route",NULL);
	if (tmp!=NULL) linphone_proxy_config_set_route(cfg,tmp);

	linphone_proxy_config_set_contact_parameters(cfg,lp_config_get_string(config,key,"contact_parameters",NULL));
	
	linphone_proxy_config_expires(cfg,lp_config_get_int(config,key,"reg_expires",LP_CONFIG_DEFAULT_INT(config,"reg_expires",600)));
	linphone_proxy_config_enableregister(cfg,lp_config_get_int(config,key,"reg_sendregister",0));
	
	linphone_proxy_config_enable_publish(cfg,lp_config_get_int(config,key,"publish",0));

	linphone_proxy_config_set_dial_escape_plus(cfg,lp_config_get_int(config,key,"dial_escape_plus",LP_CONFIG_DEFAULT_INT(config,"dial_escape_plus",0)));
	linphone_proxy_config_set_dial_prefix(cfg,lp_config_get_string(config,key,"dial_prefix",LP_CONFIG_DEFAULT_STRING(config,"dial_prefix",NULL)));
	
	tmp=lp_config_get_string(config,key,"type",NULL);
	if (tmp!=NULL && strlen(tmp)>0) 
		linphone_proxy_config_set_sip_setup(cfg,tmp);

	return cfg;
}

static void linphone_proxy_config_activate_sip_setup(LinphoneProxyConfig *cfg){
	SipSetupContext *ssc;
	SipSetup *ss=sip_setup_lookup(cfg->type);
	LinphoneCore *lc=linphone_proxy_config_get_core(cfg);
	unsigned int caps;
	if (!ss) return ;
	ssc=sip_setup_context_new(ss,cfg);
	cfg->ssctx=ssc;
	if (cfg->reg_identity==NULL){
		ms_error("Invalid identity for this proxy configuration.");
		return;
	}
	caps=sip_setup_context_get_capabilities(ssc);
	if (caps & SIP_SETUP_CAP_ACCOUNT_MANAGER){
		if (sip_setup_context_login_account(ssc,cfg->reg_identity,NULL)!=0){
			if (lc->vtable.display_warning){
				char *tmp=ms_strdup_printf(_("Could not login as %s"),cfg->reg_identity);
				lc->vtable.display_warning(lc,tmp);
				ms_free(tmp);
			}
			return;
		}
	}
	if (caps & SIP_SETUP_CAP_PROXY_PROVIDER){
		char proxy[256];
		if (sip_setup_context_get_proxy(ssc,NULL,proxy,sizeof(proxy))==0){
			linphone_proxy_config_set_server_addr(cfg,proxy);
		}else{
			ms_error("Could not retrieve proxy uri !");
		}
	}
	
}

SipSetup *linphone_proxy_config_get_sip_setup(LinphoneProxyConfig *cfg){
	if (cfg->ssctx!=NULL) return cfg->ssctx->funcs;
	if (cfg->type!=NULL){
		return sip_setup_lookup(cfg->type);
	}
	return NULL;
}

void linphone_proxy_config_update(LinphoneProxyConfig *cfg){
	LinphoneCore *lc=cfg->lc;
	if (cfg->commit){
		if (cfg->type && cfg->ssctx==NULL){
			linphone_proxy_config_activate_sip_setup(cfg);
		}
		if ((!lc->sip_conf.register_only_when_network_is_up || lc->network_reachable) &&
			(linphone_core_get_firewall_policy(lc)!=LinphonePolicyUseUpnp
					|| !lc->sip_conf.register_only_when_upnp_is_ok
					|| linphone_core_get_upnp_state(lc) == LinphoneUpnpStateOk))
			linphone_proxy_config_register(cfg);
		if (cfg->publish && cfg->publish_op==NULL){
			linphone_proxy_config_send_publish(cfg,lc->presence_mode);
		}
		cfg->commit=FALSE;
	}
}

void linphone_proxy_config_set_sip_setup(LinphoneProxyConfig *cfg, const char *type){
	if (cfg->type)
		ms_free(cfg->type);
	cfg->type=ms_strdup(type);
	if (linphone_proxy_config_get_addr(cfg)==NULL){
		/*put a placeholder so that the sip setup gets saved into the config */
		linphone_proxy_config_set_server_addr(cfg,"sip:undefined");
	}
}

SipSetupContext *linphone_proxy_config_get_sip_setup_context(LinphoneProxyConfig *cfg){
	return cfg->ssctx;
}

/**
 * @}
**/

LinphoneAccountCreator *linphone_account_creator_new(struct _LinphoneCore *core, const char *type){
	LinphoneAccountCreator *obj;
	LinphoneProxyConfig *cfg;
	SipSetup *ss=sip_setup_lookup(type);
	SipSetupContext *ssctx;
	if (!ss){
		return NULL;
	}
	if (!(sip_setup_get_capabilities(ss) & SIP_SETUP_CAP_ACCOUNT_MANAGER)){
		ms_error("%s cannot manage accounts.",type);
		return NULL;
	}
	obj=ms_new0(LinphoneAccountCreator,1);
	cfg=linphone_proxy_config_new();
	ssctx=sip_setup_context_new(ss,cfg);
	obj->lc=core;
	obj->ssctx=ssctx;
	set_string(&obj->domain,sip_setup_context_get_domains(ssctx)[0]);
	cfg->lc=core;
	return obj;
}

void linphone_account_creator_set_username(LinphoneAccountCreator *obj, const char *username){
	set_string(&obj->username,username);
}

void linphone_account_creator_set_password(LinphoneAccountCreator *obj, const char *password){
	set_string(&obj->password,password);
}

void linphone_account_creator_set_domain(LinphoneAccountCreator *obj, const char *domain){
	set_string(&obj->domain,domain);
}

void linphone_account_creator_set_route(LinphoneAccountCreator *obj, const char *route) {
	set_string(&obj->route,route);
}

void linphone_account_creator_set_email(LinphoneAccountCreator *obj, const char *email) {
	set_string(&obj->email,email);
}

void linphone_account_creator_set_suscribe(LinphoneAccountCreator *obj, int suscribe) {
	obj->suscribe = suscribe;
}

const char * linphone_account_creator_get_username(LinphoneAccountCreator *obj){
	return obj->username;
}

const char * linphone_account_creator_get_domain(LinphoneAccountCreator *obj){
	return obj->domain;
}

int linphone_account_creator_test_existence(LinphoneAccountCreator *obj){
	SipSetupContext *ssctx=obj->ssctx;
	char *uri=ms_strdup_printf("%s@%s",obj->username,obj->domain);
	int err=sip_setup_context_account_exists(ssctx,uri);
	ms_free(uri);
	return err;
}

int linphone_account_creator_test_validation(LinphoneAccountCreator *obj) {
	SipSetupContext *ssctx=obj->ssctx;
	int err=sip_setup_context_account_validated(ssctx,obj->username);
	return err;
}

LinphoneProxyConfig * linphone_account_creator_validate(LinphoneAccountCreator *obj){
	SipSetupContext *ssctx=obj->ssctx;
	char *uri=ms_strdup_printf("%s@%s",obj->username,obj->domain);
	int err=sip_setup_context_create_account(ssctx, uri, obj->password, obj->email, obj->suscribe);
	ms_free(uri);
	if (err==0) {
		obj->succeeded=TRUE;
		return sip_setup_context_get_proxy_config(ssctx);
	}
	return NULL;
}

void linphone_account_creator_destroy(LinphoneAccountCreator *obj){
	if (obj->username)
		ms_free(obj->username);
	if (obj->password)
		ms_free(obj->password);
	if (obj->domain)
		ms_free(obj->domain);
	if (!obj->succeeded){
		linphone_proxy_config_destroy(sip_setup_context_get_proxy_config(obj->ssctx));
	}
}

void linphone_proxy_config_set_user_data(LinphoneProxyConfig *cr, void * ud) {
	cr->user_data=ud;
}

void * linphone_proxy_config_get_user_data(LinphoneProxyConfig *cr) {
	return cr->user_data;
}

void linphone_proxy_config_set_state(LinphoneProxyConfig *cfg, LinphoneRegistrationState state, const char *message){
	LinphoneCore *lc=cfg->lc;
	cfg->state=state;
	if (lc && lc->vtable.registration_state_changed){
		lc->vtable.registration_state_changed(lc,cfg,state,message);
	}
}

LinphoneRegistrationState linphone_proxy_config_get_state(const LinphoneProxyConfig *cfg){
	return cfg->state;
}

 const char *linphone_registration_state_to_string(LinphoneRegistrationState cs){
	 switch(cs){
		case LinphoneRegistrationCleared:
			 return "LinphoneRegistrationCleared";
		break;
		case LinphoneRegistrationNone:
			 return "LinphoneRegistrationNone";
		break;
		case LinphoneRegistrationProgress:
			return "LinphoneRegistrationProgress";
		break;
		case LinphoneRegistrationOk:
			 return "LinphoneRegistrationOk";
		break;
		case LinphoneRegistrationFailed:
			 return "LinphoneRegistrationFailed";
		break;
	 }
	 return NULL;
 }

LinphoneReason linphone_proxy_config_get_error(const LinphoneProxyConfig *cfg) {
	return cfg->error;
}

void linphone_proxy_config_set_error(LinphoneProxyConfig *cfg,LinphoneReason error) {
	cfg->error = error;
}

const LinphoneAddress* linphone_proxy_config_get_service_route(const LinphoneProxyConfig* cfg) {
	return cfg->op?(const LinphoneAddress*) sal_op_get_service_route(cfg->op):NULL;
}
