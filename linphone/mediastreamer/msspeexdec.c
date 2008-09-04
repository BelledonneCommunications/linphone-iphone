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


#include "msspeexdec.h"

#ifdef HAVE_GLIB
#include <gmodule.h>
#endif

extern MSFilter * ms_speex_enc_new();

MSCodecInfo speex_info=
{
	{
		"Speex codec",
		0,
		MS_FILTER_AUDIO_CODEC,
		ms_speex_dec_new,
		"A high quality variable bit-rate codec from Jean Marc Valin and David Rowe."
	},
	ms_speex_enc_new,
	ms_speex_dec_new,
	0,		/*frame size */
	0,
	8000, /*minimal bitrate */
	-1,	/* sampling frequency */
	110,		/* payload type */
	"speex",
	1,
	1
};



void ms_speex_codec_init()
{

	ms_filter_register(MS_FILTER_INFO(&speex_info));
	//ms_filter_register(MS_FILTER_INFO(&speex_lbr_info));
}

#ifdef HAVE_GLIB
gchar * g_module_check_init(GModule *module)
{
	ms_speex_codec_init();
	
	return NULL;
}
#else
gchar * g_module_check_init()
{
	ms_speex_codec_init();
	
	return NULL;
}
#endif

static MSSpeexDecClass * ms_speex_dec_class=NULL;
//static MSSpeexDecClass * ms_speexnb_dec_class=NULL;

MSFilter * ms_speex_dec_new()
{
	MSSpeexDec *obj=g_new(MSSpeexDec,1);
	
	if (ms_speex_dec_class==NULL){
		ms_speex_dec_class=g_new(MSSpeexDecClass,1);
		ms_speex_dec_class_init(ms_speex_dec_class);
	}
	MS_FILTER(obj)->klass=MS_FILTER_CLASS(ms_speex_dec_class);

	ms_speex_dec_init(obj);
	return MS_FILTER(obj);
}

void ms_speex_dec_init(MSSpeexDec *obj)
{
	ms_filter_init(MS_FILTER(obj));
	obj->initialized=0;
	MS_FILTER(obj)->outfifos=obj->outf;
	MS_FILTER(obj)->inqueues=obj->inq;
	obj->outf[0]=NULL;
	obj->inq[0]=NULL;
	obj->frequency=8000; /*default value */
	
}

void ms_speex_dec_init_core(MSSpeexDec *obj,const SpeexMode *mode)
{
	int pf=1;
	
	obj->speex_state=speex_decoder_init((SpeexMode*)mode);
	speex_bits_init(&obj->bits);
	/* enable the perceptual post filter */
	speex_decoder_ctl(obj->speex_state,SPEEX_SET_PF, &pf);
	
	speex_mode_query((SpeexMode*)mode, SPEEX_MODE_FRAME_SIZE, &obj->frame_size);
	
	obj->initialized=1;
}

int ms_speex_dec_set_property(MSSpeexDec *obj, MSFilterProperty prop, int *value)
{
	if (obj->initialized){
		/* we are called when speex is running !! forbid that! */
		ms_warning("ms_speex_dec_set_property: cannot call this function when running!");
		return -1;
	}
	switch(prop){
		case MS_FILTER_PROPERTY_FREQ:
			obj->frequency=value[0];
		break;
		default:
		break;
	}
	return 0;
}

void ms_speex_dec_setup(MSSpeexDec *obj)
{
	const SpeexMode *mode;
	g_message("Speex decoder setup: freq=%i",obj->frequency);
	if ( obj->frequency< 16000) mode=&speex_nb_mode;
	else mode=&speex_wb_mode;
	ms_speex_dec_init_core(obj,mode);
}

void ms_speex_dec_unsetup(MSSpeexDec *obj)
{
	ms_speex_dec_uninit_core(obj);
}

void ms_speex_dec_class_init(MSSpeexDecClass *klass)
{
	gint frame_size=0;
	
	ms_filter_class_init(MS_FILTER_CLASS(klass));
	/* use the largest frame size to configure fifos */
	speex_mode_query(&speex_wb_mode, SPEEX_MODE_FRAME_SIZE, &frame_size);
	MS_FILTER_CLASS(klass)->process=(MSFilterProcessFunc)ms_speex_dec_process;
	MS_FILTER_CLASS(klass)->setup=(MSFilterSetupFunc)ms_speex_dec_setup;
	MS_FILTER_CLASS(klass)->unsetup=(MSFilterSetupFunc)ms_speex_dec_unsetup;
	MS_FILTER_CLASS(klass)->destroy=(MSFilterDestroyFunc)ms_speex_dec_destroy;
	MS_FILTER_CLASS(klass)->set_property=(MSFilterPropertyFunc)ms_speex_dec_set_property;
	ms_filter_class_set_name(MS_FILTER_CLASS(klass),"SpeexDecoder");
	MS_FILTER_CLASS(klass)->info=(MSFilterInfo*)&speex_info;
	MS_FILTER_CLASS(klass)->max_foutputs=1;
	MS_FILTER_CLASS(klass)->max_qinputs=1;
	MS_FILTER_CLASS(klass)->w_maxgran=frame_size*2;
	ms_trace("ms_speex_dec_class_init: w_maxgran is %i.",MS_FILTER_CLASS(klass)->w_maxgran);
}

void ms_speex_dec_uninit_core(MSSpeexDec *obj)
{
	speex_decoder_destroy(obj->speex_state);
	speex_bits_destroy(&obj->bits);
	obj->initialized=0;
}

void ms_speex_dec_uninit(MSSpeexDec *obj)
{
	
}

void ms_speex_dec_destroy(MSSpeexDec *obj)
{
	ms_speex_dec_uninit(obj);
	g_free(obj);
}

void ms_speex_dec_process(MSSpeexDec *obj)
{
	MSFifo *outf=obj->outf[0];
	MSQueue *inq=obj->inq[0];
	gint16 *output;
	gint gran=obj->frame_size*2;
	MSMessage *m;
	
	g_return_if_fail(inq!=NULL);
	g_return_if_fail(outf!=NULL);
	
	m=ms_queue_get(inq);
	g_return_if_fail(m!=NULL);
	speex_bits_reset(&obj->bits);
	ms_fifo_get_write_ptr(outf,gran,(void**)&output);
	g_return_if_fail(output!=NULL);
	if (m->data!=NULL){
		
		speex_bits_read_from(&obj->bits,m->data,m->size);
		/* decode */
		speex_decode_int(obj->speex_state,&obj->bits,(short*)output);
	}else{
		/* we have a missing packet */
		speex_decode_int(obj->speex_state,NULL,(short*)output);
	}
	ms_message_destroy(m);
	
}
