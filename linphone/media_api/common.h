#ifndef COMMON_H
#define COMMON_H


#include "media_api.h"
#include <glib.h>

#define api_trace g_message
#define api_error g_error
#define api_warn g_warning

#define MEDIA_FLOW_DUPLEX 1
#define MEDIA_FLOW_HALF_DUPLEX 2

//Mediaflow type
#define MEDIA_FLOW_VIDEO 1
#define MEDIA_FLOW_VOICE 2

//Mediaflow protocols
#define MEDIA_RTP 1
#define MEDIA_OSS 2
#define MEDIA_ALSA 3
#define MEDIA_FILE 4

//Mediaflow codec function
#define MEDIA_API_DECODER 1
#define MEDIA_API_ENCODER 2

#endif


