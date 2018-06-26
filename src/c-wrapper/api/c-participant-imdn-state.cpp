/*
 * c-participant-imdn-state.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "linphone/api/c-participant-imdn-state.h"

#include "c-wrapper/c-wrapper.h"
#include "conference/participant.h"
#include "conference/participant-imdn-state.h"

// =============================================================================

using namespace std;

L_DECLARE_C_CLONABLE_OBJECT_IMPL(ParticipantImdnState);

LinphoneParticipantImdnState *linphone_participant_imdn_state_ref (LinphoneParticipantImdnState *state) {
	belle_sip_object_ref(state);
	return state;
}

void linphone_participant_imdn_state_unref (LinphoneParticipantImdnState *state) {
	belle_sip_object_unref(state);
}

void *linphone_participant_imdn_state_get_user_data(const LinphoneParticipantImdnState *state) {
	return L_GET_USER_DATA_FROM_C_OBJECT(state);
}

void linphone_participant_imdn_state_set_user_data(LinphoneParticipantImdnState *state, void *ud) {
	L_SET_USER_DATA_FROM_C_OBJECT(state, ud);
}

const LinphoneParticipant *linphone_participant_imdn_state_get_participant (const LinphoneParticipantImdnState *state) {
	return L_GET_C_BACK_PTR(L_GET_CPP_PTR_FROM_C_OBJECT(state)->getParticipant());
}

LinphoneChatMessageState linphone_participant_imdn_state_get_state (const LinphoneParticipantImdnState *state) {
	return (LinphoneChatMessageState)L_GET_CPP_PTR_FROM_C_OBJECT(state)->getState();
}

time_t linphone_participant_imdn_state_get_state_change_time (const LinphoneParticipantImdnState *state) {
	return L_GET_CPP_PTR_FROM_C_OBJECT(state)->getStateChangeTime();
}

const LinphoneParticipantImdnState *_linphone_participant_imdn_state_from_cpp_obj (const LinphonePrivate::ParticipantImdnState &state) {
	return L_GET_C_BACK_PTR(&state);
}