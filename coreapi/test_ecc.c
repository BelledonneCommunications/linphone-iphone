/*
linphone
Copyright (C) 2011 Belledonne Communications SARL
Author: Simon MORLAT (simon.morlat@linphone.org)

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
#include "linphonecore_utils.h"

static void calibration_finished(LinphoneCore *lc, LinphoneEcCalibratorStatus status, int delay, void *data){
	ms_message("echo calibration finished %s.",status==LinphoneEcCalibratorDone ? "successfully" : "with faillure");
	if (status==LinphoneEcCalibratorDone) ms_message("Measured delay is %i",delay);
}


static char config_file[1024];
void parse_args(int argc, char *argv[]){
	if (argc != 3 || strncmp("-c",argv[1], 2) || access(argv[2],F_OK)!=0) {
		printf("Usage: test_ecc [-c config_file] where config_file will be written with the detected value\n");
		exit(-1);
	}
	strncpy(config_file,argv[2],1024);
}

int main(int argc, char *argv[]){
	if (argc>1) parse_args(argc,argv);
	int count=0;
	LinphoneCoreVTable vtable={0};
	LinphoneCore *lc=linphone_core_new(&vtable,config_file,NULL,NULL);
	
	linphone_core_enable_logs(NULL);

	linphone_core_start_echo_calibration(lc,calibration_finished,NULL,NULL,NULL);
	
	while(count++<1000){
		linphone_core_iterate(lc);
		ms_usleep(10000);
	}
	linphone_core_destroy(lc);
	return 0;
}

