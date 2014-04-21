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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef XML2LPC_H_
#define XML2LPC_H_

#include "lpconfig.h"

typedef struct _xml2lpc_context xml2lpc_context;

typedef enum _xml2lpc_log_level {
	XML2LPC_DEBUG = 0,
	XML2LPC_MESSAGE,
	XML2LPC_WARNING,
	XML2LPC_ERROR
} xml2lpc_log_level;

typedef void(*xml2lpc_function)(void *ctx, xml2lpc_log_level level, const char *fmt, va_list list);

xml2lpc_context* xml2lpc_context_new(xml2lpc_function cbf, void *ctx);
void xml2lpc_context_destroy(xml2lpc_context*);

int xml2lpc_set_xml_file(xml2lpc_context* context, const char *filename);
int xml2lpc_set_xml_fd(xml2lpc_context* context, int fd);
int xml2lpc_set_xml_string(xml2lpc_context* context, const char *content);

int xml2lpc_set_xsd_file(xml2lpc_context* context, const char *filename);
int xml2lpc_set_xsd_fd(xml2lpc_context* context, int fd);
int xml2lpc_set_xsd_string(xml2lpc_context* context, const char *content);

int xml2lpc_validate(xml2lpc_context *context);
int xml2lpc_convert(xml2lpc_context *context, LpConfig *lpc);



#endif //XML2LPC_H_
