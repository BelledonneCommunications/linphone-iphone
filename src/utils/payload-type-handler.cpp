/*
 * payload-type-handler.cpp
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

#include <cmath>

#include "private.h"

#include "logger/logger.h"

#include "payload-type-handler.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

const int PayloadTypeHandler::udpHeaderSize = 8;
const int PayloadTypeHandler::rtpHeaderSize = 12;

// 20 is the minimum, but there may be some options.
const int PayloadTypeHandler::ipv4HeaderSize = 20;

const VbrCodecBitrate PayloadTypeHandler::defaultVbrCodecBitrates[] = {
	{ 64, 44100, 50 },
	{ 64, 16000, 40 },
	{ 32, 16000, 32 },
	{ 32, 8000, 32 },
	{ 0, 8000, 24 },
	{ 0, 0, 0 }
};

// =============================================================================

int PayloadTypeHandler::findPayloadTypeNumber (const bctbx_list_t *assigned, const OrtpPayloadType *pt) {
	const OrtpPayloadType *candidate = nullptr;
	for (const bctbx_list_t *elem = assigned; elem != nullptr; elem = bctbx_list_next(elem)) {
		const OrtpPayloadType *it = reinterpret_cast<const OrtpPayloadType *>(bctbx_list_get_data(elem));
		if (
			strcasecmp(pt->mime_type, payload_type_get_mime(it)) == 0 &&
			it->clock_rate == pt->clock_rate &&
			((it->channels == pt->channels) || (pt->channels <= 0))
		) {
			candidate = it;
			if (
				(it->recv_fmtp && pt->recv_fmtp && (strcasecmp(it->recv_fmtp, pt->recv_fmtp) == 0)) ||
				(!it->recv_fmtp && !pt->recv_fmtp)
			)
				// Exact match.
				break;
		}
	}
	return candidate ? payload_type_get_number(candidate) : -1;
}

bool PayloadTypeHandler::hasTelephoneEventPayloadType (const bctbx_list_t *tev, int rate) {
	for (const bctbx_list_t *it = tev; it != nullptr; it = bctbx_list_next(it)) {
		const OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(it));
		if (pt->clock_rate == rate)
			return true;
	}
	return false;
}

bool PayloadTypeHandler::isPayloadTypeUsableForBandwidth (const OrtpPayloadType *pt, int bandwidthLimit) {
	const int videoEnablementLimit = 99;
	double codecBand = 0;
	switch (pt->type) {
		case PAYLOAD_AUDIO_CONTINUOUS:
		case PAYLOAD_AUDIO_PACKETIZED:
			codecBand = getAudioPayloadTypeBandwidth(pt, bandwidthLimit);
			return bandwidthIsGreater(bandwidthLimit, (int)codecBand);
		case PAYLOAD_VIDEO:
			// Infinite or greater than videoEnablementLimit.
			if (bandwidthLimit <= 0 || bandwidthLimit >= videoEnablementLimit)
				return true;
			break;
		case PAYLOAD_TEXT:
			return true;
	}
	return false;
}

int PayloadTypeHandler::lookupTypicalVbrBitrate (int maxBandwidth, int clockRate) {
	if (maxBandwidth <= 0)
		maxBandwidth = defaultVbrCodecBitrates[0].maxAvailableBitrate;
	for (const VbrCodecBitrate *it = &defaultVbrCodecBitrates[0]; it->minClockRate != 0; it++) {
		if ((maxBandwidth >= it->maxAvailableBitrate) && (clockRate >= it->minClockRate))
			return it->recommendedBitrate;
	}
	lError() << "lookupTypicalVbrBitrate(): should not happen";
	return 32;
}

// -----------------------------------------------------------------------------

void PayloadTypeHandler::assignPayloadTypeNumbers (const bctbx_list_t *codecs) {
	OrtpPayloadType *red = nullptr;
	OrtpPayloadType *t140 = nullptr;

	for (const bctbx_list_t *elem = codecs; elem != nullptr; elem = bctbx_list_next(elem)) {
		OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(elem));
		int number = payload_type_get_number(pt);

		// Check if number is duplicated: it could be the case if the remote forced us to use a mapping with a previous offer.
		if ((number != -1) && !(pt->flags & PAYLOAD_TYPE_FROZEN_NUMBER)) {
			if (!isPayloadTypeNumberAvailable(codecs, number, pt)) {
				lInfo() << "Reassigning payload type " << number << " " << pt->mime_type << "/" << pt->clock_rate << " because already offered";
				// Need to be re-assigned.
				number = -1;
			}
		}

		if (number == -1) {
			int dynNumber = getCore()->getCCore()->codecs_conf.dyn_pt;
			while (dynNumber < 127) {
				if (isPayloadTypeNumberAvailable(codecs, dynNumber, nullptr)) {
					payload_type_set_number(pt, dynNumber);
					dynNumber++;
					break;
				}
				dynNumber++;
			}
			if (dynNumber == 127) {
				lError() << "Too many payload types configured ! codec " << pt->mime_type << "/" << pt->clock_rate << " is disabled";
				payload_type_set_enable(pt, false);
			}
		}

		if (strcmp(pt->mime_type, payload_type_t140_red.mime_type) == 0)
			red = pt;
		else if (strcmp(pt->mime_type, payload_type_t140.mime_type) == 0)
			t140 = pt;
	}

	if (t140 && red) {
		int t140_payload_type_number = payload_type_get_number(t140);
		char *red_fmtp = ms_strdup_printf("%i/%i/%i", t140_payload_type_number, t140_payload_type_number, t140_payload_type_number);
		payload_type_set_recv_fmtp(red, red_fmtp);
		ms_free(red_fmtp);
	}
}

bctbx_list_t *PayloadTypeHandler::createSpecialPayloadTypes (const bctbx_list_t *codecs) {
	bctbx_list_t *result = createTelephoneEventPayloadTypes(codecs);
	if (linphone_core_generic_comfort_noise_enabled(getCore()->getCCore())) {
		OrtpPayloadType *cn = payload_type_clone(&payload_type_cn);
		payload_type_set_number(cn, 13);
		result = bctbx_list_append(result, cn);
	}
	return result;
}

bctbx_list_t *PayloadTypeHandler::createTelephoneEventPayloadTypes (const bctbx_list_t *codecs) {
	bctbx_list_t *result = nullptr;
	for (const bctbx_list_t *it = codecs; it != nullptr; it = bctbx_list_next(it)) {
		const OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(it));
		if (hasTelephoneEventPayloadType(result, pt->clock_rate))
			continue;

		OrtpPayloadType *tev = payload_type_clone(&payload_type_telephone_event);
		tev->clock_rate = pt->clock_rate;
		// Let it choose the number dynamically as for normal codecs.
		payload_type_set_number(tev, -1);
		// But for first telephone-event, prefer the number that was configured in the core.
		if (!result && isPayloadTypeNumberAvailable(codecs, getCore()->getCCore()->codecs_conf.telephone_event_pt, nullptr))
			payload_type_set_number(tev, getCore()->getCCore()->codecs_conf.telephone_event_pt);
		result = bctbx_list_append(result, tev);
	}
	return result;
}

bool PayloadTypeHandler::isPayloadTypeUsable (const OrtpPayloadType *pt) {
	return isPayloadTypeUsableForBandwidth(
		pt, getMinBandwidth(
			linphone_core_get_download_bandwidth(getCore()->getCCore()),
			linphone_core_get_upload_bandwidth(getCore()->getCCore())
		)
	);
}

// -----------------------------------------------------------------------------

bool PayloadTypeHandler::bandwidthIsGreater (int bandwidth1, int bandwidth2) {
	if (bandwidth1 <= 0) return true;
	if (bandwidth2 <= 0) return false;
	return bandwidth1 >= bandwidth2;
}

int PayloadTypeHandler::getAudioPayloadTypeBandwidth (const OrtpPayloadType *pt, int maxBandwidth) {
	if (payload_type_is_vbr(pt)) {
		if (pt->flags & PAYLOAD_TYPE_BITRATE_OVERRIDE) {
			lDebug() << "PayloadType " << pt->mime_type << "/" << pt->clock_rate << " has bitrate override";
			return pt->normal_bitrate / 1000;
		}
		return lookupTypicalVbrBitrate(maxBandwidth, pt->clock_rate);
	}
	/* Rounding codec bandwidth should be avoid, specially for AMR */
	return (int)ceil(getAudioPayloadTypeBandwidthFromCodecBitrate(pt) / 1000.0);
}

/*
 *((codec-birate*ptime/8) + RTP header + UDP header + IP header)*8/ptime;
 * ptime=1/npacket
 */
double PayloadTypeHandler::getAudioPayloadTypeBandwidthFromCodecBitrate (const OrtpPayloadType *pt) {
	double npacket = 50;
	if (strcmp(payload_type_get_mime(&payload_type_aaceld_44k), payload_type_get_mime(pt)) == 0)
		// Special case of aac 44K because ptime=10ms.
		npacket = 100;
	else if (strcmp(payload_type_get_mime(&payload_type_ilbc), payload_type_get_mime(pt)) == 0)
		npacket = 1000 / 30.0;

	int bitrate = pt->normal_bitrate;
	double packet_size = (((double)bitrate) / (npacket * 8)) + udpHeaderSize + rtpHeaderSize + ipv4HeaderSize;
	return packet_size * 8.0 * npacket;
}

int PayloadTypeHandler::getMaxCodecSampleRate (const bctbx_list_t *codecs) {
	int maxSampleRate = 0;
	for (const bctbx_list_t *it = codecs; it != nullptr; it = bctbx_list_next(it)) {
		OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(it));
		int sampleRate;
		if (strcasecmp("G722", pt->mime_type) == 0)
			// G722 spec says 8000 but the codec actually requires 16000.
			sampleRate = 16000;
		else
			sampleRate = pt->clock_rate;

		if (sampleRate > maxSampleRate)
			maxSampleRate = sampleRate;
	}
	return maxSampleRate;
}

int PayloadTypeHandler::getMinBandwidth (int downBandwidth, int upBandwidth) {
	if (downBandwidth <= 0) return upBandwidth;
	if (upBandwidth <= 0) return downBandwidth;
	return MIN(downBandwidth, upBandwidth);
}

int PayloadTypeHandler::getRemainingBandwidthForVideo (int total, int audio) {
	int remaining = total - audio - 10;
	if (remaining < 0) remaining = 0;
	return remaining;
}

bool PayloadTypeHandler::isPayloadTypeNumberAvailable (const bctbx_list_t *codecs, int number, const OrtpPayloadType *ignore) {
	for (const bctbx_list_t *elem = codecs; elem != nullptr; elem = bctbx_list_next(elem)) {
		const OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(elem));
		if ((pt != ignore) && (payload_type_get_number(pt) == number)) return false;
	}
	return true;
}

// -----------------------------------------------------------------------------

bctbx_list_t *PayloadTypeHandler::makeCodecsList (SalStreamType type, int bandwidthLimit, int maxCodecs, const bctbx_list_t *previousList) {
	const bctbx_list_t *allCodecs = nullptr;
	switch (type) {
		default:
		case SalAudio:
			allCodecs = getCore()->getCCore()->codecs_conf.audio_codecs;
			break;
		case SalVideo:
			allCodecs = getCore()->getCCore()->codecs_conf.video_codecs;
			break;
		case SalText:
			allCodecs = getCore()->getCCore()->codecs_conf.text_codecs;
			break;
	}

	int nb = 0;
	bctbx_list_t *result = nullptr;
	for (const bctbx_list_t *it = allCodecs; it != nullptr; it = bctbx_list_next(it)) {
		OrtpPayloadType *pt = reinterpret_cast<OrtpPayloadType *>(bctbx_list_get_data(it));
		if (!payload_type_enabled(pt))
			continue;

		if (bandwidthLimit > 0 && !isPayloadTypeUsableForBandwidth(pt, bandwidthLimit)) {
			lInfo() << "Codec " << pt->mime_type << "/" << pt->clock_rate << " eliminated because of audio bandwidth constraint of " <<
				bandwidthLimit << " kbit/s";
			continue;
		}

		if (!isPayloadTypeUsable(pt))
			continue;

		pt = payload_type_clone(pt);

		/* Look for a previously assigned number for this codec */
		int num = findPayloadTypeNumber(previousList, pt);
		if (num != -1) {
			payload_type_set_number(pt, num);
			payload_type_set_flag(pt, PAYLOAD_TYPE_FROZEN_NUMBER);
		}

		result = bctbx_list_append(result, pt);
		nb++;
		if ((maxCodecs > 0) && (nb >= maxCodecs)) break;
	}
	if (type == SalAudio) {
		bctbx_list_t *specials = createSpecialPayloadTypes(result);
		result = bctbx_list_concat(result, specials);
	}
	assignPayloadTypeNumbers(result);
	return result;
}

LINPHONE_END_NAMESPACE
