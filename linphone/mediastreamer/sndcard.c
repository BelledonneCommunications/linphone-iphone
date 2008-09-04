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


#include "sndcard.h"
#include "msfilter.h"

void snd_card_init(SndCard *obj)
{
	memset(obj,0,sizeof(SndCard));
}

void snd_card_uninit(SndCard *obj)
{
	if (obj->card_name!=NULL) g_free(obj->card_name);
}

const gchar *snd_card_get_identifier(SndCard *obj)
{
	return obj->card_name;
}

int snd_card_open_r(SndCard *obj, int bits, int stereo, int rate)
{
	g_return_val_if_fail(obj->_open_r!=NULL,-1);
	g_message("Opening sound card [%s] in capture mode with stereo=%i,rate=%i,bits=%i",obj->card_name,stereo,rate,bits);
	return obj->_open_r(obj,bits,stereo,rate);
}
int snd_card_open_w(SndCard *obj, int bits, int stereo, int rate)
{
	g_return_val_if_fail(obj->_open_w!=NULL,-1);
	g_message("Opening sound card [%s] in playback mode with stereo=%i,rate=%i,bits=%i",obj->card_name,stereo,rate,bits);
	return obj->_open_w(obj,bits,stereo,rate); 
}

gboolean snd_card_can_read(SndCard *obj){
	g_return_val_if_fail(obj->_can_read!=NULL,-1);
	return obj->_can_read(obj);
}

void snd_card_set_blocking_mode(SndCard *obj,gboolean yesno){
	g_return_if_fail(obj->_set_blocking_mode!=NULL);
	obj->_set_blocking_mode(obj,yesno);
}

int snd_card_read(SndCard *obj,char *buffer,int size)
{
	g_return_val_if_fail(obj->_read!=NULL,-1);
	return obj->_read(obj,buffer,size);
}
int snd_card_write(SndCard *obj,char *buffer,int size)
{
	g_return_val_if_fail(obj->_write!=NULL,-1);
	return obj->_write(obj,buffer,size);
}

int snd_card_get_bsize(SndCard *obj)
{
	if (obj->flags & SND_CARD_FLAGS_OPENED){
		return obj->bsize;
	}
	return -1;
}

void snd_card_close_r(SndCard *obj)
{
	g_return_if_fail(obj->_close_r!=NULL);
	g_message("Closing reading channel of soundcard.");
	obj->_close_r(obj);
}

void snd_card_close_w(SndCard *obj)
{
	g_return_if_fail(obj->_close_w!=NULL);
	g_message("Closing writing channel of soundcard.");
	obj->_close_w(obj);
}

gint snd_card_probe(SndCard *obj,int bits, int stereo, int rate)
{
	g_return_val_if_fail(obj->_probe!=NULL,-1);
	return obj->_probe(obj,bits,stereo,rate);
}

void snd_card_set_rec_source(SndCard *obj, int source)
{
	g_return_if_fail(obj->_set_rec_source!=NULL);
	obj->_set_rec_source(obj,source);
}

void snd_card_set_level(SndCard *obj, int way, int level)
{
	g_return_if_fail(obj->_set_level!=NULL);
	obj->_set_level(obj,way,level);
}

gint snd_card_get_level(SndCard *obj,int way)
{
	g_return_val_if_fail(obj->_get_level!=NULL,-1);
	return obj->_get_level(obj,way);
}


MSFilter * snd_card_create_read_filter(SndCard *obj)
{
	g_return_val_if_fail(obj->_create_read_filter!=NULL,NULL);
	return obj->_create_read_filter(obj);
}
MSFilter * snd_card_create_write_filter(SndCard *obj)
{
	g_return_val_if_fail(obj->_create_write_filter!=NULL,NULL);
	return obj->_create_write_filter(obj);
}


#ifdef HAVE_SYS_AUDIO_H
gint sys_audio_manager_init(SndCardManager *manager, gint index)
{
	/* this is a quick shortcut, as multiple soundcards on HPUX does not happen
	very often... */
	manager->cards[index]=hpux_snd_card_new("/dev/audio","/dev/audio");
	return 1;
}

#endif

#include "osscard.h"
#include "alsacard.h"
#include "jackcard.h"

#ifdef HAVE_SYS_SOUNDCARD_H
/* in osscard.c */
gint oss_card_manager_init(SndCardManager *manager, gint tabindex);
#endif

void snd_card_manager_init(SndCardManager *manager)
{
	gint index=0;
	gint tmp=0;
	memset(manager,0,sizeof(SndCardManager));
	#ifdef HAVE_SYS_SOUNDCARD_H
	tmp=oss_card_manager_init(manager,index);
	index+=tmp;
	if (index>=MAX_SND_CARDS) return;
	#endif
	#ifdef __ALSA_ENABLED__
	tmp=alsa_card_manager_init(manager,index);
	index+=tmp;
	if (index>=MAX_SND_CARDS) return;
	#endif
	#ifdef __JACK_ENABLED__
	tmp=jack_card_manager_init(manager,index);
	index+=tmp;
	if (index>=MAX_SND_CARDS) return;
	#endif
	#ifdef HAVE_SYS_AUDIO_H
	tmp=sys_audio_manager_init(manager,index);
	index+=tmp;
	#endif
}





SndCard * snd_card_manager_get_card(SndCardManager *manager,int index)
{
	g_return_val_if_fail(index>=0,NULL);
	g_return_val_if_fail(index<MAX_SND_CARDS,NULL);
	if (index>MAX_SND_CARDS) return NULL;
	return manager->cards[index];	
}

SndCard * snd_card_manager_get_card_with_string(SndCardManager *manager,const char *cardname,int *index)
{
	int i;
	for (i=0;i<MAX_SND_CARDS;i++){
		gchar *card_name;
		if (manager->cards[i]==NULL) continue;
		card_name=manager->cards[i]->card_name;
		if (card_name==NULL) continue;
		if (strcmp(card_name,cardname)==0){
			*index=i;
			return manager->cards[i];
		}
	}
	g_warning("No card %s found.",cardname);
	return NULL;
}

SndCardManager _snd_card_manager;
SndCardManager *snd_card_manager=&_snd_card_manager;
