/*
  The mediastreamer library aims at providing modular media processing and I/O
	for linphone, but also for any telephony application.
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


#ifndef MSFILTER_H
#define MSFILTER_H

#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#ifdef HAVE_GLIB
#include <glib.h>
#else
#undef VERSION
#undef PACKAGE
#include <uglib.h>
#endif

#include <string.h>
#include "msutils.h"
#include "msfifo.h"
#include "msqueue.h"

struct _MSFilter;
/*this is the abstract object and class for all filter types*/
typedef gint (*MSFilterNotifyFunc)(struct _MSFilter*, gint event, gpointer arg, gpointer userdata);

struct _MSFilter
{
	struct _MSFilterClass *klass;
	GMutex *lock;
	guchar finputs;   /* number of connected fifo inputs*/
	guchar foutputs;  /* number of connected fifo outputs*/
	guchar qinputs;   /* number of connected queue inputs*/
	guchar qoutputs;  /* number of connected queue outputs*/
	gint min_fifo_size; /* set when linking*/
	gint r_mingran;				/* read minimum granularity (for fifos).
					It can be zero so that the filter can accept any size of reading data*/
	MSFifo **infifos; /*pointer to a table of pointer to input fifos*/
	MSFifo **outfifos;  /*pointer to a table of pointer to output fifos*/
	MSQueue **inqueues;  /*pointer to a table of pointer to input queues*/
	MSQueue **outqueues;  /*pointer to a table of pointer to output queues*/
	MSFilterNotifyFunc notify_event;
	gpointer userdata;
};

typedef struct _MSFilter MSFilter;

typedef enum{
	MS_FILTER_PROPERTY_FREQ,	/* value is int */
	MS_FILTER_PROPERTY_BITRATE, /*value is int */
	MS_FILTER_PROPERTY_CHANNELS,/*value is int */
	MS_FILTER_PROPERTY_FMTP    /* value is string */
}MSFilterProperty;

#define MS_FILTER_PROPERTY_STRING_MAX_SIZE 256

typedef MSFilter * (*MSFilterNewFunc)(void);
typedef  void (*MSFilterProcessFunc)(MSFilter *);
typedef  void (*MSFilterDestroyFunc)(MSFilter *);
typedef  int (*MSFilterPropertyFunc)(MSFilter *,int ,void*);
typedef  void (*MSFilterSetupFunc)(MSFilter *, void *);  /*2nd arg is the sync */

typedef struct _MSFilterClass
{
	struct _MSFilterInfo *info;	/*pointer to a filter_info */
	gchar *name;
	guchar max_finputs;   /* maximum number of fifo inputs*/
	guchar max_foutputs;  /* maximum number of fifo outputs*/
	guchar max_qinputs;   /* maximum number of queue inputs*/
	guchar max_qoutputs;  /* maximum number of queue outputs*/
	gint r_maxgran;       /* read maximum granularity (for fifos)*/
	gint w_maxgran;				/* write maximum granularity (for fifos)*/
	gint r_offset;				/* size of kept samples behind read pointer (for fifos)*/
	gint w_offset;				/* size of kept samples behind write pointer (for fifos)*/
	MSFilterPropertyFunc set_property;
	MSFilterPropertyFunc get_property;
	MSFilterSetupFunc setup;	/* called when attaching to sync */
	void (*process)(MSFilter *filter);
	MSFilterSetupFunc unsetup;	/* called when detaching from sync */
	void (*destroy)(MSFilter *filter);
	guint attributes;
#define FILTER_HAS_FIFOS (0x0001)
#define FILTER_HAS_QUEUES (0x0001<<1)
#define FILTER_IS_SOURCE (0x0001<<2)
#define FILTER_IS_SINK (0x0001<<3)
#define FILTER_CAN_SYNC (0x0001<<4)
	guint ref_count; /*number of object using the class*/
} MSFilterClass;



#define MS_FILTER(obj) ((MSFilter*)obj)
#define MS_FILTER_CLASS(klass) ((MSFilterClass*)klass)
#define MS_FILTER_GET_CLASS(obj) ((MSFilterClass*)((MS_FILTER(obj)->klass)))

void ms_filter_class_init(MSFilterClass *filterclass);
void ms_filter_init(MSFilter *filter);

#define ms_filter_class_set_attr(filter,flag) ((filter)->attributes|=(flag))
#define ms_filter_class_unset_attr(filter,flag) ((filter)->attributes&=~(flag))

#define ms_filter_class_set_name(__klass,__name)  (__klass)->name=g_strdup((__name))
#define ms_filter_class_set_info(_klass,_info)	(_klass)->info=(_info)
/* public*/

#define  ms_filter_process(filter) ((filter)->klass->process((filter)))

#define ms_filter_lock(filter)		g_mutex_lock((filter)->lock)
#define ms_filter_unlock(filter)	g_mutex_unlock((filter)->lock)
/* low level connect functions */
int ms_filter_link(MSFilter *m1, gint pin1, MSFilter *m2,gint pin2, gint linktype);
int ms_filter_unlink(MSFilter *m1, gint pin1, MSFilter *m2,gint pin2,gint linktype);

/* high level connect functions */
int ms_filter_add_link(MSFilter *m1, MSFilter *m2);
int ms_filter_remove_links(MSFilter *m1, MSFilter *m2);

void ms_filter_set_notify_func(MSFilter* filter,MSFilterNotifyFunc func, gpointer userdata);
void ms_filter_notify_event(MSFilter *filter,gint event, gpointer arg);

int ms_filter_set_property(MSFilter *f,MSFilterProperty property, void *value);
int ms_filter_get_property(MSFilter *f,MSFilterProperty property, void *value);


gint ms_filter_fifos_have_data(MSFilter *f);
gint ms_filter_queues_have_data(MSFilter *f);

void ms_filter_uninit(MSFilter *obj);
void ms_filter_destroy(MSFilter *f);

#define ms_filter_get_mingran(f) ((f)->r_mingran)
#define ms_filter_set_mingran(f,gran) ((f)->r_mingran=(gran))

#define LINK_DEFAULT 0
#define LINK_FIFO 1
#define LINK_QUEUE 2


#define MSFILTER_VERSION(a,b,c) (((a)<<2)|((b)<<1)|(c))

enum _MSFilterType
{
	MS_FILTER_DISK_IO,
	MS_FILTER_AUDIO_CODEC,
	MS_FILTER_VIDEO_CODEC,
	MS_FILTER_NET_IO,
	MS_FILTER_VIDEO_IO,
	MS_FILTER_AUDIO_IO,
	MS_FILTER_OTHER
};

typedef enum _MSFilterType MSFilterType;


/* find the first codec in the left part of the stream */
MSFilter * ms_filter_search_upstream_by_type(MSFilter *f,MSFilterType type);

struct _MSFilterInfo
{
	gchar *name;
	gint version;
	MSFilterType type;
	MSFilterNewFunc constructor;
	char *description;  /*some textual information*/
};

typedef struct _MSFilterInfo MSFilterInfo;

void ms_filter_register(MSFilterInfo *finfo);
void ms_filter_unregister(MSFilterInfo *finfo);
MSFilterInfo * ms_filter_get_by_name(const gchar *name);

MSFilter * ms_filter_new_with_name(const gchar *name);



extern GList *filter_list;
#define MS_FILTER_INFO(obj)	((MSFilterInfo*)obj)

void swap_buffer(gchar *buffer, gint len);


#endif
