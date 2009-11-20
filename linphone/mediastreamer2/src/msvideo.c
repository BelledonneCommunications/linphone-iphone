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

#include "mediastreamer2/msvideo.h"

static void yuv_buf_init(YuvBuf *buf, int w, int h, uint8_t *ptr){
	int ysize,usize;
	ysize=w*h;
	usize=ysize/4;
	buf->w=w;
	buf->h=h;
	buf->planes[0]=ptr;
	buf->planes[1]=buf->planes[0]+ysize;
	buf->planes[2]=buf->planes[1]+usize;
	buf->planes[3]=0;
	buf->strides[0]=w;
	buf->strides[1]=w/2;
	buf->strides[2]=buf->strides[1];
	buf->strides[3]=0;
}

int yuv_buf_init_from_mblk(YuvBuf *buf, mblk_t *m){
	int size=m->b_wptr-m->b_rptr;
	int w,h;
	if (size==(MS_VIDEO_SIZE_QCIF_W*MS_VIDEO_SIZE_QCIF_H*3)/2){
		w=MS_VIDEO_SIZE_QCIF_W;
		h=MS_VIDEO_SIZE_QCIF_H;
	}else if (size==(MS_VIDEO_SIZE_CIF_W*MS_VIDEO_SIZE_CIF_H*3)/2){
		w=MS_VIDEO_SIZE_CIF_W;
		h=MS_VIDEO_SIZE_CIF_H;
	}else if (size==(MS_VIDEO_SIZE_QVGA_W*MS_VIDEO_SIZE_QVGA_H*3)/2){
		w=MS_VIDEO_SIZE_QVGA_W;
		h=MS_VIDEO_SIZE_QVGA_H;
	}else if (size==(MS_VIDEO_SIZE_VGA_W*MS_VIDEO_SIZE_VGA_H*3)/2){
		w=MS_VIDEO_SIZE_VGA_W;
		h=MS_VIDEO_SIZE_VGA_H;
	}else if (size==(MS_VIDEO_SIZE_4CIF_W*MS_VIDEO_SIZE_4CIF_H*3)/2){
		w=MS_VIDEO_SIZE_4CIF_W;
		h=MS_VIDEO_SIZE_4CIF_H;
	}else if (size==(MS_VIDEO_SIZE_SVGA_W*MS_VIDEO_SIZE_SVGA_H*3)/2){
		w=MS_VIDEO_SIZE_SVGA_W;
		h=MS_VIDEO_SIZE_SVGA_H;
	}else if (size==(MS_VIDEO_SIZE_SQCIF_W*MS_VIDEO_SIZE_SQCIF_H*3)/2){
		w=MS_VIDEO_SIZE_SQCIF_W;
		h=MS_VIDEO_SIZE_SQCIF_H;
	}else if (size==(MS_VIDEO_SIZE_QQVGA_W*MS_VIDEO_SIZE_QQVGA_H*3)/2){
		w=MS_VIDEO_SIZE_QQVGA_W;
		h=MS_VIDEO_SIZE_QQVGA_H;
	}else if (size==(MS_VIDEO_SIZE_NS1_W*MS_VIDEO_SIZE_NS1_H*3)/2){
		w=MS_VIDEO_SIZE_NS1_W;
		h=MS_VIDEO_SIZE_NS1_H;
	}else if (size==(MS_VIDEO_SIZE_QSIF_W*MS_VIDEO_SIZE_QSIF_H*3)/2){
		w=MS_VIDEO_SIZE_QSIF_W;
		h=MS_VIDEO_SIZE_QSIF_H;
	}else if (size==(MS_VIDEO_SIZE_SIF_W*MS_VIDEO_SIZE_SIF_H*3)/2){
		w=MS_VIDEO_SIZE_SIF_W;
		h=MS_VIDEO_SIZE_SIF_H;
	}else if (size==(MS_VIDEO_SIZE_4SIF_W*MS_VIDEO_SIZE_4SIF_H*3)/2){
		w=MS_VIDEO_SIZE_4SIF_W;
		h=MS_VIDEO_SIZE_4SIF_H;
	}else if (size==(MS_VIDEO_SIZE_288P_W*MS_VIDEO_SIZE_288P_H*3)/2){
		w=MS_VIDEO_SIZE_288P_W;
		h=MS_VIDEO_SIZE_288P_H;
	}else if (size==(MS_VIDEO_SIZE_448P_W*MS_VIDEO_SIZE_448P_H*3)/2){
		w=MS_VIDEO_SIZE_448P_W;
		h=MS_VIDEO_SIZE_448P_H;
	}else if (size==(MS_VIDEO_SIZE_576P_W*MS_VIDEO_SIZE_576P_H*3)/2){
		w=MS_VIDEO_SIZE_576P_W;
		h=MS_VIDEO_SIZE_576P_H;
	}else if (size==(MS_VIDEO_SIZE_720P_W*MS_VIDEO_SIZE_720P_H*3)/2){
		w=MS_VIDEO_SIZE_720P_W;
		h=MS_VIDEO_SIZE_720P_H;
	}else if (size==(MS_VIDEO_SIZE_1080P_W*MS_VIDEO_SIZE_1080P_H*3)/2){
		w=MS_VIDEO_SIZE_1080P_W;
		h=MS_VIDEO_SIZE_1080P_H;
	}else if (size==(MS_VIDEO_SIZE_SDTV_W*MS_VIDEO_SIZE_SDTV_H*3)/2){
		w=MS_VIDEO_SIZE_SDTV_W;
		h=MS_VIDEO_SIZE_SDTV_H;
	}else if (size==(MS_VIDEO_SIZE_HDTVP_W*MS_VIDEO_SIZE_HDTVP_H*3)/2){
		w=MS_VIDEO_SIZE_HDTVP_W;
		h=MS_VIDEO_SIZE_HDTVP_H;
	}else if (size==(MS_VIDEO_SIZE_XGA_W*MS_VIDEO_SIZE_XGA_H*3)/2){
		w=MS_VIDEO_SIZE_XGA_W;
		h=MS_VIDEO_SIZE_XGA_H;
	}else if (size==(MS_VIDEO_SIZE_WXGA_W*MS_VIDEO_SIZE_WXGA_H*3)/2){
		w=MS_VIDEO_SIZE_WXGA_W;
		h=MS_VIDEO_SIZE_WXGA_H;
	}else if (size==(160*112*3)/2){/*format used by econf*/
		w=160;
		h=112;
	}else if (size==(320*200*3)/2){/*format used by gTalk */
		w=320;
		h=200;
	}else {
		ms_error("Unsupported image size: size=%i (bug somewhere !)",size);
		return -1;
	}
	yuv_buf_init(buf,w,h,m->b_rptr);
	return 0;
}

void yuv_buf_init_from_mblk_with_size(YuvBuf *buf, mblk_t *m, int w, int h){
	yuv_buf_init(buf,w,h,m->b_rptr);
}

mblk_t * yuv_buf_alloc(YuvBuf *buf, int w, int h){
	int size=(w*h*3)/2;
	mblk_t *msg=allocb(size,0);
	yuv_buf_init(buf,w,h,msg->b_wptr);
	msg->b_wptr+=size;
	return msg;
}

static void plane_copy(const uint8_t *src_plane, int src_stride,
	uint8_t *dst_plane, int dst_stride, MSVideoSize roi){
	int i;
	for(i=0;i<roi.height;++i){
		memcpy(dst_plane,src_plane,roi.width);
		src_plane+=src_stride;
		dst_plane+=dst_stride;
	}
}

void yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi){
	plane_copy(src_planes[0],src_strides[0],dst_planes[0],dst_strides[0],roi);
	roi.width=roi.width/2;
	roi.height=roi.height/2;
	plane_copy(src_planes[1],src_strides[1],dst_planes[1],dst_strides[1],roi);
	plane_copy(src_planes[2],src_strides[2],dst_planes[2],dst_strides[2],roi);
}

static void plane_mirror(uint8_t *p, int linesize, int w, int h){
	int i,j;
	uint8_t tmp;
	for(j=0;j<h;++j){
		for(i=0;i<w/2;++i){
			tmp=p[i];
			p[i]=p[w-1-i];
			p[w-1-i]=tmp;
		}
		p+=linesize;
	}
}

/*in place mirroring*/
void yuv_buf_mirror(YuvBuf *buf){
	plane_mirror(buf->planes[0],buf->strides[0],buf->w,buf->h);
	plane_mirror(buf->planes[1],buf->strides[1],buf->w/2,buf->h/2);
	plane_mirror(buf->planes[2],buf->strides[2],buf->w/2,buf->h/2);
}

#ifndef MAKEFOURCC
#define MAKEFOURCC(a,b,c,d) ((d)<<24 | (c)<<16 | (b)<<8 | (a))
#endif

MSPixFmt ms_fourcc_to_pix_fmt(uint32_t fourcc){
	MSPixFmt ret;
	switch (fourcc){
		case MAKEFOURCC('I','4','2','0'):
			ret=MS_YUV420P;
		break;
		case MAKEFOURCC('Y','U','Y','2'):
			ret=MS_YUY2;
		break;
		case MAKEFOURCC('Y','U','Y','V'):
			ret=MS_YUYV;
		break;
		case MAKEFOURCC('U','Y','V','Y'):
			ret=MS_UYVY;
		break;
		case 0: /*BI_RGB on windows*/
			ret=MS_RGB24;
		break;
		default:
			ret=MS_PIX_FMT_UNKNOWN;
	}
	return ret;
}

void rgb24_revert(uint8_t *buf, int w, int h, int linesize){
	uint8_t *p,*pe;
	int i,j;
	uint8_t *end=buf+((h-1)*linesize);
	uint8_t exch;
	p=buf;
	pe=end-1;
	for(i=0;i<h/2;++i){
		for(j=0;j<w*3;++j){
			exch=p[i];
			p[i]=pe[-i];
			pe[-i]=exch;
		}
		p+=linesize;
		pe-=linesize;
	}	
}

void rgb24_copy_revert(uint8_t *dstbuf, int dstlsz,
				const uint8_t *srcbuf, int srclsz, MSVideoSize roi){
	int i,j;
	const uint8_t *psrc;
	uint8_t *pdst;
	psrc=srcbuf;
	pdst=dstbuf+(dstlsz*(roi.height-1));
	for(i=0;i<roi.height;++i){
		for(j=0;j<roi.width*3;++j){
			pdst[(roi.width*3)-1-j]=psrc[j];
		}
		pdst-=dstlsz;
		psrc+=srclsz;
	}
}

static MSVideoSize _ordered_vsizes[]={
	{MS_VIDEO_SIZE_QCIF_W,MS_VIDEO_SIZE_QCIF_H},
	{MS_VIDEO_SIZE_QVGA_W,MS_VIDEO_SIZE_QVGA_H},
	{MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H},
	{MS_VIDEO_SIZE_VGA_W,MS_VIDEO_SIZE_VGA_H},
	{MS_VIDEO_SIZE_4CIF_W,MS_VIDEO_SIZE_4CIF_H},
	{MS_VIDEO_SIZE_720P_W,MS_VIDEO_SIZE_720P_H},
	{0,0}
};

MSVideoSize ms_video_size_get_just_lower_than(MSVideoSize vs){
	MSVideoSize *p;
	MSVideoSize ret;
	ret.width=0;
	ret.height=0;
	for(p=_ordered_vsizes;p->width!=0;++p){
		if (ms_video_size_greater_than(vs,*p) && !ms_video_size_equal(vs,*p)){
			ret=*p;
		}else return ret;
	}
	return ret;
};
