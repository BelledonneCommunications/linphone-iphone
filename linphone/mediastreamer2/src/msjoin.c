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

static void join_process(MSFilter *f){
	mblk_t *im;
	if (f->inputs[0]!=NULL)
	{
		while((im=ms_queue_get(f->inputs[0]))!=NULL){
			ms_queue_put(f->outputs[0],im);
		}
	}
	if (f->inputs[1]!=NULL)
	{
		while((im=ms_queue_get(f->inputs[1]))!=NULL){
			int payload;
			payload=mblk_set_payload_type(im, 123);
			ms_queue_put(f->outputs[0],im);
		}
	}
}

#ifdef _MSC_VER

MSFilterDesc ms_join_desc={
	MS_JOIN_ID,
	"MSJoin",
	N_("A filter that send several inputs to one output."),
	MS_FILTER_OTHER,
	NULL,
	2,
	1,
    NULL,
	NULL,
	join_process,
	NULL,
	NULL,
    NULL
};

#else

MSFilterDesc ms_join_desc={
	.id=MS_JOIN_ID,
	.name="MSJoin",
	.text=N_("A filter that send several inputs to one output."),
	.category=MS_FILTER_OTHER,
	.ninputs=2,
	.noutputs=1,
	.process=join_process
};

#endif

MS_FILTER_DESC_EXPORT(ms_join_desc)
