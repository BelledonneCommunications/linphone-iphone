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

#include "mediastreamer2/msfilter.h"


static void void_sink_process(MSFilter *f){
	mblk_t *im;
	while((im=ms_queue_get(f->inputs[0]))!=NULL){
		freemsg(im);
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_void_sink_desc={
	MS_VOID_SINK_ID,
	"MSVoidSink",
	N_("A filter that trashes its input (useful for terminating some graphs)."),
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	NULL,
	NULL,
	void_sink_process,
	NULL,
	NULL
};

#else

MSFilterDesc ms_void_sink_desc={
	.id=MS_VOID_SINK_ID,
	.name="MSVoidSink",
	.text=N_("A filter that trashes its input (useful for terminating some graphs)."),
	.category=MS_FILTER_OTHER,
	.ninputs=1,
	.noutputs=0,
	.process=void_sink_process,
};

#endif

MS_FILTER_DESC_EXPORT(ms_void_sink_desc)
