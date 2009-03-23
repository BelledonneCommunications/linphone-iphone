/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef msfilter_h
#define msfilter_h

#include "mscommon.h"
#include "msqueue.h"
#include "allfilters.h"

/**
 * @file msfilter.h
 * @brief mediastreamer2 msfilter.h include file
 *
 * This file provide the API needed to create, link,
 * unlink, find and destroy filter.
 *
 * It also provides definitions if you wish to implement
 * your own filters.
 *
 */

/**
 * @defgroup mediastreamer2_filter Filter API - manage mediastreamer2 filters.
 * @ingroup mediastreamer2_api
 * @{
 */

/**
 * Structure for filter's methods (init, preprocess, process, postprocess, uninit).
 * @var MSFilterFunc
 */
typedef void (*MSFilterFunc)(struct _MSFilter *f);

/**
 * Structure for filter's methods used to set filter's options.
 * @var MSFilterMethodFunc
 */
typedef int (*MSFilterMethodFunc)(struct _MSFilter *f, void *arg);

/**
 * Structure for filter's methods used as a callback to notify events.
 * @var MSFilterNotifyFunc
 */
typedef void (*MSFilterNotifyFunc)(void *userdata , unsigned int id, void *arg);

struct _MSFilterMethod{
	int id;
	MSFilterMethodFunc method;
};


/**
 * Structure for holding filter's methods to set filter's options.
 * @var MSFilterMethod
 */
typedef struct _MSFilterMethod MSFilterMethod;

enum _MSFilterCategory{
	MS_FILTER_OTHER,
	MS_FILTER_ENCODER,
	MS_FILTER_DECODER
};

/**
 * Structure to describe filter's category.
 * <PRE>
 *     MS_FILTER_OTHER
 *     MS_FILTER_ENCODER
 *     MS_FILTER_DECODER
 * </PRE>
 * @var MSFilterCategory
 */
typedef enum _MSFilterCategory MSFilterCategory;

enum _MSFilterFlags{
	MS_FILTER_IS_PUMP = 1
};

typedef enum _MSFilterFlags MSFilterFlags;

struct _MSFilterDesc{
	MSFilterId id;	/* the id declared in allfilters.h */
	const char *name; /* filter name */
	const char *text; /*some descriptive text*/
	MSFilterCategory category;
	const char *enc_fmt; /* must be set if MS_FILTER_ENCODER/MS_FILTER_DECODER */
	int ninputs; /*number of inputs */
	int noutputs; /*number of outputs */
	MSFilterFunc init;
	MSFilterFunc preprocess;	/* called once before processing */
	MSFilterFunc process;		/* called every tick to do the filter's job*/
	MSFilterFunc postprocess;	/*called once after processing */
	MSFilterFunc uninit;
	MSFilterMethod *methods;
	unsigned int flags;
};

/**
 * Structure for filter's description.
 * @var MSFilterDesc
 */
typedef struct _MSFilterDesc MSFilterDesc;

struct _MSFilter{
	MSFilterDesc *desc;
	/*protected attributes */
	ms_mutex_t lock;
	MSQueue **inputs;
	MSQueue **outputs;
	MSFilterNotifyFunc notify;
	void *notify_ud;
	void *data;
	struct _MSTicker *ticker;
	/*private attributes */
	uint32_t last_tick;
	bool_t seen;
};


/**
 * Structure to create/link/unlink/destroy filter's object.
 * @var MSFilter
 */
typedef struct _MSFilter MSFilter;

struct _MSConnectionPoint{
	MSFilter *filter;
	int pin;
};

/**
 * Structure that represents a connection point of a MSFilter
 * @var MSConnectionPoint
 */
typedef struct _MSConnectionPoint MSConnectionPoint;

struct _MSConnectionHelper{
	MSConnectionPoint last;
};

/**
 * Structure that holds data when using the ms_connection_helper_* functions.
 * @var MSConnectionHelper
**/
typedef struct _MSConnectionHelper MSConnectionHelper;


#ifdef __cplusplus
extern "C"{
#endif

/**
 * Register a filter description. (plugins use only!)
 *
 * When you build your own plugin, this method will
 * add the encoder or decoder to the internal list
 * of supported codec. Then, this plugin can be used
 * transparently from the application.
 *
 * ms_filter_get_encoder, ms_filter_get_decoder,
 * ms_filter_create_encoder, ms_filter_create_decoder
 * and ms_filter_codec_supported
 * can then be used as if the codec was internally.
 * supported.
 *
 * @param desc    a filter description.
 */
void ms_filter_register(MSFilterDesc *desc);

/**
 * Retrieve encoders according to codec name.
 *
 * Internal supported codecs:
 *    PCMU, PCMA, speex, gsm
 * Existing Public plugins:
 *    iLBC
 *
 * @param mime    A string indicating the codec.
 *
 * Returns: a MSFilterDesc if successfull, NULL otherwise.
 */
MSFilterDesc * ms_filter_get_encoder(const char *mime);

/**
 * Retrieve decoders according to codec name.
 *
 * Internal supported codecs:
 *    PCMU, PCMA, speex, gsm
 * Existing Public plugins:
 *    iLBC
 *
 * @param mime    A string indicating the codec.
 *
 * Returns: a MSFilterDesc if successfull, NULL otherwise.
 */
MSFilterDesc * ms_filter_get_decoder(const char *mime);

/**
 * Create encoder filter according to codec name.
 *
 * Internal supported codecs:
 *    PCMU, PCMA, speex, gsm
 * Existing Public plugins:
 *    iLBC
 *
 * @param mime    A string indicating the codec.
 *
 * Returns: a MSFilter if successfull, NULL otherwise.
 */
MSFilter * ms_filter_create_encoder(const char *mime);

/**
 * Create decoder filter according to codec name.
 *
 * Internal supported codecs:
 *    PCMU, PCMA, speex, gsm
 * Existing Public plugins:
 *    iLBC
 *
 * @param mime    A string indicating the codec.
 *
 * Returns: a MSFilter if successfull, NULL otherwise.
 */
MSFilter * ms_filter_create_decoder(const char *mime);

/**
 * Check if a encode or decode filter exists for a codec name.
 *
 * Internal supported codecs:
 *    PCMU, PCMA, speex, gsm
 * Existing Public plugins:
 *    iLBC
 *
 * @param mime    A string indicating the codec.
 *
 * Returns: TRUE if successfull, FALSE otherwise.
 */
bool_t ms_filter_codec_supported(const char *mime);

/**
 * Create decoder filter according to a filter's MSFilterId.
 *
 * @param id     A MSFilterId identifier for the filter.
 *
 * Returns: a MSFilter if successfull, NULL otherwise.
 */
MSFilter *ms_filter_new(MSFilterId id);

/**
 * Create decoder filter according to a filter's name.
 *
 * @param name   A name for the filter.
 *
 * Returns: a MSFilter if successfull, NULL otherwise.
 */
MSFilter *ms_filter_new_from_name(const char *name);

/**
 * Create decoder filter according to a filter's description.
 *
 * The primary use is to create your own filter's in your
 * application and avoid registration inside mediastreamer2.
 * 
 * @param desc   A MSFilterDesc for the filter.
 *
 * Returns: a MSFilter if successfull, NULL otherwise.
 */
MSFilter *ms_filter_new_from_desc(MSFilterDesc *desc);

/**
 * Link one OUTPUT pin from a filter to an INPUT pin of another filter.
 *
 * All data coming from the OUTPUT pin of one filter will be distributed
 * to the INPUT pin of the second filter.
 *
 * @param f1   A MSFilter object containing the OUTPUT pin
 * @param pin1 An index of an OUTPUT pin.
 * @param f2   A MSFilter object containing the INPUT pin
 * @param pin2 An index of an INPUT pin.
 *
 * Returns: 0 if sucessful, -1 otherwise.
 */
int ms_filter_link(MSFilter *f1, int pin1, MSFilter *f2, int pin2);

/**
 * Unlink one OUTPUT pin from a filter to an INPUT pin of another filter.
 *
 * @param f1   A MSFilter object containing the OUTPUT pin
 * @param pin1 An index of an OUTPUT pin.
 * @param f2   A MSFilter object containing the INPUT pin
 * @param pin2 An index of an INPUT pin.
 *
 * Returns: 0 if sucessful, -1 otherwise.
 */
int ms_filter_unlink(MSFilter *f1, int pin1, MSFilter *f2, int pin2);

/**
 * Call a filter's method to set or get options.
 *
 * @param f    A MSFilter object.
 * @param id   A private filter ID for the option.
 * @param arg  A private user data for the filter.
 *
 * Returns: 0 if successfull, -1 otherwise.
 */
int ms_filter_call_method(MSFilter *f, unsigned int id, void *arg);

/**
 * Call a filter's method to set options.
 *
 * @param f    A MSFilter object.
 * @param id   A private filter ID for the option.
 *
 * Returns: 0 if successfull, -1 otherwise.
 */
int ms_filter_call_method_noarg(MSFilter *f, unsigned int id);

/**
 * Set a callback on filter's to be informed of private filter's event.
 *
 * @param f        A MSFilter object.
 * @param fn       A MSFilterNotifyFunc that will be called.
 * @param userdata A pointer to private data.
 *
 * Returns: 0 if successfull, -1 otherwise.
 */
void ms_filter_set_notify_callback(MSFilter *f, MSFilterNotifyFunc fn, void *userdata);

/**
 * Get MSFilterId's filter.
 *
 * @param f        A MSFilter object.
 *
 * Returns: MSFilterId if successfull, -1 otherwise.
 */
MSFilterId ms_filter_get_id(MSFilter *f);

/**
 * Destroy a filter object.
 *
 * @param f        A MSFilter object.
 *
 */
void ms_filter_destroy(MSFilter *f);

/**
 * Initialize a MSConnectionHelper.
 *
 * @param h A MSConnectionHelper, usually (but not necessarily) on stack
 *
**/
void ms_connection_helper_start(MSConnectionHelper *h);

/**
 * \brief Enter a MSFilter to be connected into the MSConnectionHelper object.
 * 
 * This functions enters a MSFilter to be connected into the MSConnectionHelper
 * object and connects it to the last entered if not the first one.
 * The MSConnectionHelper is useful to reduce the amount of code necessary to create graphs in case 
 * the connections are made in an ordered manner and some filters are present conditionally in graphs.
 * For example, instead of writing
 * \code
 * ms_filter_link(f1,0,f2,1);
 * ms_filter_link(f2,0,f3,0);
 * ms_filter_link(f3,1,f4,0);
 * \endcode
 * You can write:
 * \code
 * MSConnectionHelper h;
 * ms_connection_helper_start(&h);
 * ms_connection_helper_link(&h,f1,-1,0);
 * ms_connection_helper_link(&h,f2,1,0);
 * ms_connection_helper_link(&h,f3,0,1);
 * ms_connection_helper_link(&h,f4,0,-1);
 * \endcode
 * Which is a bit longer to write here, but now imagine f2 needs to be present in the graph only
 * in certain conditions: in the first case you have rewrite the two first lines, in the second case
 * you just need to replace the fourth line by:
 * \code
 * if (my_condition) ms_connection_helper_link(&h,f2,1,0);
 * \endcode
 *
 * @param h a connection helper 
 * @param f a MSFilter
 * @param inpin an input pin number with which the MSFilter needs to connect to previously entered MSFilter
 * @param outpin an output pin number with which the MSFilter needs to be connected to the next entered MSFilter
 * 
 * Returns: the return value of ms_filter_link() that is called internally to this function.
**/
int ms_connection_helper_link(MSConnectionHelper *h, MSFilter *f, int inpin, int outpin);


/**
 * \brief Enter a MSFilter to be disconnected into the MSConnectionHelper object.
 * Process exactly the same way as ms_connection_helper_link() but calls ms_filter_unlink() on the 
 * entered filters.
**/
int ms_connection_helper_unlink(MSConnectionHelper *h, MSFilter *f, int inpin, int outpin);

/* I define the id taking the lower bits of the address of the MSFilterDesc object,
the method index (_cnt_) and the argument size */
/* I hope using this to avoid type mismatch (calling a method on the wrong filter)*/
#define MS_FILTER_METHOD_ID(_id_,_cnt_,_argsize_) \
	(  (((unsigned long)(_id_)) & 0xFFFF)<<16 | (_cnt_<<8) | (_argsize_ & 0xFF ))

#define MS_FILTER_METHOD(_id_,_count_,_argtype_) \
	MS_FILTER_METHOD_ID(_id_,_count_,sizeof(_argtype_))

#define MS_FILTER_METHOD_NO_ARG(_id_,_count_) \
	MS_FILTER_METHOD_ID(_id_,_count_,0)


#define MS_FILTER_BASE_METHOD(_count_,_argtype_) \
	MS_FILTER_METHOD_ID(MS_FILTER_BASE_ID,_count_,sizeof(_argtype_))

#define MS_FILTER_BASE_METHOD_NO_ARG(_count_) \
	MS_FILTER_METHOD_ID(MS_FILTER_BASE_ID,_count_,0)

#define MS_FILTER_EVENT(_id_,_count_,_argtype_) \
	MS_FILTER_METHOD_ID(_id_,_count_,sizeof(_argtype_))

#define MS_FILTER_EVENT_NO_ARG(_id_,_count_)\
	MS_FILTER_METHOD_ID(_id_,_count_,0)

/* some MSFilter base generic methods:*/
#define MS_FILTER_SET_SAMPLE_RATE	MS_FILTER_BASE_METHOD(0,int)
#define MS_FILTER_GET_SAMPLE_RATE	MS_FILTER_BASE_METHOD(1,int)
#define MS_FILTER_SET_BITRATE		MS_FILTER_BASE_METHOD(2,int)
#define MS_FILTER_GET_BITRATE		MS_FILTER_BASE_METHOD(3,int)
#define MS_FILTER_GET_NCHANNELS		MS_FILTER_BASE_METHOD(5,int)
#define MS_FILTER_SET_NCHANNELS		MS_FILTER_BASE_METHOD(6,int)
#define MS_FILTER_ADD_FMTP		MS_FILTER_BASE_METHOD(7,const char)
#define MS_FILTER_ADD_ATTR		MS_FILTER_BASE_METHOD(8,const char)
#define MS_FILTER_SET_MTU		MS_FILTER_BASE_METHOD(9,int)
#define MS_FILTER_GET_MTU		MS_FILTER_BASE_METHOD(10,int)


/* more specific methods: to be moved into implementation specific header files*/
#define MS_FILTER_SET_FRAMESIZE 	MS_FILTER_BASE_METHOD(11,int)
#define MS_FILTER_SET_FILTERLENGTH 	MS_FILTER_BASE_METHOD(12,int)
#define MS_FILTER_SET_OUTPUT_SAMPLE_RATE MS_FILTER_BASE_METHOD(13,int)
#define MS_FILTER_ENABLE_DIRECTMODE	MS_FILTER_BASE_METHOD(14,int)
#define MS_FILTER_ENABLE_VAD		MS_FILTER_BASE_METHOD(15,int)
#define MS_FILTER_GET_STAT_DISCARDED	MS_FILTER_BASE_METHOD(16,int)
#define MS_FILTER_GET_STAT_MISSED	MS_FILTER_BASE_METHOD(17,int)
#define MS_FILTER_GET_STAT_INPUT	MS_FILTER_BASE_METHOD(18,int)
#define MS_FILTER_GET_STAT_OUTPUT	MS_FILTER_BASE_METHOD(19,int)
#define MS_FILTER_ENABLE_AGC 		MS_FILTER_BASE_METHOD(20,int)
#define MS_FILTER_SET_PLAYBACKDELAY MS_FILTER_BASE_METHOD(21,int)
#define MS_FILTER_ENABLE_HALFDUPLEX MS_FILTER_BASE_METHOD(22,int)
#define MS_FILTER_SET_VAD_PROB_START MS_FILTER_BASE_METHOD(23,int)
#define MS_FILTER_SET_VAD_PROB_CONTINUE MS_FILTER_BASE_METHOD(24,int)
#define MS_FILTER_SET_MAX_GAIN  MS_FILTER_BASE_METHOD(25,int)

#define MS_CONF_SPEEX_PREPROCESS_MIC	MS_FILTER_EVENT(MS_CONF_ID, 1, void*)
#define MS_CONF_CHANNEL_VOLUME	MS_FILTER_EVENT(MS_CONF_ID, 3, void*)

#define MS_SPEEX_EC_PREPROCESS_MIC	MS_FILTER_EVENT(MS_SPEEX_EC_ID, 1, void*)
#define MS_SPEEX_EC_ECHO_STATE	MS_FILTER_EVENT(MS_SPEEX_EC_ID, 2, void*)
/** @} */

/*private methods*/
void ms_filter_process(MSFilter *f);
void ms_filter_preprocess(MSFilter *f, struct _MSTicker *t);
void ms_filter_postprocess(MSFilter *f);
bool_t ms_filter_inputs_have_data(MSFilter *f);
void ms_filter_notify(MSFilter *f, unsigned int id, void *arg);
void ms_filter_notify_no_arg(MSFilter *f, unsigned int id);
#define ms_filter_lock(f)	ms_mutex_lock(&(f)->lock)
#define ms_filter_unlock(f)	ms_mutex_unlock(&(f)->lock)
void ms_filter_unregister_all(void);

#ifdef __cplusplus
}
#endif

/* used by awk script in Makefile.am to generate alldescs.c */
#define MS_FILTER_DESC_EXPORT(desc)

/* xgettext markup */
#define N_(String) String

#endif
