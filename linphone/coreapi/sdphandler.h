 /*
  * Linphone is sip (RFC3261) compatible internet phone.
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Lesser General Public
  * License as published by the Free Software Foundation; either
  * version 2.1 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public
  * License along with this library; if not, write to the Free Software
  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
  */

#ifndef SDP_HANDLER_H
#define SDP_HANDLER_H

#include <osipparser2/sdp_message.h>
#include "linphonecore.h"

typedef struct _sdp_payload
{
	int line;	/* the index of the m= line */
	int pt;		/*payload type */
	int localport;
	int remoteport;
	int b_as_bandwidth;	/* application specific bandwidth */
	char *proto;
	char *c_nettype;
	char *c_addrtype;
	char *c_addr;
	char *c_addr_multicast_ttl;
	char *c_addr_multicast_int;
	char *a_rtpmap;
	char *a_fmtp;
	char *relay_host;
	int relay_port;
	char *relay_session_id;
	int a_ptime;
} sdp_payload_t;

typedef struct _sdp_context sdp_context_t;

typedef int (*sdp_handler_read_codec_func_t) (struct _sdp_context *,
											sdp_payload_t *);
typedef int (*sdp_handler_write_codec_func_t) (struct _sdp_context *);

typedef struct _sdp_handler
{
	sdp_handler_read_codec_func_t accept_audio_codecs;   /*from remote sdp */
	sdp_handler_read_codec_func_t accept_video_codecs;   /*from remote sdp */
	sdp_handler_write_codec_func_t set_audio_codecs;	/*to local sdp */
	sdp_handler_write_codec_func_t set_video_codecs;	/*to local sdp */
	sdp_handler_read_codec_func_t get_audio_codecs;	/*from incoming answer  */
	sdp_handler_read_codec_func_t get_video_codecs;	/*from incoming answer  */
} sdp_handler_t;


typedef enum _sdp_context_state
{
	SDP_CONTEXT_STATE_INIT,
	SDP_CONTEXT_STATE_NEGOCIATION_OPENED,
	SDP_CONTEXT_STATE_NEGOCIATION_CLOSED
} sdp_context_state_t;

struct _sdp_context
{
	sdp_handler_t *handler;
	char *localip;
	char *username;
	void *reference;
	sdp_message_t *offer;		/* the local sdp to be used for outgoing request */
	char *offerstr;
	sdp_message_t *answer;		/* the local sdp generated from an inc request */
	char *answerstr;
	char *relay;
	char *relay_session_id;
	int negoc_status;	/* in sip code */
	int incb;
	sdp_context_state_t state;
};

/* create a context for a sdp negociation: localip is the ip address to be used in the sdp message, can
be a firewall address.
It can be null when negociating for an incoming offer; In that case it will be guessed. */
sdp_context_t *sdp_handler_create_context(sdp_handler_t *handler, const char *localip, const char *username, const char *relay);
void sdp_context_set_user_pointer(sdp_context_t * ctx, void* up);
void *sdp_context_get_user_pointer(sdp_context_t * ctx);
void sdp_context_add_audio_payload( sdp_context_t * ctx, sdp_payload_t * payload);
void sdp_context_add_video_payload( sdp_context_t * ctx, sdp_payload_t * payload);
char * sdp_context_get_offer(sdp_context_t *ctx);
char * sdp_context_get_answer(sdp_context_t* ctx, sdp_message_t *remote_offer);
int sdp_context_get_status(sdp_context_t* ctx);
void sdp_context_read_answer(sdp_context_t *ctx, sdp_message_t *remote_answer);
void sdp_context_free(sdp_context_t *ctx);

int sdp_payload_init (sdp_payload_t * payload);
#endif
