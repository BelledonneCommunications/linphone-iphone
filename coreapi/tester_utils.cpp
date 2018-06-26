/*
tester_utils.cpp
Copyright (C) 2017  Belledonne Communications SARL

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

#include "tester_utils.h"
#include "private.h"

#include "call/call-p.h"
#include "chat/chat-room/chat-room-p.h"
#include "core/core-p.h"
#include "c-wrapper/c-wrapper.h"
#include "conference/session/media-session-p.h"
#include "event-log/conference/conference-chat-message-event.h"

using namespace std;

using namespace LinphonePrivate;

LinphoneVcardContext *linphone_core_get_vcard_context(const LinphoneCore *lc) {
	return lc->vcard_context;
}

void linphone_core_set_zrtp_not_available_simulation(LinphoneCore *lc, bool_t enabled) {
	lc->zrtp_not_available_simulation = enabled;
}

belle_http_provider_t *linphone_core_get_http_provider(const LinphoneCore *lc) {
	return lc->http_provider;
}

void linphone_core_enable_send_call_stats_periodical_updates(LinphoneCore *lc, bool_t enabled) {
	lc->send_call_stats_periodical_updates = enabled;
}

void linphone_core_set_zrtp_cache_db(LinphoneCore *lc, sqlite3 *cache_db) {
	lc->zrtp_cache_db = cache_db;
}

LinphoneCoreCbs *linphone_core_get_first_callbacks(const LinphoneCore *lc) {
	return ((VTableReference *)lc->vtable_refs->data)->cbs;
}

bctbx_list_t **linphone_core_get_call_logs_attribute(LinphoneCore *lc) {
	return &lc->call_logs;
}

void linphone_core_cbs_set_auth_info_requested(LinphoneCoreCbs *cbs, LinphoneCoreAuthInfoRequestedCb cb) {
	cbs->vtable->auth_info_requested = cb;
}

LinphoneQualityReporting *linphone_call_log_get_quality_reporting(LinphoneCallLog *call_log) {
	return &call_log->reporting;
}

reporting_session_report_t **linphone_quality_reporting_get_reports(LinphoneQualityReporting *qreporting) {
	return &qreporting->reports[0];
}

const bctbx_list_t *linphone_friend_get_insubs(const LinphoneFriend *fr) {
	return fr->insubs;
}

int linphone_friend_list_get_expected_notification_version(const LinphoneFriendList *list) {
	return list->expected_notification_version;
}

unsigned int linphone_friend_list_get_storage_id(const LinphoneFriendList *list) {
	return list->storage_id;
}

unsigned int linphone_friend_get_storage_id(const LinphoneFriend *lf) {
	return lf->storage_id;
}

void linphone_friend_set_core(LinphoneFriend *lf, LinphoneCore *lc) {
	lf->lc = lc;
}

LinphoneFriendList *linphone_friend_get_friend_list(const LinphoneFriend *lf) {
	return lf->friend_list;
}

bctbx_list_t **linphone_friend_list_get_friends_attribute(LinphoneFriendList *lfl) {
	return &lfl->friends;
}

const bctbx_list_t *linphone_friend_list_get_dirty_friends_to_update(const LinphoneFriendList *lfl) {
	return lfl->dirty_friends_to_update;
}

int linphone_friend_list_get_revision(const LinphoneFriendList *lfl) {
	return lfl->revision;
}

unsigned int _linphone_call_get_nb_media_starts (const LinphoneCall *call) {
	return L_GET_PRIVATE_FROM_C_OBJECT(call)->getMediaStartCount();
}

belle_sip_source_t *_linphone_call_get_dtmf_timer (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getDtmfTimer();
}

bool_t _linphone_call_has_dtmf_sequence (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getDtmfSequence().empty() ? FALSE : TRUE;
}

SalMediaDescription *_linphone_call_get_local_desc (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getLocalDesc();
}

SalMediaDescription *_linphone_call_get_result_desc (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getResultDesc();
}

MSWebCam *_linphone_call_get_video_device (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getVideoDevice();
}

void _linphone_call_add_local_desc_changed_flag (LinphoneCall *call, int flag) {
	L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->addLocalDescChangedFlag(flag);
}

int _linphone_call_get_main_audio_stream_index (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getMainAudioStreamIndex();
}

int _linphone_call_get_main_text_stream_index (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getMainTextStreamIndex();
}

int _linphone_call_get_main_video_stream_index (const LinphoneCall *call) {
	return L_GET_PRIVATE(static_pointer_cast<LinphonePrivate::MediaSession>(
		L_GET_PRIVATE_FROM_C_OBJECT(call)->getActiveSession()))->getMainVideoStreamIndex();
}

void _linphone_chat_room_enable_migration(LinphoneChatRoom *cr, bool_t enable) {
	shared_ptr<AbstractChatRoom> acr = L_GET_CPP_PTR_FROM_C_OBJECT(cr);
	L_GET_PRIVATE(acr->getCore())->mainDb->enableChatRoomMigration(acr->getChatRoomId(), !!enable);
}

int _linphone_chat_room_get_transient_message_count (const LinphoneChatRoom *cr) {
	shared_ptr<const ChatRoom> chatRoom = static_pointer_cast<const ChatRoom>(L_GET_CPP_PTR_FROM_C_OBJECT(cr));
	return (int)L_GET_PRIVATE(chatRoom)->transientEvents.size();
}

LinphoneChatMessage * _linphone_chat_room_get_first_transient_message (const LinphoneChatRoom *cr) {
	shared_ptr<const ChatRoom> chatRoom = static_pointer_cast<const ChatRoom>(L_GET_CPP_PTR_FROM_C_OBJECT(cr));
	if (L_GET_PRIVATE(chatRoom)->transientEvents.empty())
		return nullptr;
	shared_ptr<ConferenceChatMessageEvent> event = static_pointer_cast<ConferenceChatMessageEvent>(
		L_GET_PRIVATE(chatRoom)->transientEvents.front()
	);
	return L_GET_C_BACK_PTR(event->getChatMessage());
}