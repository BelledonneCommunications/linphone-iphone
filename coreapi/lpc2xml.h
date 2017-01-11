/*
linphone
Copyright (C) 2012 Belledonne Communications SARL

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

#ifndef LPC2XML_H_
#define LPC2XML_H_

#include "linphone/lpconfig.h"

typedef struct _lpc2xml_context lpc2xml_context;

typedef enum _lpc2xml_log_level {
	LPC2XML_DEBUG = 0,
	LPC2XML_MESSAGE,
	LPC2XML_WARNING,
	LPC2XML_ERROR
} lpc2xml_log_level;

typedef void(*lpc2xml_function)(void *ctx, lpc2xml_log_level level, const char *fmt, va_list list);

LINPHONE_PUBLIC lpc2xml_context* lpc2xml_context_new(lpc2xml_function cbf, void *ctx);
LINPHONE_PUBLIC void lpc2xml_context_destroy(lpc2xml_context*);

LINPHONE_PUBLIC int lpc2xml_set_lpc(lpc2xml_context* context, const LpConfig *lpc);

LINPHONE_PUBLIC int lpc2xml_convert_file(lpc2xml_context* context, const char *filename);
LINPHONE_PUBLIC int lpc2xml_convert_fd(lpc2xml_context* context, int fd);
LINPHONE_PUBLIC int lpc2xml_convert_string(lpc2xml_context* context, char **content);


#endif //LPC2XML_H_
