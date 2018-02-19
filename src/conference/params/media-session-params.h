/*
 * media-session-params.h
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

#ifndef _L_MEDIA_SESSION_PARAMS_H_
#define _L_MEDIA_SESSION_PARAMS_H_

#include <ortp/payloadtype.h>

#include "call-session-params.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class MediaSession;
class MediaSessionPrivate;
class MediaSessionParamsPrivate;

class MediaSessionParams : public CallSessionParams {
	friend class MediaSession;
	friend class MediaSessionPrivate;

public:
	MediaSessionParams ();
	MediaSessionParams (const MediaSessionParams &other);
	virtual ~MediaSessionParams ();

	MediaSessionParams &operator= (const MediaSessionParams &other);

	void initDefault (const std::shared_ptr<Core> &core) override;

	bool audioEnabled () const;
	bool audioMulticastEnabled () const;
	void enableAudio (bool value);
	void enableAudioMulticast (bool value);
	int getAudioBandwidthLimit () const;
	LinphoneMediaDirection getAudioDirection () const;
	const OrtpPayloadType * getUsedAudioCodec () const;
	LinphonePayloadType * getUsedAudioPayloadType () const;
	void setAudioBandwidthLimit (int value);
	void setAudioDirection (LinphoneMediaDirection direction);

	void enableVideo (bool value);
	void enableVideoMulticast (bool value);
	float getReceivedFps () const;
	LinphoneVideoDefinition * getReceivedVideoDefinition () const;
	float getSentFps () const;
	LinphoneVideoDefinition * getSentVideoDefinition () const;
	const OrtpPayloadType * getUsedVideoCodec () const;
	LinphonePayloadType * getUsedVideoPayloadType () const;
	LinphoneMediaDirection getVideoDirection () const;
	void setVideoDirection (LinphoneMediaDirection direction);
	bool videoEnabled () const;
	bool videoMulticastEnabled () const;

	void enableRealtimeText (bool value);
	const OrtpPayloadType * getUsedRealtimeTextCodec () const;
	LinphonePayloadType * getUsedRealtimeTextPayloadType () const;
	bool realtimeTextEnabled () const;

	bool avpfEnabled () const;
	void enableAvpf (bool value);
	uint16_t getAvpfRrInterval () const;
	void setAvpfRrInterval (uint16_t value);

	bool lowBandwidthEnabled () const;
	void enableLowBandwidth (bool value);

	const std::string& getRecordFilePath () const;
	void setRecordFilePath (const std::string &path);

	bool earlyMediaSendingEnabled () const;
	void enableEarlyMediaSending (bool value);

	void enableMandatoryMediaEncryption (bool value);
	LinphoneMediaEncryption getMediaEncryption () const;
	bool mandatoryMediaEncryptionEnabled () const;
	void setMediaEncryption (LinphoneMediaEncryption encryption);

	SalMediaProto getMediaProto () const;
	const char * getRtpProfile () const;

	void addCustomSdpAttribute (const std::string &attributeName, const std::string &attributeValue);
	void clearCustomSdpAttributes ();
	const char * getCustomSdpAttribute (const std::string &attributeName) const;

	void addCustomSdpMediaAttribute (LinphoneStreamType lst, const std::string &attributeName, const std::string &attributeValue);
	void clearCustomSdpMediaAttributes (LinphoneStreamType lst);
	const char * getCustomSdpMediaAttribute (LinphoneStreamType lst, const std::string &attributeName) const;

private:
	L_DECLARE_PRIVATE(MediaSessionParams);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_MEDIA_SESSION_PARAMS_H_
