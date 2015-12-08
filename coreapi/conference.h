/*******************************************************************************
 *            conference.cc
 *
 *  Thu Nov 26, 2015
 *  Copyright  2015  Belledonne Communications
 *  Author: Linphone's team
 *  Email info@belledonne-communications.com
 ******************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef CONFERENCE_H
#define CONFERENCE_H

#include "linphonecore.h"

typedef struct _LinphoneConference LinphoneConference;

typedef enum {
	LinphoneConferenceClassLocal,
	LinphoneConferenceClassRemote
} LinphoneConferenceClass;

LinphoneConference *linphone_local_conference_new(LinphoneCore *core);
LinphoneConference *linphone_remote_conference_new(LinphoneCore *core);
void linphone_conference_free(LinphoneConference *obj);

int linphone_conference_add_call(LinphoneConference *obj, LinphoneCall *call);
int linphone_conference_remove_call(LinphoneConference *obj, LinphoneCall *call);
int linphone_conference_terminate(LinphoneConference *obj);

int linphone_conference_add_local_participant(LinphoneConference *obj);
int linphone_conference_remove_local_participant(LinphoneConference *obj);
bool_t linphone_conference_local_participant_is_in(const LinphoneConference *obj);
AudioStream *linphone_conference_get_audio_stream(const LinphoneConference *obj);

int linphone_conference_mute_microphone(LinphoneConference *obj, bool_t val);
bool_t linphone_conference_microphone_is_muted(const LinphoneConference *obj);
float linphone_conference_get_input_volume(const LinphoneConference *obj);
int linphone_conference_get_participant_count(const LinphoneConference *obj);

int linphone_conference_start_recording(LinphoneConference *obj, const char *path);
int linphone_conference_stop_recording(LinphoneConference *obj);

void linphone_conference_on_call_stream_starting(LinphoneConference *obj, LinphoneCall *call, bool_t is_paused_by_remote);
void linphone_conference_on_call_stream_stopping(LinphoneConference *obj, LinphoneCall *call);
void linphone_conference_on_call_terminating(LinphoneConference *obj, LinphoneCall *call);

bool_t linphone_conference_check_class(LinphoneConference *obj, LinphoneConferenceClass _class);

#endif // CONFERENCE_H
