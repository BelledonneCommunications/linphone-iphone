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



#ifndef SNDCARD_H
#define SNDCARD_H

#undef PACKAGE
#undef VERSION
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif
#undef PACKAGE
#undef VERSION

#ifdef HAVE_GLIB
#include <glib.h>
#else
#include <uglib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
	
/* the base class for all soundcards: SndCard */
struct _SndCard;
	
typedef int (*SndCardOpenFunc)(struct _SndCard*,int, int, int);
typedef void (*SndCardSetBlockingModeFunc)(struct _SndCard*, gboolean );
typedef void (*SndCardCloseFunc)(struct _SndCard*);
typedef gint (*SndCardIOFunc)(struct _SndCard*,char *,int);
typedef void (*SndCardDestroyFunc)(struct _SndCard*);
typedef gboolean (*SndCardPollFunc)(struct _SndCard*);
typedef gint (*SndCardMixerGetLevelFunc)(struct _SndCard*,gint);
typedef void (*SndCardMixerSetRecSourceFunc)(struct _SndCard*,gint);	
typedef void (*SndCardMixerSetLevelFunc)(struct _SndCard*,gint ,gint);
typedef struct _MSFilter * (*SndCardCreateFilterFunc)(struct _SndCard *);	

struct _SndCard
{
	gchar *card_name;          /* SB16 PCI for example */
	gint index;
	gint bsize;
	gint rate;
	gint stereo;
	gint bits;
	gint flags;
#define SND_CARD_FLAGS_OPENED 1
	SndCardOpenFunc _probe;
	SndCardOpenFunc _open_r;
	SndCardOpenFunc _open_w;
	SndCardSetBlockingModeFunc _set_blocking_mode;
	SndCardPollFunc _can_read;
	SndCardIOFunc _read;
	SndCardIOFunc _write;
	SndCardCloseFunc _close_r;
	SndCardCloseFunc _close_w;
	SndCardMixerGetLevelFunc _get_level;
	SndCardMixerSetLevelFunc _set_level;
	SndCardMixerSetRecSourceFunc _set_rec_source;
	SndCardCreateFilterFunc _create_read_filter;
	SndCardCreateFilterFunc _create_write_filter;
	SndCardDestroyFunc _destroy;
};


typedef struct _SndCard SndCard;
	
void snd_card_init(SndCard *obj);
void snd_card_uninit(SndCard *obj);
gint snd_card_probe(SndCard *obj, int bits, int stereo, int rate);
int snd_card_open_r(SndCard *obj, int bits, int stereo, int rate);
int snd_card_open_w(SndCard *obj, int bits, int stereo, int rate);
int snd_card_get_bsize(SndCard *obj);
gboolean snd_card_can_read(SndCard *obj);
int snd_card_read(SndCard *obj,char *buffer,int size);
int snd_card_write(SndCard *obj,char *buffer,int size);
void snd_card_set_blocking_mode(SndCard *obj,gboolean yesno);
void snd_card_close_r(SndCard *obj);
void snd_card_close_w(SndCard *obj);

void snd_card_set_rec_source(SndCard *obj, int source); /* source='l' or 'm'*/
void snd_card_set_level(SndCard *obj, int way, int level);
gint snd_card_get_level(SndCard *obj,int way);

const gchar *snd_card_get_identifier(SndCard *obj);

struct _MSFilter * snd_card_create_read_filter(SndCard *sndcard);
struct _MSFilter * snd_card_create_write_filter(SndCard *sndcard);


#define SND_CARD_LEVEL_GENERAL 1
#define SND_CARD_LEVEL_INPUT   2
#define SND_CARD_LEVEL_OUTPUT  3


int snd_card_destroy(SndCard *obj);

#define SND_CARD(obj) ((SndCard*)(obj))




/* SndCardManager */

#define MAX_SND_CARDS 20


struct _SndCardManager
{
	SndCard *cards[MAX_SND_CARDS];
};

typedef struct _SndCardManager SndCardManager;

void snd_card_manager_init(SndCardManager *manager);
SndCard * snd_card_manager_get_card(SndCardManager *manager,int index);
SndCard * snd_card_manager_get_card_with_string(SndCardManager *manager,const char *cardname,int *index);

extern SndCardManager *snd_card_manager;

#ifdef __cplusplus
}
#endif

#endif
