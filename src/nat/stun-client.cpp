/*
 * stun-client.cpp
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

#include "private.h"

#include "logger/logger.h"

#include "stun-client.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

int StunClient::run (int audioPort, int videoPort, int textPort) {
	stunDiscoveryDone = false;
	if (linphone_core_ipv6_enabled(getCore()->getCCore())) {
		lWarning() << "STUN support is not implemented for ipv6";
		return -1;
	}
	if (!linphone_core_get_stun_server(getCore()->getCCore()))
		return -1;
	const struct addrinfo *ai = linphone_core_get_stun_server_addrinfo(getCore()->getCCore());
	if (!ai) {
		lError() << "Could not obtain STUN server addrinfo";
		return -1;
	}

	/* Create the RTP sockets and send STUN messages to the STUN server */
	ortp_socket_t sockAudio = createStunSocket(audioPort);
	if (sockAudio == -1)
		return -1;
	ortp_socket_t sockVideo = -1;
	if (linphone_core_video_enabled(getCore()->getCCore())) {
		sockVideo = createStunSocket(videoPort);
		if (sockVideo == -1)
			return -1;
	}
	ortp_socket_t sockText = -1;
	if (linphone_core_realtime_text_enabled(getCore()->getCCore())) {
		sockText = createStunSocket(textPort);
		if (sockText == -1)
			return -1;
	}

	int ret = 0;
	int loops = 0;
	bool gotAudio = false;
	bool gotVideo = false;
	bool gotText = false;
	bool coneAudio = false;
	bool coneVideo = false;
	bool coneText = false;
	double elapsed;
	struct timeval init;
	ortp_gettimeofday(&init, nullptr);

	do {
		int id;
		if ((loops % 20) == 0) {
			lInfo() << "Sending STUN requests...";
			sendStunRequest(sockAudio, ai->ai_addr, (socklen_t)ai->ai_addrlen, 11, true);
			sendStunRequest(sockAudio, ai->ai_addr, (socklen_t)ai->ai_addrlen, 1, false);
			if (sockVideo != -1) {
				sendStunRequest(sockVideo, ai->ai_addr, (socklen_t)ai->ai_addrlen, 22, true);
				sendStunRequest(sockVideo, ai->ai_addr, (socklen_t)ai->ai_addrlen, 2, false);
			}
			if (sockText != -1) {
				sendStunRequest(sockText, ai->ai_addr, (socklen_t)ai->ai_addrlen, 33, true);
				sendStunRequest(sockText, ai->ai_addr, (socklen_t)ai->ai_addrlen, 3, false);
			}
		}
		ms_usleep(10000);

		if (recvStunResponse(sockAudio, audioCandidate, id) > 0) {
			lInfo() << "STUN test result: local audio port maps to " << audioCandidate.address << ":" << audioCandidate.port;
			if (id == 11) coneAudio = true;
			gotAudio = true;
		}
		if (recvStunResponse(sockVideo, videoCandidate, id) > 0) {
			lInfo() << "STUN test result: local video port maps to " << videoCandidate.address << ":" << videoCandidate.port;
			if (id == 22) coneVideo = true;
			gotVideo = true;
		}
		if (recvStunResponse(sockText, textCandidate, id) > 0) {
			lInfo() << "STUN test result: local text port maps to " << textCandidate.address << ":" << textCandidate.port;
			if (id == 33) coneText = true;
			gotText = true;
		}
		struct timeval cur;
		ortp_gettimeofday(&cur, nullptr);
		elapsed = static_cast<double>(cur.tv_sec - init.tv_sec) * 1000 + static_cast<double>(cur.tv_usec - init.tv_usec) / 1000;
		if (elapsed > 2000.) {
			lInfo() << "STUN responses timeout, going ahead";
			ret = -1;
			break;
		}
		loops++;
	} while (!(gotAudio && (gotVideo || sockVideo == -1) && (gotText || sockText == -1)));

	if (ret == 0)
		ret = (int)elapsed;

	if (!gotAudio)
		lError() << "No STUN server response for audio port";
	else if (!coneAudio)
		lInfo() << "NAT is symmetric for audio port";

	if (sockVideo != -1) {
		if (!gotVideo)
			lError() << "No STUN server response for video port";
		else if (!coneVideo)
			lInfo() << "NAT is symmetric for video port";
	}

	if (sockText != -1) {
		if (!gotText)
			lError() << "No STUN server response for text port";
		else if (!coneText)
			lInfo() << "NAT is symmetric for text port";
	}

	close_socket(sockAudio);
	if (sockVideo != -1) close_socket(sockVideo);
	if (sockText != -1) close_socket(sockText);
	stunDiscoveryDone = true;
	return ret;
}

void StunClient::updateMediaDescription (SalMediaDescription *md) const {
	if (!stunDiscoveryDone) return;
	for (int i = 0; i < SAL_MEDIA_DESCRIPTION_MAX_STREAMS; i++) {
		if (!sal_stream_description_active(&md->streams[i]))
			continue;
		if (md->streams[i].type == SalAudio && audioCandidate.port != 0) {
			strncpy(md->streams[i].rtp_addr, audioCandidate.address.c_str(), sizeof(md->streams[i].rtp_addr));
			md->streams[i].rtp_port = audioCandidate.port;
			if (
				(
					!audioCandidate.address.empty() &&
					!videoCandidate.address.empty() &&
					audioCandidate.address == videoCandidate.address
				) ||
				sal_media_description_get_nb_active_streams(md) == 1
			)
				strncpy(md->addr, audioCandidate.address.c_str(), sizeof(md->addr));
		} else if (md->streams[i].type == SalVideo && videoCandidate.port != 0) {
			strncpy(md->streams[i].rtp_addr, videoCandidate.address.c_str(), sizeof(md->streams[i].rtp_addr));
			md->streams[i].rtp_port = videoCandidate.port;
		} else if (md->streams[i].type == SalText && textCandidate.port != 0) {
			strncpy(md->streams[i].rtp_addr, textCandidate.address.c_str(), sizeof(md->streams[i].rtp_addr));
			md->streams[i].rtp_port = textCandidate.port;
		}
	}
}

// -----------------------------------------------------------------------------

ortp_socket_t StunClient::createStunSocket (int localPort) {
	if (localPort < 0)
		return -1;
	ortp_socket_t sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		lError() << "Fail to create socket";
		return -1;
	}
	struct sockaddr_in laddr;
	memset(&laddr, 0, sizeof(laddr));
	laddr.sin_family = AF_INET;
	laddr.sin_addr.s_addr = INADDR_ANY;
	laddr.sin_port = htons(static_cast<uint16_t>(localPort));
	if (::bind(sock, (struct sockaddr *)&laddr, sizeof(laddr)) < 0) {
		lError() << "Bind socket to 0.0.0.0:" << localPort << " failed: " << getSocketError();
		close_socket(sock);
		return -1;
	}
	int optval = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (SOCKET_OPTION_VALUE)&optval, sizeof(optval)) < 0)
		lWarning() << "Fail to set SO_REUSEADDR";
	set_non_blocking_socket(sock);
	return sock;
}

int StunClient::recvStunResponse (ortp_socket_t sock, Candidate &candidate, int &id) {
	char buf[MS_STUN_MAX_MESSAGE_SIZE];
	int len = MS_STUN_MAX_MESSAGE_SIZE;

	len = static_cast<int>(recv(sock, buf, static_cast<size_t>(len), 0));
	if (len > 0) {
		struct in_addr ia;
		MSStunMessage *resp = ms_stun_message_create_from_buffer_parsing((uint8_t *)buf, (ssize_t)len);
		if (resp) {
			UInt96 trId = ms_stun_message_get_tr_id(resp);
			id = trId.octet[0];
			const MSStunAddress *stunAddr = ms_stun_message_get_xor_mapped_address(resp);
			if (stunAddr) {
				candidate.port = stunAddr->ip.v4.port;
				ia.s_addr = htonl(stunAddr->ip.v4.addr);
			} else {
				stunAddr = ms_stun_message_get_mapped_address(resp);
				if (stunAddr) {
					candidate.port = stunAddr->ip.v4.port;
					ia.s_addr = htonl(stunAddr->ip.v4.addr);
				} else
					len = -1;
			}
			if (len > 0)
				candidate.address = inet_ntoa(ia);
		}
	}
	return len;
}

int StunClient::sendStunRequest (
	ortp_socket_t sock,
	const struct sockaddr *server,
	socklen_t addrlen,
	int id,
	bool changeAddr
) {
	MSStunMessage *req = ms_stun_binding_request_create();
	UInt96 trId = ms_stun_message_get_tr_id(req);
	trId.octet[0] = static_cast<unsigned char>(id);
	ms_stun_message_set_tr_id(req, trId);
	ms_stun_message_enable_change_ip(req, changeAddr);
	ms_stun_message_enable_change_port(req, changeAddr);
	int err = 0;
	char *buf = nullptr;
	size_t len = ms_stun_message_encode(req, &buf);
	if (len <= 0) {
		lError() << "Failed to encode STUN message";
		err = -1;
	} else {
		err = static_cast<int>(bctbx_sendto(sock, buf, len, 0, server, addrlen));
		if (err < 0) {
			lError() << "sendto failed: " << strerror(errno);
			err = -1;
		}
	}
	if (buf)
		ms_free(buf);
	ms_free(req);
	return err;
}

LINPHONE_END_NAMESPACE
