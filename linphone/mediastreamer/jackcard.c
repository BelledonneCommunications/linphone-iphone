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

  JACK support
  Copyright (C) 2004  Tobias Gehrig tobias@gehrig.tk
*/

#include "jackcard.h"

#ifdef __JACK_ENABLED__

#include "msossread.h"
#include "msosswrite.h"

#include <signal.h>

#define READBUFFERSIZE 524288
#define WRITEBUFFERSIZE 524288
#define BSIZE 512

/**
 * jack_shutdown:
 * @arg: 
 * 
 * This is the shutdown callback for this JACK application.
 * It is called by JACK if the server ever shuts down or
 * decides to disconnect the client.
 * 
 */
void
jack_shutdown (void *arg)
{
  JackCard* obj = (JackCard*) arg;

  obj->jack_running = FALSE;
  obj->jack_active = FALSE;
  obj->read.port = NULL;
  if (obj->read.open)
    obj->read.init = TRUE;
  obj->write.port = NULL;
  if (obj->write.open)
    obj->write.init = TRUE;
}

int samplerate(jack_nframes_t rate, void *arg)
{
  JackCard* obj = (JackCard*) arg;
  int error;

  obj->rate = rate;
  if (obj->read.open) {
    obj->read.data.src_ratio = (double)obj->read.rate / (double)obj->rate;
    obj->read.data.input_frames = (long)((double)obj->read.frames/obj->read.data.src_ratio);
    g_free(obj->read.data.data_in);
    obj->read.data.data_in = malloc(obj->read.data.input_frames*sizeof(float));
    if (obj->read.src_state)
      if ((error = src_set_ratio(obj->read.src_state, obj->read.data.src_ratio)) != 0)
	g_warning("Error while resetting the write samplerate: %s", src_strerror(error));
  }
  if (obj->write.open) {
    obj->write.data.src_ratio = (double)obj->rate / (double)obj->write.rate;
    obj->write.data.output_frames = (long)((double)obj->write.frames*obj->write.data.src_ratio);
    g_free(obj->write.data.data_out);
    obj->write.data.data_out = malloc(obj->write.data.output_frames*sizeof(float));
    if (obj->write.src_state) 
      if ((error = src_set_ratio(obj->write.src_state, obj->write.data.src_ratio)) != 0)
	g_warning("Error while resetting the write samplerate: %s", src_strerror(error));
  }
  return 0;
}

/*
 * The process callback for this JACK application.
 * It is called by JACK at the appropriate times.
 * @nframes : 
 * @arg :
 */
int
process (jack_nframes_t nframes, void *arg)
{
  JackCard* obj = (JackCard*) arg;
  sample_t *out;
  sample_t *in;
  
  if (obj->clear && !obj->write.can_process) {
    out = (sample_t *) jack_port_get_buffer (obj->write.port, nframes);
    memset (out, 0, nframes * sizeof(sample_t));
    obj->clear = FALSE;
  }
  
  if (!obj->can_process)
    return 0;

  if(obj->read.can_process) {
    in = (sample_t *) jack_port_get_buffer (obj->read.port, nframes);
    jack_ringbuffer_write (obj->read.buffer, (void *) in, sizeof(sample_t) * nframes);
  }

  if (obj->write.can_process) {
    out = (sample_t *) jack_port_get_buffer (obj->write.port, nframes);
    memset (out, 0, nframes * sizeof(sample_t));
    if (obj->clear && jack_ringbuffer_read_space(obj->write.buffer) == 0) {
      obj->write.can_process = FALSE;
      if (!obj->read.open)
	obj->can_process = FALSE;
      obj->clear = FALSE;
      return 0;
    }
    jack_ringbuffer_read (obj->write.buffer, (void *) out, sizeof(sample_t) * nframes);
  }
  return 0;      
}

int jack_init(JackCard* obj)
{
  char* client_name;
  int error;

  if (!obj->jack_running) {
    obj->client = NULL;
    client_name = g_strdup_printf("linphone-%u", g_random_int());
    if ((obj->client = jack_client_new (client_name)) == NULL) {
      g_warning("cannot create jack client");
      g_free(client_name);
      return -1;
    }
    g_message("Found Jack Daemon");
    g_free(client_name);
    
    /* tell the JACK server to call `process()' whenever
       there is work to be done.
    */
    jack_set_process_callback (obj->client, process, obj);

    /* tell the JACK server to call `jack_shutdown()' if
       it ever shuts down, either entirely, or if it
       just decides to stop calling us.
    */
    jack_on_shutdown (obj->client, jack_shutdown, obj);
    jack_set_sample_rate_callback (obj->client, samplerate, obj);
    obj->rate = jack_get_sample_rate (obj->client);
    if (obj->rate == 0) {
      g_warning ("rate is 0???");
      if (jack_client_close(obj->client) != 0)
	g_warning("could not close client");
      return -1;
    }
    obj->buffer_size = jack_get_buffer_size(obj->client);
    obj->jack_running = TRUE;
  }

  if (!obj->jack_active) {
    if (jack_activate (obj->client)) {
      g_warning("cannot activate jack client");
      return -1;
    } else obj->jack_active = TRUE;
  }

  if (obj->read.init) {
    if (!obj->read.port && (obj->read.port = jack_port_register (obj->client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0))==NULL) {
      g_warning("error while trying to register input port");
      return -1;
    }
    if (!obj->read.phys_ports && (obj->read.phys_ports = jack_get_ports (obj->client, NULL, NULL, JackPortIsPhysical|JackPortIsOutput)) == NULL) {
      g_warning("Cannot find any physical capture ports\n");
      jack_port_unregister(obj->client, obj->read.port);
      obj->read.port = NULL;
      return -1;
    }
    if (!jack_port_connected(obj->read.port))
      if ((error = jack_connect (obj->client, obj->read.phys_ports[0], jack_port_name (obj->read.port))) != 0) {
	g_warning("cannot connect input ports: %s -> %s\n", jack_port_name (obj->read.port), obj->read.phys_ports[0]);
	if (error == EEXIST) g_warning("connection already made");
	else {
	  jack_port_unregister(obj->client, obj->read.port);
	  obj->read.port = NULL;
	  return -1;
	}
      }
    obj->read.init = FALSE;
  }

  if (obj->write.init) {
    if (!obj->write.port && (obj->write.port = jack_port_register (obj->client, "output", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0))==NULL) {
      g_warning("error while trying to register output port");
      return -1;
    }
    if (!obj->write.phys_ports && (obj->write.phys_ports = jack_get_ports (obj->client, NULL, NULL, JackPortIsPhysical|JackPortIsInput)) == NULL) {
      g_warning("Cannot find any physical playback ports\n");
      jack_port_unregister(obj->client, obj->write.port);
      obj->write.port = NULL;
      return -1;
    }
    if (!jack_port_connected(obj->write.port)) {
      if ((error = jack_connect (obj->client, jack_port_name (obj->write.port), obj->write.phys_ports[0])) != 0) {
	g_warning("cannot connect output ports: %s -> %s\n", jack_port_name (obj->write.port), obj->write.phys_ports[0]);
	if (error == EEXIST) g_warning("connection already made");
	else {
	  jack_port_unregister(obj->client, obj->write.port);
	  obj->write.port = NULL;
	  return -1;
	}
      }
      if ((error = jack_connect (obj->client, jack_port_name (obj->write.port), obj->write.phys_ports[1])) != 0) {
	g_warning("cannot connect output ports: %s -> %s\n", jack_port_name (obj->write.port), obj->write.phys_ports[1]);
	if (error == EEXIST) g_warning("connection already made");
	else {
	  jack_port_unregister(obj->client, obj->write.port);
	  obj->write.port = NULL;
	  return -1;
	}
      }
    }
    obj->write.init = FALSE;
  }
  return 0;
}

int jack_card_open_r(JackCard *obj,int bits,int stereo,int rate)
{
  int channels = stereo + 1, bsize, error;
  obj->read.init = TRUE;
  if (jack_init(obj) != 0) return -1;

  obj->read.rate = rate;
  obj->sample_size = bits / 8;
  obj->frame_size = channels * obj->sample_size;
  bsize = BSIZE;
  obj->read.frames = bsize / 2;
  SND_CARD(obj)->bsize = bsize;
  SND_CARD(obj)->flags |= SND_CARD_FLAGS_OPENED;
  obj->read.channels = channels;
  if ((obj->read.src_state = src_new (SRC_SINC_FASTEST, channels, &error)) == NULL)
    g_warning("Error while initializing the samplerate converter: %s", src_strerror(error));
  obj->read.data.src_ratio = (double)rate / (double)obj->rate;
  obj->read.data.input_frames = (long)((double)obj->read.frames/obj->read.data.src_ratio);
  obj->read.data.data_in = malloc(obj->read.data.input_frames*sizeof(float));
  obj->read.data.data_out = malloc(obj->read.frames*sizeof(float));
  obj->read.data.end_of_input = 0;
  if (!obj->read.buffer)
    obj->read.buffer = jack_ringbuffer_create(READBUFFERSIZE);
  obj->read.can_process = TRUE;
  obj->can_process = TRUE;
  obj->read.open = TRUE;
  obj->read.init = FALSE;
  return 0;
}

int jack_card_open_w(JackCard *obj,int bits,int stereo,int rate)
{
  int channels = stereo + 1, bsize, err;
  obj->write.init = TRUE;
  if (jack_init(obj) != 0) return -1;

  obj->write.rate = rate;
  obj->sample_size = bits / 8;
  obj->frame_size = channels * obj->sample_size;
  bsize = BSIZE;
  obj->write.frames = bsize / 2;
  SND_CARD(obj)->bsize = bsize;
  SND_CARD(obj)->flags |= SND_CARD_FLAGS_OPENED;
  obj->write.channels = channels;
  if ((obj->write.src_state = src_new (SRC_SINC_FASTEST, channels, &err)) == NULL)
    g_warning("Error while initializing the samplerate converter: %s", src_strerror(err));
  obj->write.data.src_ratio = (double)obj->rate / (double)rate;
  obj->write.data.data_in = malloc(obj->write.frames*sizeof(float));
  obj->write.data.end_of_input = 0;
  obj->write.data.output_frames = (long)((double)obj->write.frames*obj->write.data.src_ratio);
  obj->write.data.data_out = malloc(obj->write.data.output_frames*sizeof(float));
  if (!obj->write.buffer)
    obj->write.buffer = jack_ringbuffer_create(WRITEBUFFERSIZE);
  obj->write.can_process = TRUE;
  obj->can_process = TRUE;
  obj->write.open = TRUE;
  obj->write.init = FALSE;
  return 0;
}

void jack_card_set_blocking_mode(JackCard *obj, gboolean yesno)
{
}

void jack_card_close_r(JackCard *obj)
{
  obj->read.open = FALSE;
  obj->read.init = FALSE;
  obj->read.can_process = FALSE;
  if (!obj->write.open)
    obj->can_process = FALSE;
  if (obj->read.src_state)
    obj->read.src_state = src_delete (obj->read.src_state);
  g_free(obj->read.data.data_in);
  g_free(obj->read.data.data_out);
}

void jack_card_close_w(JackCard *obj)
{
  obj->write.open = FALSE;
  obj->write.init = FALSE;
  obj->clear = TRUE;
  if (!obj->jack_running) {
    obj->write.can_process = FALSE;
    obj->can_process = FALSE;
  }
  if (obj->write.src_state)
    obj->write.src_state = src_delete (obj->write.src_state);
  g_free(obj->write.data.data_in);
  g_free(obj->write.data.data_out);
}

int jack_card_probe(JackCard *obj,int bits,int stereo,int rate)
{
  if (obj->jack_running) return BSIZE;
  else if (jack_init(obj) == 0) return BSIZE;
  else return -1;
}

void jack_card_destroy(JackCard *obj)
{
  if (obj->jack_running) jack_client_close (obj->client);
  snd_card_uninit(SND_CARD(obj));
  if (obj->read.buffer) {
    jack_ringbuffer_free(obj->read.buffer);
    obj->read.buffer = NULL;
  }
  if (obj->write.buffer) {
    jack_ringbuffer_free(obj->write.buffer);
    obj->write.buffer = NULL;
  }
  if (obj->read.phys_ports) {
    g_free(obj->read.phys_ports);
    obj->read.phys_ports = NULL;
  }
  if (obj->write.phys_ports) {
    g_free(obj->write.phys_ports);
    obj->write.phys_ports = NULL;
  }
}

gboolean jack_card_can_read(JackCard *obj)
{
  g_return_val_if_fail(obj->read.buffer!=NULL,0);
  if (jack_ringbuffer_read_space(obj->read.buffer)>=(long)((double)obj->read.frames/obj->read.data.src_ratio)*sizeof(sample_t)) return TRUE;
  else return FALSE;
}

int jack_card_read(JackCard *obj,char *buf,int size)
{
  size_t bytes, can_read, i;
  int error;
  float norm, value;

  g_return_val_if_fail((obj->read.buffer!=NULL)&&(obj->read.src_state!=NULL),-1);
  if (jack_init(obj) != 0) return -1;
  size /= 2;
  can_read = MIN(size, obj->read.frames);
  //  can_read = MIN(((long)((double)can_read / obj->read.data.src_ratio))*sizeof(sample_t), jack_ringbuffer_read_space(obj->read.buffer));
  can_read = ((long)((double)can_read / obj->read.data.src_ratio))*sizeof(sample_t);
  obj->read.can_process = FALSE;
  bytes = jack_ringbuffer_read (obj->read.buffer, (void *)obj->read.data.data_in, can_read);
  obj->read.can_process = TRUE;
  obj->read.data.input_frames = bytes / sizeof(sample_t);
  can_read = MIN(size, obj->read.frames);
  obj->read.data.output_frames = can_read;
  if ((error = src_process(obj->read.src_state, &(obj->read.data))) != 0)
    g_warning("error while samplerate conversion. error: %s", src_strerror(error));
  norm = obj->read.level*obj->level*(float)0x8000;
  for (i=0; i < obj->read.data.output_frames_gen; i++) {
    value = obj->read.data.data_out[i]*norm;
    if (value >= 32767.0) 
      ((short*)buf)[i] = 32767;
    else if (value <= -32768.0)
      ((short*)buf)[i] = -32768;
    else
      ((short*)buf)[i] = (short)value;
  }
  bytes = obj->read.data.output_frames_gen * 2;
  return bytes;
}

int jack_card_write(JackCard *obj,char *buf,int size)
{
  size_t bytes, can_write, i;
  int error;
  float norm;

  g_return_val_if_fail((obj->write.buffer!=NULL)&&(obj->write.src_state!=NULL),-1);
  if (jack_init(obj) != 0) return -1;
  size /= 2;
  can_write = MIN(size, obj->write.frames);
  norm = obj->write.level*obj->level/(float)0x8000;
  for (i=0; i<can_write; i++) {
    obj->write.data.data_in[i] = (float)((short*)buf)[i]*norm;
  }
  obj->write.data.input_frames = can_write;
  if ((error = src_process(obj->write.src_state, &(obj->write.data))) != 0)
    g_warning("error while samplerate conversion. error: %s", src_strerror(error));
  obj->write.can_process = FALSE;
  bytes = jack_ringbuffer_write (obj->write.buffer, (void *) obj->write.data.data_out, sizeof(sample_t)*obj->write.data.output_frames_gen);
  obj->write.can_process = TRUE;
  return bytes;
}

void jack_card_set_level(JackCard *obj,gint way,gint a)
{
  switch(way){
  case SND_CARD_LEVEL_GENERAL:
    obj->level = (float)a / 100.0;
    break;
  case SND_CARD_LEVEL_INPUT:
    obj->read.level = (float)a / 100.0;
    break;
  case SND_CARD_LEVEL_OUTPUT:
    obj->write.level = (float)a / 100.0;
    break;
  default:
    g_warning("jack_card_set_level: unsupported command.");
  }
}

gint jack_card_get_level(JackCard *obj,gint way)
{
  gint value = 0;

  switch(way){
  case SND_CARD_LEVEL_GENERAL:
    value = (gint)(obj->level*100.0);
    break;
  case SND_CARD_LEVEL_INPUT:
    value = (gint)(obj->read.level*100.0);
    break;
  case SND_CARD_LEVEL_OUTPUT:
    value = (gint)(obj->write.level*100.0);
    break;
  default:
    g_warning("jack_card_get_level: unsupported command.");
  }
  return value;
}

void jack_card_set_source(JackCard *obj,int source)
{
}

MSFilter *jack_card_create_read_filter(JackCard *card)
{
	MSFilter *f=ms_oss_read_new();
	ms_oss_read_set_device(MS_OSS_READ(f),SND_CARD(card)->index);
	return f;
}

MSFilter *jack_card_create_write_filter(JackCard *card)
{
	MSFilter *f=ms_oss_write_new();
	ms_oss_write_set_device(MS_OSS_WRITE(f),SND_CARD(card)->index);
	return f;
}
SndCard * jack_card_new(jack_client_t *client)
{
	JackCard * obj;
	SndCard *base;

	obj= g_new0(JackCard,1);

	if (!client) return NULL;
	obj->client = client;
	obj->jack_running = TRUE;
	obj->jack_active = FALSE;
	obj->can_process = FALSE;
	obj->clear = TRUE;
	obj->write.can_process = FALSE;
	obj->write.open = FALSE;
	obj->write.init = TRUE;
	obj->write.port = NULL;
	obj->write.phys_ports = NULL;
	obj->write.buffer = NULL;
	obj->read.can_process = FALSE;
	obj->read.open = FALSE;
	obj->read.init = TRUE;
	obj->read.port = NULL;
	obj->read.phys_ports = NULL;
	obj->read.buffer = NULL;

	/* tell the JACK server to call `process()' whenever
           there is work to be done.
        */
        jack_set_process_callback (client, process, obj);

        /* tell the JACK server to call `jack_shutdown()' if
           it ever shuts down, either entirely, or if it
           just decides to stop calling us.
        */
        jack_on_shutdown (client, jack_shutdown, obj);

	jack_set_sample_rate_callback (client, samplerate, obj);

	obj->rate = jack_get_sample_rate (client);
	obj->buffer_size = jack_get_buffer_size(obj->client);

	jack_init(obj);
	
	base= SND_CARD(obj);
	snd_card_init(base);
	
#ifdef HAVE_GLIB
	base->card_name=g_strdup_printf("JACK client");
#else
	base->card_name=malloc(100);
	snprintf(base->card_name, 100, "JACK client");
#endif

	base->_probe=(SndCardOpenFunc)jack_card_probe;
	base->_open_r=(SndCardOpenFunc)jack_card_open_r;
	base->_open_w=(SndCardOpenFunc)jack_card_open_w;
	base->_can_read=(SndCardPollFunc)jack_card_can_read;
	base->_set_blocking_mode=(SndCardSetBlockingModeFunc)jack_card_set_blocking_mode;
	base->_read=(SndCardIOFunc)jack_card_read;
	base->_write=(SndCardIOFunc)jack_card_write;
	base->_close_r=(SndCardCloseFunc)jack_card_close_r;
	base->_close_w=(SndCardCloseFunc)jack_card_close_w;
	base->_set_rec_source=(SndCardMixerSetRecSourceFunc)jack_card_set_source;
	base->_set_level=(SndCardMixerSetLevelFunc)jack_card_set_level;
	base->_get_level=(SndCardMixerGetLevelFunc)jack_card_get_level;
	base->_destroy=(SndCardDestroyFunc)jack_card_destroy;
	base->_create_read_filter=(SndCardCreateFilterFunc)jack_card_create_read_filter;
	base->_create_write_filter=(SndCardCreateFilterFunc)jack_card_create_write_filter;
	
	obj->read.buffer=NULL;
	obj->write.buffer=NULL;
	obj->buffer_size = 0;
	obj->level = 1.0;
	obj->write.level = 1.0;
	obj->read.level = 1.0;

	return base;
}


gint jack_card_manager_init(SndCardManager *m, gint index)
{
  jack_client_t *client = NULL;
  char* client_name;

  client_name=g_strdup_printf("linphone-%u", g_random_int());
  if ((client = jack_client_new (client_name))!= NULL)
    {
      g_message("Found Jack Daemon");
      g_free(client_name);
      m->cards[index]=jack_card_new(client);
      m->cards[index]->index=index;
      return 1;
    } else {
      g_free(client_name);
      return 0;
    }
}

#endif
