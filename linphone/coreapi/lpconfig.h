/***************************************************************************
 *            lpconfig.h
 *
 *  Thu Mar 10 15:02:49 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon.morlat@linphone.org
 ****************************************************************************/

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
 
#ifndef LPCONFIG_H
#define LPCONFIG_H

typedef struct _LpConfig LpConfig;

#ifdef __cplusplus
extern "C" {
#endif

LpConfig * lp_config_new(const char *filename);
const char *lp_config_get_string(LpConfig *lpconfig, const char *section, const char *key, const char *default_string);
int lp_config_get_int(LpConfig *lpconfig,const char *section, const char *key, int default_value);
void lp_config_set_string(LpConfig *lpconfig,const char *section, const char *key, const char *value);
void lp_config_set_int(LpConfig *lpconfig,const char *section, const char *key, int value);
int lp_config_sync(LpConfig *lpconfig);
int lp_config_has_section(LpConfig *lpconfig, const char *section);
void lp_config_clean_section(LpConfig *lpconfig, const char *section);
void lp_config_destroy(LpConfig *cfg);
	
#ifdef __cplusplus
}
#endif

#endif
