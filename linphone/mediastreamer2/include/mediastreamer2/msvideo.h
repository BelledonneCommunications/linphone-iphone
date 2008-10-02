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

#ifndef msvideo_h
#define msvideo_h

#include "msfilter.h"

/* some global constants for video MSFilter(s) */
#define MS_VIDEO_SIZE_CIF_W 352
#define MS_VIDEO_SIZE_CIF_H 288
#define MS_VIDEO_SIZE_QCIF_W 176
#define MS_VIDEO_SIZE_QCIF_H 144
#define MS_VIDEO_SIZE_4CIF_W 704
#define MS_VIDEO_SIZE_4CIF_H 576

#define MS_VIDEO_SIZE_QQVGA_W 160
#define MS_VIDEO_SIZE_QQVGA_H 120

#define MS_VIDEO_SIZE_QVGA_W 320
#define MS_VIDEO_SIZE_QVGA_H 240

#define MS_VIDEO_SIZE_VGA_W 640
#define MS_VIDEO_SIZE_VGA_H 480

#define MS_VIDEO_SIZE_720P_W 1280
#define MS_VIDEO_SIZE_720P_H 720

#define MS_VIDEO_SIZE_NS1_W 324
#define MS_VIDEO_SIZE_NS1_H 248

#define MS_VIDEO_SIZE_1024_W 1024
#define MS_VIDEO_SIZE_1024_H 768

#define MS_VIDEO_SIZE_MAX_W MS_VIDEO_SIZE_1024_W
#define MS_VIDEO_SIZE_MAX_H MS_VIDEO_SIZE_1024_H

typedef struct MSVideoSize{
	int width,height;
} MSVideoSize;

typedef struct MSRect{
	int x,y,w,h;
} MSRect;

#define MS_VIDEO_SIZE_CIF (MSVideoSize){MS_VIDEO_SIZE_CIF_W,MS_VIDEO_SIZE_CIF_H}
#define MS_VIDEO_SIZE_QCIF (MSVideoSize){MS_VIDEO_SIZE_QCIF_W,MS_VIDEO_SIZE_QCIF_H}
#define MS_VIDEO_SIZE_4CIF (MSVideoSize){MS_VIDEO_SIZE_4CIF_W,MS_VIDEO_SIZE_4CIF_H}

#define MS_VIDEO_SIZE_QQVGA (MSVideoSize){MS_VIDEO_SIZE_QQVGA_W,MS_VIDEO_SIZE_QQVGA_H}
#define MS_VIDEO_SIZE_QVGA (MSVideoSize){MS_VIDEO_SIZE_QVGA_W,MS_VIDEO_SIZE_QVGA_H}
#define MS_VIDEO_SIZE_VGA (MSVideoSize){MS_VIDEO_SIZE_VGA_W,MS_VIDEO_SIZE_VGA_H}

#define MS_VIDEO_SIZE_720P (MSVideoSize){MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H}

#define MS_VIDEO_SIZE_NS1 (MSVideoSize){MS_VIDEO_SIZE_NS1_W,MS_VIDEO_SIZE_NS1_H}

#define MS_VIDEO_SIZE_1024 (MSVideoSize){MS_VIDEO_SIZE_1024_W, MS_VIDEO_SIZE_1024_H}

typedef enum{
	MS_YUV420P,
	MS_YUYV,
	MS_RGB24,
	MS_MJPEG,
	MS_UYVY,
	MS_YUY2,   /* -> same as MS_YUYV */
	MS_PIX_FMT_UNKNOWN
}MSPixFmt;

typedef struct _MSPicture{
	int w,h;
	uint8_t *planes[4]; /*we usually use 3 planes, 4th is for compatibility */
	int strides[4];	/*with ffmpeg's swscale.h */
}MSPicture;

typedef struct _MSPicture YuvBuf; /*for backward compatibility*/

#ifdef __cplusplus
extern "C"{
#endif

int ms_pix_fmt_to_ffmpeg(MSPixFmt fmt);
MSPixFmt ffmpeg_pix_fmt_to_ms(int fmt);
MSPixFmt ms_fourcc_to_pix_fmt(uint32_t fourcc);
void ms_ffmpeg_check_init(void);
int yuv_buf_init_from_mblk(MSPicture *buf, mblk_t *m);
void yuv_buf_init_from_mblk_with_size(MSPicture *buf, mblk_t *m, int w, int h);
mblk_t * yuv_buf_alloc(MSPicture *buf, int w, int h);
void yuv_buf_copy(uint8_t *src_planes[], const int src_strides[], 
		uint8_t *dst_planes[], const int dst_strides[3], MSVideoSize roi);
void yuv_buf_mirror(YuvBuf *buf);
void rgb24_revert(uint8_t *buf, int w, int h, int linesize);

#ifdef __cplusplus
}
#endif

#define MS_FILTER_SET_VIDEO_SIZE	MS_FILTER_BASE_METHOD(100,MSVideoSize)
#define MS_FILTER_GET_VIDEO_SIZE	MS_FILTER_BASE_METHOD(101,MSVideoSize)

#define MS_FILTER_SET_PIX_FMT		MS_FILTER_BASE_METHOD(102,MSPixFmt)
#define MS_FILTER_GET_PIX_FMT		MS_FILTER_BASE_METHOD(103,MSPixFmt)

#define MS_FILTER_SET_FPS		MS_FILTER_BASE_METHOD(104,float)
#define MS_FILTER_GET_FPS		MS_FILTER_BASE_METHOD(105,float)

/* request a video-fast-update (=I frame for H263,MP4V-ES) to a video encoder*/
#define MS_FILTER_REQ_VFU		MS_FILTER_BASE_METHOD_NO_ARG(106)

#endif
