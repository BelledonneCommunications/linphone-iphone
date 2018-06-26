/*
 * payload-type-handler.h
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

#ifndef _L_PAYLOAD_TYPE_HANDLER_H_
#define _L_PAYLOAD_TYPE_HANDLER_H_

#include "linphone/utils/general.h"

#include "c-wrapper/internal/c-sal.h"
#include "core/core.h"
#include "core/core-accessor.h"

#define PAYLOAD_TYPE_ENABLED PAYLOAD_TYPE_USER_FLAG_0
#define PAYLOAD_TYPE_BITRATE_OVERRIDE PAYLOAD_TYPE_USER_FLAG_3
#define PAYLOAD_TYPE_FROZEN_NUMBER PAYLOAD_TYPE_USER_FLAG_4

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

struct VbrCodecBitrate {
	int maxAvailableBitrate;
	int minClockRate;
	int recommendedBitrate;
};

class Core;

class PayloadTypeHandler : public CoreAccessor {
public:
	explicit PayloadTypeHandler (const std::shared_ptr<Core> &core) : CoreAccessor(core) {}

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
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_PAYLOAD_TYPE_HANDLER_H_
