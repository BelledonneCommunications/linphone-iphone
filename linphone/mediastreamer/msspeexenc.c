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



#include "msspeexenc.h"
#include "ms.h"
extern MSCodecInfo speex_info;

static MSSpeexEncClass * ms_speex_enc_class=NULL;

MSFilter * ms_speex_enc_new()
{
	MSSpeexEnc *obj=g_new(MSSpeexEnc,1);
	
	if (ms_speex_enc_class==NULL){
		ms_speex_enc_class=g_new(MSSpeexEncClass,1);
		ms_speex_enc_class_init(ms_speex_enc_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_speex_enc_class);
	ms_speex_enc_init(MS_SPEEX_ENC(obj));
	return MS_FILTER(obj);
}

void ms_speex_enc_init(MSSpeexEnc *obj)
{
	ms_filter_init(MS_FILTER(obj));
	MS_FILTER(obj)->infifos=obj->inf;
	MS_FILTER(obj)->outqueues=obj->outq;
	obj->inf[0]=NULL;
	obj->outq[0]=NULL;
	obj->frequency=8000;
	obj->bitrate=30000;
  obj->initialized=0;
}

void ms_speex_enc_init_core(MSSpeexEnc *obj,const SpeexMode *mode, gint bitrate)
{
	int proc_type, proc_speed;
	gchar *proc_vendor;
	int tmp;
	int frame_size;
	
	obj->speex_state=speex_encoder_init((SpeexMode*)mode);
	speex_bits_init(&obj->bits);
	
	if (bitrate>0) {
		bitrate++;
		speex_encoder_ctl(obj->speex_state, SPEEX_SET_BITRATE, &bitrate);
		g_message("Setting speex output bitrate less or equal than %i",bitrate-1);
	}

	proc_speed=ms_proc_get_speed();
	proc_vendor=ms_proc_get_param("vendor_id");
	if (proc_speed<0 || proc_vendor==NULL){
		g_warning("Can't guess processor features: setting speex encoder to its lowest complexity.");
		tmp=1;
		speex_encoder_ctl(obj->speex_state,SPEEX_SET_COMPLEXITY,&tmp);
	}else if ((proc_speed!=-1) && (proc_speed<200)){
		g_warning("A cpu speed less than 200 Mhz is not enough: let's reduce the complexity of the speex codec.");
		tmp=1;
		speex_encoder_ctl(obj->speex_state,SPEEX_SET_COMPLEXITY,&tmp);
	}else if (proc_vendor!=NULL) {
		if (strncmp(proc_vendor,"GenuineIntel",strlen("GenuineIntel"))==0){
			proc_type=ms_proc_get_type();
			if (proc_type==5){
				g_warning("A pentium I is not enough fast for speex codec in normal mode: let's reduce its complexity.");
				tmp=1;
				speex_encoder_ctl(obj->speex_state,SPEEX_SET_COMPLEXITY,&tmp);
			}
		}
		g_free(proc_vendor);
	}
	/* guess the used input frame size */
	speex_mode_query((SpeexMode*)mode, SPEEX_MODE_FRAME_SIZE, &frame_size);
	MS_FILTER(obj)->r_mingran=frame_size*2;
	ms_trace("ms_speex_init: using frame size of %i.",MS_FILTER(obj)->r_mingran);
	
	obj->initialized=1;
}

/* must be called before the encoder is running*/
int ms_speex_enc_set_property(MSSpeexEnc *obj,int property,int *value)
{
	if (obj->initialized){
		/* we are called when speex is running !! forbid that! */
		ms_warning("ms_speex_enc_set_property: cannot call this function when running!");
		return -1;
	}
	switch(property){
		case MS_FILTER_PROPERTY_FREQ:
			obj->frequency=value[0];
		break;
		case MS_FILTER_PROPERTY_BITRATE: /* to specify max bitrate */
			obj->bitrate=value[0];
		break;
	}
	return 0;
}

void ms_speex_enc_setup(MSSpeexEnc *obj)
{
	const SpeexMode *mode;
	g_message("Speex encoder setup: freq=%i",obj->frequency);
	if ( obj->frequency< 16000) mode=&speex_nb_mode;
	else mode=&speex_wb_mode;
	ms_speex_enc_init_core(obj,mode,obj->bitrate);
	
}

void ms_speex_enc_unsetup(MSSpeexEnc *obj)
{
	ms_speex_enc_uninit_core(obj);
}

void ms_speex_enc_class_init(MSSpeexEncClass *klass)
{
	gint frame_size=0;
	
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	/* we take the larger (wb) frame size */
	speex_mode_query(&speex_wb_mode, SPEEX_MODE_FRAME_SIZE, &frame_size);
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_speex_enc_process;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_speex_enc_destroy;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_speex_enc_setup;
	MS_FILTER_CLASS(klass)->unsetup=(MSFilterSetupFunc)ms_speex_enc_unsetup;
	MS_FILTER_CLASS(klass)->set_property=(MSFilterPropertyFunc)ms_speex_enc_set_property;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"SpeexEncoder");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&speex_info;
	MS_FILTER_CLASS(klass)->max_finputs=1;
	MS_FILTER_CLASS(klass)->max_qoutputs=1;
	MS_FILTER_CLASS(klass)->r_maxgran=frame_size*2;
	ms_trace("ms_speex_enc_class_init: r_maxgran is %i.",MS_FILTER_CLASS(klass)->r_maxgran);
}

void ms_speex_enc_uninit_core(MSSpeexEnc *obj)
{
	if (obj->initialized){
		speex_encoder_destroy(obj->speex_state);
		speex_bits_destroy(&obj->bits);
		obj->initialized=0;
	}
}

void ms_speex_enc_destroy(MSSpeexEnc *obj)
{
	ms_speex_enc_uninit_core(obj);
	g_free(obj);
}

void ms_speex_enc_process(MSSpeexEnc *obj)
{
	MSFifo *inf=obj->inf[0];
	MSQueue *outq=obj->outq[0];
	gint16 *input;
	gint gran=MS_FILTER(obj)->r_mingran;
	MSMessage *m;
	
	g_return_if_fail(inf!=NULL);
	g_return_if_fail(outq!=NULL);
	
	ms_fifo_get_read_ptr(inf,gran,(void**)&input);
	g_return_if_fail(input!=NULL);
	/* encode */
	speex_bits_reset(&obj->bits);
	speex_encode_int(obj->speex_state,(short*)input,&obj->bits);
	m=ms_message_new(speex_bits_nbytes(&obj->bits));
	m->size=speex_bits_write(&obj->bits,m->data,m->size);
	ms_queue_put(outq,m);
}
