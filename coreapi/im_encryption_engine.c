/*
ImEncryptionEgine.c
Copyright (C) 2016  Belledonne Communications SARL

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

#include "linphonecore.h"
#include "im_encryption_engine.h"

struct _LinphoneImEncryptionEngineCbs {
	void *user_data;
	LinphoneImEncryptionEngineIncomingMessageCb process_incoming_message;
	LinphoneImEncryptionEngineDownloadingFileCb process_downlading_file;
	LinphoneImEncryptionEngineOutgoingMessageCb process_outgoing_message;
};

struct _LinphoneImEncryptionEngine {
	void *user_data;
	LinphoneImEncryptionEngineCbs *callbacks;
};

LinphoneImEncryptionEngineCbs *linphone_im_encryption_engine_cbs_new(void) {
	LinphoneImEncryptionEngineCbs *cbs = ms_new0(LinphoneImEncryptionEngineCbs, 1);
	return cbs;
}

void linphone_im_encryption_engine_cbs_destory(LinphoneImEncryptionEngineCbs *cbs) {
	ms_free(cbs);
}

void *linphone_im_encryption_engine_cbs_get_user_data(const LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->user_data;
}

void linphone_im_encryption_engine_cbs_set_user_data(LinphoneImEncryptionEngineCbs *cbs, void *data) {
	cbs->user_data = data;
}

LinphoneImEncryptionEngine *linphone_im_encryption_engine_new(void) {
	LinphoneImEncryptionEngine *imee = ms_new0(LinphoneImEncryptionEngine, 1);
	imee->callbacks = linphone_im_encryption_engine_cbs_new();
	return imee;
}

void linphone_im_encryption_engine_destory(LinphoneImEncryptionEngine *imee) {
	if (imee->callbacks) linphone_im_encryption_engine_cbs_destory(imee->callbacks);
	ms_free(imee);
}

void *linphone_im_encryption_engine_get_user_data(const LinphoneImEncryptionEngine *imee) {
	return imee->user_data;
}

void linphone_im_encryption_engine_set_user_data(LinphoneImEncryptionEngine *imee, void *data) {
	imee->user_data = data;
}

LinphoneImEncryptionEngineCbs* linphone_im_encryption_engine_get_callbacks(const LinphoneImEncryptionEngine *imee) {
	return imee->callbacks;
}

LinphoneImEncryptionEngineIncomingMessageCb linphone_im_encryption_engine_cbs_get_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_incoming_message;
}

void linphone_im_encryption_engine_cbs_set_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineIncomingMessageCb cb) {
	cbs->process_incoming_message = cb;
}

LinphoneImEncryptionEngineOutgoingMessageCb linphone_im_encryption_engine_cbs_get_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_outgoing_message;
}

void linphone_im_encryption_engine_cbs_set_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineOutgoingMessageCb cb) {
	cbs->process_outgoing_message = cb;
}

LinphoneImEncryptionEngineDownloadingFileCb linphone_im_encryption_engine_cbs_get_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_downlading_file;
}

void linphone_im_encryption_engine_cbs_set_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineDownloadingFileCb cb) {
	cbs->process_downlading_file = cb;
}