/*
	belle-sip - SIP (RFC3261) library.
    Copyright (C) 2010  Belledonne Communications SARL

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LIBLINPHONE_TESTER_H_
#define LIBLINPHONE_TESTER_H_



const char* test_domain;
const char* auth_domain;
const char* test_username;
const char* test_password;



typedef struct _stats {
	int number_of_LinphoneRegistrationNone;
	int number_of_LinphoneRegistrationProgress ;
	int number_of_LinphoneRegistrationOk ;
	int number_of_LinphoneRegistrationCleared ;
	int number_of_LinphoneRegistrationFailed ;
	int number_of_auth_info_requested ;


	int number_of_LinphoneCallIncomingReceived;
	int number_of_LinphoneCallOutgoingInit;
	int number_of_LinphoneCallOutgoingProgress;
	int number_of_LinphoneCallOutgoingRinging;
	int number_of_LinphoneCallOutgoingEarlyMedia;
	int number_of_LinphoneCallConnected;
	int number_of_LinphoneCallStreamsRunning;
	int number_of_LinphoneCallPausing;
	int number_of_LinphoneCallPaused;
	int number_of_LinphoneCallResuming;
	int number_of_LinphoneCallRefered;
	int number_of_LinphoneCallError;
	int number_of_LinphoneCallEnd;
	int number_of_LinphoneCallPausedByRemote;
	int number_of_LinphoneCallUpdatedByRemote;
	int number_of_LinphoneCallIncomingEarlyMedia;
	int number_of_LinphoneCallUpdating;
	int number_of_LinphoneCallReleased;

	int number_of_LinphoneMessageReceived;

	int number_of_NewSubscriptionRequest;
	int number_of_NotifyReceived;
}stats;
typedef struct _LinphoneCoreManager {
	LinphoneCoreVTable v_table;
	LinphoneCore* lc;
	stats stat;
	LinphoneAddress* identity;
} LinphoneCoreManager;


LinphoneCoreManager* linphone_core_manager_new(const char* rc_file);
void linphone_core_manager_destroy(LinphoneCoreManager* mgr);

void reset_counters( stats* counters);

void registration_state_changed(struct _LinphoneCore *lc, LinphoneProxyConfig *cfg, LinphoneRegistrationState cstate, const char *message);
void auth_info_requested(LinphoneCore *lc, const char *realm, const char *username);
LinphoneCore* create_lc_with_auth(unsigned int with_auth) ;
LinphoneAddress * create_linphone_address(const char * domain);
LinphoneCore* configure_lc_from(LinphoneCoreVTable* v_table, const char* file,int proxy_count);
bool_t wait_for(LinphoneCore* lc_1, LinphoneCore* lc_2,int* counter,int value);

int call_test_suite ();
int register_test_suite ();
int message_test_suite ();
int presence_test_suite ();

#endif /* LIBLINPHONE_TESTER_H_ */
