/*
im_encryption_engine.c
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

#include "linphone/core.h"
#include "linphone/im_encryption_engine.h"
#include "private.h"

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneImEncryptionEngineCbs);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneImEncryptionEngineCbs, belle_sip_object_t,
	NULL, // destroy
	NULL, // clone
	NULL, // marshal
	TRUE
);

static void linphone_im_encryption_engine_destroy(LinphoneImEncryptionEngine *imee) {
	if (imee->callbacks) linphone_im_encryption_engine_cbs_unref(imee->callbacks);
}

BELLE_SIP_DECLARE_NO_IMPLEMENTED_INTERFACES(LinphoneImEncryptionEngine);

BELLE_SIP_INSTANCIATE_VPTR(LinphoneImEncryptionEngine, belle_sip_object_t,
	(belle_sip_object_destroy_t)linphone_im_encryption_engine_destroy,
	NULL, // clone
	NULL, // marshal
	TRUE
);

LinphoneImEncryptionEngineCbs *linphone_im_encryption_engine_cbs_new(void) {
	LinphoneImEncryptionEngineCbs *cbs = belle_sip_object_new(LinphoneImEncryptionEngineCbs);
	belle_sip_object_ref(cbs);
	return cbs;
}

LinphoneImEncryptionEngineCbs * linphone_im_encryption_engine_cbs_ref(LinphoneImEncryptionEngineCbs *cbs) {
	belle_sip_object_ref(cbs);
	return cbs;
}

void linphone_im_encryption_engine_cbs_unref(LinphoneImEncryptionEngineCbs *cbs) {
	belle_sip_object_unref(cbs);
}

void *linphone_im_encryption_engine_cbs_get_user_data(const LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->user_data;
}

void linphone_im_encryption_engine_cbs_set_user_data(LinphoneImEncryptionEngineCbs *cbs, void *data) {
	cbs->user_data = data;
}

LinphoneImEncryptionEngine *linphone_im_encryption_engine_new(LinphoneCore *lc) {
	LinphoneImEncryptionEngine *imee = belle_sip_object_new(LinphoneImEncryptionEngine);
	belle_sip_object_ref(imee);
	imee->lc = lc;
	imee->callbacks = linphone_im_encryption_engine_cbs_new();
	return imee;
}

LinphoneImEncryptionEngine * linphone_im_encryption_engine_ref(LinphoneImEncryptionEngine *imee) {
	belle_sip_object_ref(imee);
	return imee;
}

void linphone_im_encryption_engine_unref(LinphoneImEncryptionEngine *imee) {
	belle_sip_object_unref(imee);
}

void *linphone_im_encryption_engine_get_user_data(const LinphoneImEncryptionEngine *imee) {
	return imee->user_data;
}

void linphone_im_encryption_engine_set_user_data(LinphoneImEncryptionEngine *imee, void *data) {
	imee->user_data = data;
}

LinphoneCore * linphone_im_encryption_engine_get_core(LinphoneImEncryptionEngine *imee) {
	return imee->lc;
}

LinphoneImEncryptionEngineCbs* linphone_im_encryption_engine_get_callbacks(const LinphoneImEncryptionEngine *imee) {
	return imee->callbacks;
}

LinphoneImEncryptionEngineCbsIncomingMessageCb linphone_im_encryption_engine_cbs_get_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_incoming_message;
}

void linphone_im_encryption_engine_cbs_set_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsIncomingMessageCb cb) {
	cbs->process_incoming_message = cb;
}

LinphoneImEncryptionEngineCbsOutgoingMessageCb linphone_im_encryption_engine_cbs_get_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_outgoing_message;
}

void linphone_im_encryption_engine_cbs_set_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsOutgoingMessageCb cb) {
	cbs->process_outgoing_message = cb;
}

LinphoneImEncryptionEngineCbsDownloadingFileCb linphone_im_encryption_engine_cbs_get_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_downlading_file;
}

void linphone_im_encryption_engine_cbs_set_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsDownloadingFileCb cb) {
	cbs->process_downlading_file = cb;
}

LinphoneImEncryptionEngineCbsUploadingFileCb linphone_im_encryption_engine_cbs_get_process_uploading_file(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->process_uploading_file;
}

void linphone_im_encryption_engine_cbs_set_process_uploading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsUploadingFileCb cb) {
	cbs->process_uploading_file = cb;
}

LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb linphone_im_encryption_engine_cbs_get_is_encryption_enabled_for_file_transfer(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->is_encryption_enabled_for_file_transfer;
}

void linphone_im_encryption_engine_cbs_set_is_encryption_enabled_for_file_transfer(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsIsEncryptionEnabledForFileTransferCb cb) {
	cbs->is_encryption_enabled_for_file_transfer = cb;
}

LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb linphone_im_encryption_engine_cbs_get_generate_file_transfer_key(LinphoneImEncryptionEngineCbs *cbs) {
	return cbs->generate_file_transfer_key;
}

void linphone_im_encryption_engine_cbs_set_generate_file_transfer_key(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineCbsGenerateFileTransferKeyCb cb) {
		cbs->generate_file_transfer_key = cb;
}
