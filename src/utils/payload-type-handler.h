/*
 * payload-type-handler.h
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PAYLOAD_TYPE_HANDLER_H_
#define _PAYLOAD_TYPE_HANDLER_H_

#include "linphone/utils/general.h"

#include "sal/sal.h"

#define PAYLOAD_TYPE_ENABLED PAYLOAD_TYPE_USER_FLAG_0
#define PAYLOAD_TYPE_BITRATE_OVERRIDE PAYLOAD_TYPE_USER_FLAG_3
#define PAYLOAD_TYPE_FROZEN_NUMBER PAYLOAD_TYPE_USER_FLAG_4

// =============================================================================

L_DECL_C_STRUCT(LinphoneCore);

LINPHONE_BEGIN_NAMESPACE

struct VbrCodecBitrate {
	int maxAvailableBitrate;
	int minClockRate;
	int recommendedBitrate;
};

class PayloadTypeHandler {
public:
	PayloadTypeHandler (LinphoneCore *core) : core(core) {}

	bctbx_list_t *makeCodecsList (SalStreamType type, int bandwidthLimit, int maxCodecs, const bctbx_list_t *previousList);

	static bool bandwidthIsGreater (int bandwidth1, int bandwidth2);
	static int getAudioPayloadTypeBandwidth (const OrtpPayloadType *pt, int maxBandwidth);
	static double getAudioPayloadTypeBandwidthFromCodecBitrate (const OrtpPayloadType *pt);
	static int getMaxCodecSampleRate (const bctbx_list_t *codecs);
	static int getMinBandwidth (int downBandwidth, int upBandwidth);
	static int getRemainingBandwidthForVideo (int total, int audio);
	static bool isPayloadTypeNumberAvailable (const bctbx_list_t *codecs, int number, const OrtpPayloadType *ignore);

private:
	static int findPayloadTypeNumber (const bctbx_list_t *assigned, const OrtpPayloadType *pt);
	static bool hasTelephoneEventPayloadType (const bctbx_list_t *tev, int rate);
	static bool isPayloadTypeUsableForBandwidth (const OrtpPayloadType *pt, int bandwidthLimit);
	static int lookupTypicalVbrBitrate (int maxBandwidth, int clockRate);

	void assignPayloadTypeNumbers (const bctbx_list_t *codecs);
	bctbx_list_t *createSpecialPayloadTypes (const bctbx_list_t *codecs);
	bctbx_list_t *createTelephoneEventPayloadTypes (const bctbx_list_t *codecs);
	bool isPayloadTypeUsable (const OrtpPayloadType *pt);

	static const int udpHeaderSize;
	static const int rtpHeaderSize;
	static const int ipv4HeaderSize;
	static const VbrCodecBitrate defaultVbrCodecBitrates[];

	LinphoneCore *core = nullptr;
};

LINPHONE_END_NAMESPACE

#endif // ifndef _PAYLOAD_TYPE_HANDLER_H_
