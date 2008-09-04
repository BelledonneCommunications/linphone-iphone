/*
  The oRTP library is an RTP (Realtime Transport Protocol - rfc3550) stack.
  Copyright (C) 2001  Simon MORLAT simon.morlat@linphone.org

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef RTCP_H
#define RTCP_H

#include <ortp/port.h>

#define RTCP_MAX_RECV_BUFSIZE 1024

#define RTCP_SENDER_INFO_SIZE 20
#define RTCP_REPORT_BLOCK_SIZE 24
#define RTCP_COMMON_HEADER_SIZE 4
#define RTCP_SSRC_FIELD_SIZE 4

#ifdef __cplusplus
extern "C"{
#endif

/* RTCP common header */

typedef enum {
    RTCP_SR	= 200,
    RTCP_RR	= 201,
    RTCP_SDES	= 202,
    RTCP_BYE	= 203,
    RTCP_APP	= 204
} rtcp_type_t;
 
 
typedef struct rtcp_common_header
{
#ifdef ORTP_BIGENDIAN
        uint16_t version:2;
        uint16_t padbit:1;
        uint16_t rc:5;
        uint16_t packet_type:8;
#else
        uint16_t rc:5;
        uint16_t padbit:1;
        uint16_t version:2;
	uint16_t packet_type:8;
#endif
        uint16_t length:16;
} rtcp_common_header_t;

#define rtcp_common_header_set_version(ch,v) (ch)->version=v
#define rtcp_common_header_set_padbit(ch,p) (ch)->padbit=p
#define rtcp_common_header_set_rc(ch,rc) (ch)->rc=rc
#define rtcp_common_header_set_packet_type(ch,pt) (ch)->packet_type=pt
#define rtcp_common_header_set_length(ch,l)	(ch)->length=htons(l)

#define rtcp_common_header_get_version(ch) ((ch)->version)
#define rtcp_common_header_get padbit(ch) ((ch)->padbit)
#define rtcp_common_header_get_rc(ch) ((ch)->rc)
#define rtcp_common_header_get_packet_type(ch) ((ch)->packet_type)
#define rtcp_common_header_get_length(ch)	ntohs((ch)->length)


/* SR or RR  packets */

typedef struct sender_info
{
        uint32_t ntp_timestamp_msw;
        uint32_t ntp_timestamp_lsw;
        uint32_t rtp_timestamp;
        uint32_t senders_packet_count;
        uint32_t senders_octet_count;
} sender_info_t;

uint64_t sender_info_get_ntp_timestamp(const sender_info_t *si);
#define sender_info_get_rtp_timestamp(si)	((si)->rtp_timestamp)
#define sender_info_get_packet_count(si) \
	ntohl((si)->senders_packet_count)
#define sender_info_get_octet_count(si) \
	ntohl((si)->senders_octet_count)


typedef struct report_block
{
        uint32_t ssrc;
        uint32_t fl_cnpl;/*fraction lost + cumulative number of packet lost*/
        uint32_t ext_high_seq_num_rec; /*extended highest sequence number received */
        uint32_t interarrival_jitter;
        uint32_t lsr; /*last SR */
        uint32_t delay_snc_last_sr; /*delay since last sr*/
} report_block_t;

#define report_block_get_ssrc(rb) \
	ntohl((rb)->ssrc)
#define report_block_get_fraction_lost(rb) \
	(((uint32_t)ntohl((rb)->fl_cnpl))>>24)
#define report_block_get_cum_packet_loss(rb) \
	(((uint32_t)ntohl((rb)->fl_cnpl)) & 0xFFFFFF)
#define report_block_get_high_ext_seq(rb) \
	ntohl(((report_block_t*)(rb))->ext_high_seq_num_rec)
#define report_block_get_interarrival_jitter(rb) \
	ntohl(((report_block_t*)(rb))->interarrival_jitter)
#define report_block_get_last_SR_time(rb) \
	ntohl(((report_block_t*)(rb))->lsr)
#define report_block_get_last_SR_delay(rb) \
	ntohl(((report_block_t*)(rb))->delay_snc_last_sr)

#define report_block_set_fraction_lost(rb,fl)\
	((rb)->fl_cnpl)=htonl( (ntohl((rb)->fl_cnpl) & 0xFFFFFF) | (((fl) & 0xFF)<<24))

#define report_block_set_cum_packet_lost(rb,cpl)\
	((rb)->fl_cnpl)=htonl( (ntohl((rb)->fl_cnpl) & 0xFF000000) | (((cpl) & 0xFFFFFF)))

/* SDES packets */

typedef enum {
    RTCP_SDES_END		= 0,
    RTCP_SDES_CNAME 	= 1,
    RTCP_SDES_NAME	= 2,
    RTCP_SDES_EMAIL	= 3,
    RTCP_SDES_PHONE	= 4,
    RTCP_SDES_LOC		= 5,
    RTCP_SDES_TOOL	= 6,
    RTCP_SDES_NOTE	= 7,
    RTCP_SDES_PRIV		= 8,
    RTCP_SDES_MAX		= 9
} rtcp_sdes_type_t;

typedef struct sdes_chunk
{
	uint32_t csrc;
} sdes_chunk_t;


#define sdes_chunk_get_csrc(c)	ntohl((c)->csrc)

typedef struct sdes_item
{
	uint8_t item_type;
	uint8_t len;
	char content[1];	
} sdes_item_t;

#define RTCP_SDES_MAX_STRING_SIZE 255
#define RTCP_SDES_ITEM_HEADER_SIZE 2
#define RTCP_SDES_CHUNK_DEFAULT_SIZE 1024
#define RTCP_SDES_CHUNK_HEADER_SIZE (sizeof(sdes_chunk_t))

/* RTCP bye packet */

typedef struct rtcp_bye_reason
{
	uint8_t len;
	char content[1];
} rtcp_bye_reason_t;
 
typedef struct rtcp_bye
{
	rtcp_common_header_t ch;
	uint32_t ssrc[1];  /* the bye may contain several ssrc/csrc */
} rtcp_bye_t;
#define RTCP_BYE_HEADER_SIZE sizeof(rtcp_bye_t)
#define RTCP_BYE_REASON_MAX_STRING_SIZE 255



typedef struct rtcp_sr{
	rtcp_common_header_t ch;
	uint32_t ssrc;
	sender_info_t si;
	report_block_t rb[1];
} rtcp_sr_t;

typedef struct rtcp_rr{
	rtcp_common_header_t ch;
	uint32_t ssrc;
	report_block_t rb[1];
} rtcp_rr_t;

typedef struct rtcp_app{
	rtcp_common_header_t ch;
	uint32_t ssrc;
	char name[4];
} rtcp_app_t;

struct _RtpSession;
void rtp_session_rtcp_process_send(struct _RtpSession *s);
void rtp_session_rtcp_process_recv(struct _RtpSession *s);

#define RTCP_DEFAULT_REPORT_INTERVAL 5


/* packet parsing api */

/*in case of coumpound packet, set read pointer of m to the beginning of the next RTCP
packet */
bool_t rtcp_next_packet(mblk_t *m);
/* put the read pointer at the first RTCP packet of the compound packet (as before any previous calls ot rtcp_next_packet() */
void rtcp_rewind(mblk_t *m);
/* get common header*/
const rtcp_common_header_t * rtcp_get_common_header(const mblk_t *m);

/*Sender Report accessors */
/* check if this packet is a SR and if it is correct */
bool_t rtcp_is_SR(const mblk_t *m);
uint32_t rtcp_SR_get_ssrc(const mblk_t *m);
const sender_info_t * rtcp_SR_get_sender_info(const mblk_t *m);
const report_block_t * rtcp_SR_get_report_block(const mblk_t *m, int idx);

/*Receiver report accessors*/
bool_t rtcp_is_RR(const mblk_t *m);
uint32_t rtcp_RR_get_ssrc(const mblk_t *m);
const report_block_t * rtcp_RR_get_report_block(const mblk_t *m,int idx);

/*SDES accessors */
bool_t rtcp_is_SDES(const mblk_t *m);
typedef void (*SdesItemFoundCallback)(void *user_data, uint32_t csrc, rtcp_sdes_type_t t, const char *content, uint8_t content_len); 
void rtcp_sdes_parse(const mblk_t *m, SdesItemFoundCallback cb, void *user_data);

/*BYE accessors */
bool_t rtcp_is_BYE(const mblk_t *m);
bool_t rtcp_BYE_get_ssrc(const mblk_t *m, int idx, uint32_t *ssrc);
bool_t rtcp_BYE_get_reason(const mblk_t *m, const char **reason, int *reason_len);

/*APP accessors */
bool_t rtcp_is_APP(const mblk_t *m);
int rtcp_APP_get_subtype(const mblk_t *m);
uint32_t rtcp_APP_get_ssrc(const mblk_t *m);
/* name argument is supposed to be at least 4 characters (note: no '\0' written)*/
void rtcp_APP_get_name(const mblk_t *m, char *name); 
/* retrieve the data. when returning, data points directly into the mblk_t */
void rtcp_APP_get_data(const mblk_t *m, uint8_t **data, int *len);


#ifdef __cplusplus
}
#endif

#endif
