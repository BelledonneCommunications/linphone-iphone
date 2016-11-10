/*
ImEncryptionEgine.h
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

#ifndef IM_ENCRYPTION_ENGINE_H
#define IM_ENCRYPTION_ENGINE_H

#include <mediastreamer2/mscommon.h>

#ifndef LINPHONE_PUBLIC
#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

typedef int (*LinphoneImEncryptionEngineIncomingMessageCb)(LinphoneCore* lc, LinphoneChatRoom *room, LinphoneChatMessage *msg);

typedef int (*LinphoneImEncryptionEngineOutgoingMessageCb)(LinphoneCore* lc, LinphoneChatRoom *room, LinphoneChatMessage *msg);

typedef int (*LinphoneImEncryptionEngineDownloadingFileCb)(LinphoneCore *lc, LinphoneChatMessage *msg, const char *buffer, size_t size, char **decrypted_buffer);

typedef struct _LinphoneImEncryptionEngineCbs LinphoneImEncryptionEngineCbs;

typedef struct _LinphoneImEncryptionEngine LinphoneImEncryptionEngine;

LinphoneImEncryptionEngineCbs *linphone_im_encryption_engine_cbs_new(void);

void linphone_im_encryption_engine_cbs_destory(LinphoneImEncryptionEngineCbs *cbs);

LINPHONE_PUBLIC void *linphone_im_encryption_engine_cbs_get_user_data(const LinphoneImEncryptionEngineCbs *cbs);

LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_user_data(LinphoneImEncryptionEngineCbs *cbs, void *data);

LINPHONE_PUBLIC LinphoneImEncryptionEngine *linphone_im_encryption_engine_new(void);

LINPHONE_PUBLIC void linphone_im_encryption_engine_destory(LinphoneImEncryptionEngine *imee);

LINPHONE_PUBLIC void *linphone_im_encryption_engine_get_user_data(const LinphoneImEncryptionEngine *imee);

LINPHONE_PUBLIC void linphone_im_encryption_engine_set_user_data(LinphoneImEncryptionEngine *imee, void *data);

LINPHONE_PUBLIC LinphoneImEncryptionEngineCbs* linphone_im_encryption_engine_get_callbacks(const LinphoneImEncryptionEngine *imee);

LINPHONE_PUBLIC LinphoneImEncryptionEngineIncomingMessageCb linphone_im_encryption_engine_cbs_get_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs);

LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_incoming_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineIncomingMessageCb cb);

LINPHONE_PUBLIC LinphoneImEncryptionEngineOutgoingMessageCb linphone_im_encryption_engine_cbs_get_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs);

LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_outgoing_message(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineOutgoingMessageCb cb);

LINPHONE_PUBLIC LinphoneImEncryptionEngineDownloadingFileCb linphone_im_encryption_engine_cbs_get_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs);

LINPHONE_PUBLIC void linphone_im_encryption_engine_cbs_set_process_downloading_file(LinphoneImEncryptionEngineCbs *cbs, LinphoneImEncryptionEngineDownloadingFileCb cb);

#endif /* IM_ENCRYPTION_ENGINE_H */