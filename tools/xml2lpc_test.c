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

#include <stdio.h>
#include "xml2lpc.h"

void cb_function(void *ctx, xml2lpc_log_level level, const char *msg, va_list list) {
	const char *header = "";
	switch(level) {
		case XML2LPC_DEBUG:
			header = "DEBUG";
			break;
		case XML2LPC_MESSAGE:
			header = "MESSAGE";
			break;
		case XML2LPC_WARNING:
			header = "WARNING";
			break;
		case XML2LPC_ERROR:
			header = "ERROR";
			break;
	}
	fprintf(stdout, "%s - ", header);
	vfprintf(stdout, msg, list);
	fprintf(stdout, "\n");
}

void show_usage(int argc, char *argv[]) {
	fprintf(stderr, "usage %s convert <xml_file> <lpc_file>\n"
			"      %s validate <xml_file> <xsd_file>\n",
			argv[0], argv[0]);
}

int main(int argc, char *argv[]) {
	xml2lpc_context *ctx;
	if(argc != 4) {
		show_usage(argc, argv);
		return -1;
	}

	ctx = xml2lpc_context_new(cb_function, NULL);
	xml2lpc_set_xml_file(ctx, argv[2]);
	if(strcmp("convert", argv[1]) == 0) {
		LpConfig *lpc = lp_config_new(argv[3]);
		xml2lpc_convert(ctx, lpc);
		lp_config_sync(lpc);
		lp_config_destroy(lpc);
	} else if(strcmp("validate", argv[1]) == 0) {
		xml2lpc_set_xsd_file(ctx, argv[3]);
		xml2lpc_validate(ctx);
	} else {
		show_usage(argc, argv);
	}
	xml2lpc_context_destroy(ctx);
	return 0;
}

