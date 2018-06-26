/*
 * c-chat-room-cbs.cpp
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

#include "linphone/api/c-chat-room-cbs.h"

#include "c-wrapper/c-wrapper.h"

// =============================================================================

struct _LinphoneChatRoomCbs {
	belle_sip_object_t base;
	void *userData;
	LinphoneChatRoomCbsIsComposingReceivedCb isComposingReceivedCb;
	LinphoneChatRoomCbsMessageReceivedCb messageReceivedCb;
	LinphoneChatRoomCbsParticipantAddedCb participantAddedCb;
	LinphoneChatRoomCbsParticipantRemovedCb participantRemovedCb;
	LinphoneChatRoomCbsParticipantDeviceAddedCb participantDeviceAddedCb;
	LinphoneChatRoomCbsParticipantDeviceRemovedCb participantDeviceRemovedCb;
	LinphoneChatRoomCbsParticipantAdminStatusChangedCb participantAdminStatusChangedCb;
	LinphoneChatRoomCbsStateChangedCb stateChangedCb;
	LinphoneChatRoomCbsSubjectChangedCb subjectChangedCb;
	LinphoneChatRoomCbsConferenceJoinedCb conferenceJoinedCb;
	LinphoneChatRoomCbsConferenceLeftCb conferenceLeftCb;
	LinphoneChatRoomCbsUndecryptableMessageReceivedCb undecryptableMessageReceivedCb;
	LinphoneChatRoomCbsChatMessageReceivedCb chatMessageReceivedCb;
	LinphoneChatRoomCbsChatMessageSentCb chatMessageSentCb;
	LinphoneChatRoomCbsConferenceAddressGenerationCb conferenceAddressGenerationCb;
	LinphoneChatRoomCbsParticipantDeviceFetchRequestedCb participantDeviceFetchRequestedCb;
	LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb participantsCapabilitiesChecked;
	LinphoneChatRoomCbsParticipantRegistrationSubscriptionRequestedCb participantRegistrationSubscriptionRequestedCb;
	LinphoneChatRoomCbsParticipantRegistrationUnsubscriptionRequestedCb participantRegistrationUnsubscriptionRequestedCb;
	LinphoneChatRoomCbsShouldChatMessageBeStoredCb shouldMessageBeStoredCb;
};

BELLE_SIP_DECLARE_VPTR_NO_EXPORT(LinphoneChatRoomCbs);

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneChatRoomCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneChatRoomCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	FALSE
);

// =============================================================================

LinphoneChatRoomCbs * _linphone_chat_room_cbs_new (void) {
	return belle_sip_object_new(LinphoneChatRoomCbs);
}

LinphoneChatRoomCbs * linphone_chat_room_cbs_ref (LinphoneChatRoomCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_chat_room_cbs_unref (LinphoneChatRoomCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void * linphone_chat_room_cbs_get_user_data (const LinphoneChatRoomCbs *cbs) {
	return cbs->userData;
}

void linphone_chat_room_cbs_set_user_data (LinphoneChatRoomCbs *cbs, void *ud) {
	cbs->userData = ud;
}

LinphoneChatRoomCbsIsComposingReceivedCb linphone_chat_room_cbs_get_is_composing_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->isComposingReceivedCb;
}

void linphone_chat_room_cbs_set_is_composing_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsIsComposingReceivedCb cb) {
	cbs->isComposingReceivedCb = cb;
}

LinphoneChatRoomCbsMessageReceivedCb linphone_chat_room_cbs_get_message_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->messageReceivedCb;
}

void linphone_chat_room_cbs_set_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsMessageReceivedCb cb) {
	cbs->messageReceivedCb = cb;
}

LinphoneChatRoomCbsChatMessageReceivedCb linphone_chat_room_cbs_get_chat_message_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->chatMessageReceivedCb;
}

void linphone_chat_room_cbs_set_chat_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsChatMessageReceivedCb cb) {
	cbs->chatMessageReceivedCb = cb;
}

LinphoneChatRoomCbsChatMessageSentCb linphone_chat_room_cbs_get_chat_message_sent (const LinphoneChatRoomCbs *cbs) {
	return cbs->chatMessageSentCb;
}

void linphone_chat_room_cbs_set_chat_message_sent (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsChatMessageSentCb cb) {
	cbs->chatMessageSentCb = cb;
}

LinphoneChatRoomCbsParticipantAddedCb linphone_chat_room_cbs_get_participant_added (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantAddedCb;
}

void linphone_chat_room_cbs_set_participant_added (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantAddedCb cb) {
	cbs->participantAddedCb = cb;
}

LinphoneChatRoomCbsParticipantRemovedCb linphone_chat_room_cbs_get_participant_removed (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantRemovedCb;
}

void linphone_chat_room_cbs_set_participant_removed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantRemovedCb cb) {
	cbs->participantRemovedCb = cb;
}

LinphoneChatRoomCbsParticipantAdminStatusChangedCb linphone_chat_room_cbs_get_participant_admin_status_changed (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantAdminStatusChangedCb;
}

void linphone_chat_room_cbs_set_participant_admin_status_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantAdminStatusChangedCb cb) {
	cbs->participantAdminStatusChangedCb = cb;
}

LinphoneChatRoomCbsStateChangedCb linphone_chat_room_cbs_get_state_changed (const LinphoneChatRoomCbs *cbs) {
	return cbs->stateChangedCb;
}

void linphone_chat_room_cbs_set_state_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsStateChangedCb cb) {
	cbs->stateChangedCb = cb;
}

LinphoneChatRoomCbsSubjectChangedCb linphone_chat_room_cbs_get_subject_changed (const LinphoneChatRoomCbs *cbs) {
	return cbs->subjectChangedCb;
}

void linphone_chat_room_cbs_set_subject_changed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsSubjectChangedCb cb) {
	cbs->subjectChangedCb = cb;
}

LinphoneChatRoomCbsUndecryptableMessageReceivedCb linphone_chat_room_cbs_get_undecryptable_message_received (const LinphoneChatRoomCbs *cbs) {
	return cbs->undecryptableMessageReceivedCb;
}

void linphone_chat_room_cbs_set_undecryptable_message_received (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsUndecryptableMessageReceivedCb cb) {
	cbs->undecryptableMessageReceivedCb = cb;
}

LinphoneChatRoomCbsParticipantDeviceAddedCb linphone_chat_room_cbs_get_participant_device_added (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantDeviceAddedCb;
}

void linphone_chat_room_cbs_set_participant_device_added (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantDeviceAddedCb cb) {
	cbs->participantDeviceAddedCb = cb;
}

LinphoneChatRoomCbsParticipantDeviceRemovedCb linphone_chat_room_cbs_get_participant_device_removed (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantDeviceRemovedCb;
}

void linphone_chat_room_cbs_set_participant_device_removed (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantDeviceRemovedCb cb) {
	cbs->participantDeviceRemovedCb = cb;
}

LinphoneChatRoomCbsConferenceJoinedCb linphone_chat_room_cbs_get_conference_joined (const LinphoneChatRoomCbs *cbs) {
	return cbs->conferenceJoinedCb;
}

void linphone_chat_room_cbs_set_conference_joined (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsConferenceJoinedCb cb) {
	cbs->conferenceJoinedCb = cb;
}

LinphoneChatRoomCbsConferenceLeftCb linphone_chat_room_cbs_get_conference_left (const LinphoneChatRoomCbs *cbs) {
	return cbs->conferenceLeftCb;
}

void linphone_chat_room_cbs_set_conference_left (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsConferenceLeftCb cb) {
	cbs->conferenceLeftCb = cb;
}

LinphoneChatRoomCbsConferenceAddressGenerationCb linphone_chat_room_cbs_get_conference_address_generation (const LinphoneChatRoomCbs *cbs) {
	return cbs->conferenceAddressGenerationCb;
}

void linphone_chat_room_cbs_set_conference_address_generation (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsConferenceAddressGenerationCb cb) {
	cbs->conferenceAddressGenerationCb = cb;
}

LinphoneChatRoomCbsParticipantDeviceFetchRequestedCb linphone_chat_room_cbs_get_participant_device_fetch_requested (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantDeviceFetchRequestedCb;
}

void linphone_chat_room_cbs_set_participant_device_fetch_requested (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantDeviceFetchRequestedCb cb) {
	cbs->participantDeviceFetchRequestedCb = cb;
}

LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb linphone_chat_room_cbs_get_participants_capabilities_checked (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantsCapabilitiesChecked;
}

void linphone_chat_room_cbs_set_participants_capabilities_checked (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantsCapabilitiesCheckedCb cb) {
	cbs->participantsCapabilitiesChecked = cb;
}

LinphoneChatRoomCbsParticipantRegistrationSubscriptionRequestedCb linphone_chat_room_cbs_get_participant_registration_subscription_requested (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantRegistrationSubscriptionRequestedCb;
}

void linphone_chat_room_cbs_set_participant_registration_subscription_requested (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantRegistrationSubscriptionRequestedCb cb) {
	cbs->participantRegistrationSubscriptionRequestedCb = cb;
}

LinphoneChatRoomCbsParticipantRegistrationUnsubscriptionRequestedCb linphone_chat_room_cbs_get_participant_registration_unsubscription_requested (const LinphoneChatRoomCbs *cbs) {
	return cbs->participantRegistrationUnsubscriptionRequestedCb;
}

void linphone_chat_room_cbs_set_participant_registration_unsubscription_requested (LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsParticipantRegistrationUnsubscriptionRequestedCb cb) {
	cbs->participantRegistrationUnsubscriptionRequestedCb = cb;
}

LinphoneChatRoomCbsShouldChatMessageBeStoredCb linphone_chat_room_cbs_get_chat_message_should_be_stored( LinphoneChatRoomCbs *cbs) {
	return cbs->shouldMessageBeStoredCb;
}

void linphone_chat_room_cbs_set_chat_message_should_be_stored( LinphoneChatRoomCbs *cbs, LinphoneChatRoomCbsShouldChatMessageBeStoredCb cb) {
	cbs->shouldMessageBeStoredCb = cb;
}
