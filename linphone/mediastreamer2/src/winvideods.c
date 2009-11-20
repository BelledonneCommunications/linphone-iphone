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

#define UNICODE
#define AYMERIC_TEST
#define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA

#include "mediastreamer2/msvideo.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/msv4l.h"
#include "mediastreamer2/mswebcam.h"

#include "nowebcam.h"

#ifdef HAVE_LIBAVCODEC_AVCODEC_H
#include <libavcodec/avcodec.h>
#else
#include <ffmpeg/avcodec.h>
#endif

#include <dshow.h>
#include <dmodshow.h>
#include <dmoreg.h>

#include <streams.h>
#include <initguid.h>

#include "dxfilter.h"
EXTERN_C const CLSID CLSID_NullRenderer;

typedef struct V4wState{

	char dev[512];
	int devidx;

	IGraphBuilder *m_pGraph;
	ICaptureGraphBuilder2 *m_pBuilder;
	IMediaControl *m_pControl;
	CDXFilter *m_pDXFilter;
	IBaseFilter *m_pIDXFilter;	
	IBaseFilter *m_pNullRenderer;
	IBaseFilter *m_pDeviceFilter;
	DWORD rotregvalue;

	MSVideoSize vsize;
	int pix_fmt;
	mblk_t *mire[10];
	char nowebcamimage[256];
	queue_t rq;
	ms_mutex_t mutex;
	int frame_ind;
	int frame_max;
	float fps;
	uint64_t start_time;
	int frame_count;
}V4wState;

static V4wState *s_callback=NULL;

static void dummy(void*p){
}

HRESULT ( Callback)(IMediaSample* pSample, REFERENCE_TIME* sTime, REFERENCE_TIME* eTime, BOOL changed)
{
	BYTE *byte_buf=NULL;
	mblk_t *buf;

	V4wState *s = s_callback;
	if (s==NULL)
		return S_OK;

	HRESULT hr = pSample->GetPointer(&byte_buf);
	if (FAILED(hr))
	{
		return S_OK;
	}

	int size = pSample->GetActualDataLength();
	if (size>+1000)
	{
		buf=allocb(size,0);
		memcpy(buf->b_wptr, byte_buf, size);
		if (s->pix_fmt==MS_RGB24)
		{
			/* Conversion from top down bottom up (BGR to RGB and flip) */
			unsigned long Index,nPixels;
			unsigned char *blue;
			unsigned char tmp;
			short iPixelSize;

			blue=buf->b_wptr;

			nPixels=s->vsize.width*s->vsize.height;
			iPixelSize=24/8;

			for(Index=0;Index!=nPixels;Index++)  // For each pixel
			{
				tmp=*blue;
				*blue=*(blue+2);
				*(blue+2)=tmp;
				blue+=iPixelSize;
			}

			unsigned char *pLine1, *pLine2;
			int iLineLen,iIndex;

			iLineLen=s->vsize.width*iPixelSize;
			pLine1=buf->b_wptr;
			pLine2=&(buf->b_wptr)[iLineLen * (s->vsize.height - 1)];

			for( ;pLine1<pLine2;pLine2-=(iLineLen*2))
			{
				for(iIndex=0;iIndex!=iLineLen;pLine1++,pLine2++,iIndex++)
				{
					tmp=*pLine1;
					*pLine1=*pLine2;
					*pLine2=tmp;       
				}
			}
		}
		buf->b_wptr+=size;  

		ms_mutex_lock(&s->mutex);
		putq(&s->rq, buf);
		ms_mutex_unlock(&s->mutex);

	}
	return S_OK;
}

HRESULT GetPinCategory(IPin *pPin, GUID *pPinCategory)
{
	HRESULT hr;
	IKsPropertySet *pKs;
	hr = pPin->QueryInterface(IID_IKsPropertySet, (void **)&pKs);
	if (FAILED(hr))
	{
		// The pin does not support IKsPropertySet.
		return hr;
	}
	// Try to retrieve the pin category.
	DWORD cbReturned;
	hr = pKs->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL, 0, 
		pPinCategory, sizeof(GUID), &cbReturned);

	// If this succeeded, pPinCategory now contains the category GUID.

	pKs->Release();
	return hr;
}

int try_format(IBaseFilter *m_pDeviceFilter, int format, GUID *pPinCategory)
{
	HRESULT hr=S_OK;
	IEnumPins *pEnum=0;
	ULONG ulFound;
	IPin *pPin;

	GUID guid_format;
	DWORD biCompression;
	DWORD biBitCount;

	// Verify input
	if (!m_pDeviceFilter)
		return -1;

	if (format == MS_YUV420P)
		guid_format = (GUID)FOURCCMap(MAKEFOURCC('I','4','2','0'));
	else if (format == MS_YUYV)
		guid_format = MEDIASUBTYPE_YUYV;
	else if (format == MS_UYVY)
		guid_format = MEDIASUBTYPE_UYVY;
	else if (format == MS_RGB24)
		guid_format = MEDIASUBTYPE_RGB24;
	else if (format == MS_YUY2)
		guid_format = MEDIASUBTYPE_YUY2;

	if (format == MS_YUV420P)
		biCompression = MAKEFOURCC('I','4','2','0');
	else if (format == MS_YUYV)
		biCompression = MAKEFOURCC('Y','U','Y','V');
	else if (format == MS_UYVY)
		biCompression = MAKEFOURCC('U','Y','V','Y');
	else if (format == MS_RGB24)
		biCompression = BI_RGB;
	else if (format == MS_YUY2)
		biCompression = MAKEFOURCC('Y','U','Y','2');

	if (format == MS_YUV420P)
		biBitCount = 12;
	else if (format == MS_YUYV)
		biBitCount = 16;
	else if (format == MS_UYVY)
		biBitCount = 16;
	else if (format == MS_RGB24)
		biBitCount = 24;
	else if (format == MS_YUY2)
		biBitCount = 16;

	// Get pin enumerator
	hr = m_pDeviceFilter->EnumPins(&pEnum);
	if(FAILED(hr)) 
		return -1;

	pEnum->Reset();

	// Count every pin on the filter
	while(S_OK == pEnum->Next(1, &pPin, &ulFound))
	{
		PIN_DIRECTION pindir = (PIN_DIRECTION) 3;

		hr = pPin->QueryDirection(&pindir);

		if(pindir != PINDIR_INPUT)
		{
			IEnumMediaTypes *ppEnum;
			ULONG ulFound2;

			GetPinCategory(pPin, pPinCategory);
			if (*pPinCategory!=PIN_CATEGORY_CAPTURE
				&& *pPinCategory!=PIN_CATEGORY_PREVIEW)
				continue;

			hr = pPin->EnumMediaTypes(&ppEnum);
			if(FAILED(hr)) 
				continue;

			AM_MEDIA_TYPE *ppMediaTypes;
			while(S_OK == ppEnum->Next(1, &ppMediaTypes, &ulFound2))
			{
				if (ppMediaTypes->formattype != FORMAT_VideoInfo)
					continue;
				if (ppMediaTypes->majortype != MEDIATYPE_Video)
					continue;
				if (ppMediaTypes->subtype != guid_format)
					continue;
				VIDEOINFO *pvi = (VIDEOINFO *)ppMediaTypes->pbFormat;
				if (pvi->bmiHeader.biCompression!=biCompression)
					continue;
				if (pvi->bmiHeader.biBitCount!=biBitCount)
					continue;

				pPin->Release();
				pEnum->Release();
				return 0;
			}
		}

		pPin->Release();
	}

	pEnum->Release();
	return -1;
}

int try_format_size(V4wState *s, int format, int width, int height, GUID *pPinCategory)
{
	HRESULT hr=S_OK;
	IEnumPins *pEnum=0;
	ULONG ulFound;
	IPin *pPin;

	GUID guid_format;
	DWORD biCompression;
	DWORD biBitCount;

	// Verify input
	if (!s->m_pDeviceFilter)
		return -1;

	if (format == MS_YUV420P)
		guid_format = (GUID)FOURCCMap(MAKEFOURCC('I','4','2','0'));
	else if (format == MS_YUYV)
		guid_format = MEDIASUBTYPE_YUYV;
	else if (format == MS_UYVY)
		guid_format = MEDIASUBTYPE_UYVY;
	else if (format == MS_RGB24)
		guid_format = MEDIASUBTYPE_RGB24;
	else if (format == MS_YUY2)
		guid_format = MEDIASUBTYPE_YUY2;

	if (format == MS_YUV420P)
		biCompression = MAKEFOURCC('I','4','2','0');
	else if (format == MS_YUYV)
		biCompression = MAKEFOURCC('Y','U','Y','V');
	else if (format == MS_UYVY)
		biCompression = MAKEFOURCC('U','Y','V','Y');
	else if (format == MS_RGB24)
		biCompression = BI_RGB;
	else if (format == MS_YUY2)
		biCompression = MAKEFOURCC('Y','U','Y','2');

	if (format == MS_YUV420P)
		biBitCount = 12;
	else if (format == MS_YUYV)
		biBitCount = 16;
	else if (format == MS_UYVY)
		biBitCount = 16;
	else if (format == MS_RGB24)
		biBitCount = 24;
	else if (format == MS_YUY2)
		biBitCount = 16;

	// Get pin enumerator
	hr = s->m_pDeviceFilter->EnumPins(&pEnum);
	if(FAILED(hr)) 
		return -1;

	pEnum->Reset();

	// Count every pin on the filter
	while(S_OK == pEnum->Next(1, &pPin, &ulFound))
	{
		PIN_DIRECTION pindir = (PIN_DIRECTION) 3;

		hr = pPin->QueryDirection(&pindir);

		if(pindir != PINDIR_INPUT)
		{
			IEnumMediaTypes *ppEnum;
			ULONG ulFound2;
			hr = pPin->EnumMediaTypes(&ppEnum);
			if(FAILED(hr)) 
				continue;

			GUID pCurrentPinCategory;
			GetPinCategory(pPin, &pCurrentPinCategory);
			if (*pPinCategory!=pCurrentPinCategory)
				continue;

			AM_MEDIA_TYPE *ppMediaTypes;
			while(S_OK == ppEnum->Next(1, &ppMediaTypes, &ulFound2))
			{
				if (ppMediaTypes->formattype != FORMAT_VideoInfo)
					continue;
				if (ppMediaTypes->majortype != MEDIATYPE_Video)
					continue;
				if (ppMediaTypes->subtype != guid_format)
					continue;
				VIDEOINFO *pvi = (VIDEOINFO *)ppMediaTypes->pbFormat;
				if (pvi->bmiHeader.biCompression!=biCompression)
					continue;
				if (pvi->bmiHeader.biBitCount!=biBitCount)
					continue;
				if (pvi->bmiHeader.biHeight!=height)
					continue;
				if (pvi->bmiHeader.biWidth!=width)
					continue;

				s->vsize.width = width;
				s->vsize.height = height;

				pPin->Release();
				pEnum->Release();
				return 0;
			}
		}

		pPin->Release();
	} 

	pEnum->Release();
	return -1;
}

static int v4w_configure_videodevice(V4wState *s)
{
	// Initialize COM
	CoInitialize(NULL);

	// get a Graph
	HRESULT hr= CoCreateInstance (CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, //IID_IBaseFilter,
		(void **)&s->m_pGraph);
	if(FAILED(hr))
	{
		return -1;
	}

	// get a CaptureGraphBuilder2
	hr= CoCreateInstance (CLSID_CaptureGraphBuilder2,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ICaptureGraphBuilder2, //IID_IBaseFilter,
		(void **)&s->m_pBuilder);
	if(FAILED(hr))
	{
		return -2;
	}

	// connect capture graph builder with the graph
	s->m_pBuilder->SetFiltergraph(s->m_pGraph);

	// get mediacontrol so we can start and stop the filter graph
	hr=s->m_pGraph->QueryInterface (IID_IMediaControl, (void **)&s->m_pControl);
	if(FAILED(hr))
	{
		return -3;
	}


	ICreateDevEnum *pCreateDevEnum = NULL;
	IEnumMoniker *pEnumMoniker = NULL;
	IMoniker *pMoniker = NULL;

	ULONG nFetched = 0;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
		IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);
	if(FAILED(hr))
	{
		return -4;
	}

	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumMoniker, 0);
	if (FAILED(hr) || pEnumMoniker == NULL) {
		//printf("no device\n");
		return -5;
	}

	pEnumMoniker->Reset();

	int pos=0;
	while(S_OK == pEnumMoniker->Next(1, &pMoniker, &nFetched) )
	{
		IPropertyBag *pBag;
		hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
		if( hr != S_OK )
			continue; 

		if (s->dev[0]=='\0')
			break;

		VARIANT var;
		VariantInit(&var);
		hr = pBag->Read( L"FriendlyName", &var, NULL ); 
		if( hr != S_OK )
		{
			pMoniker->Release();
			continue;
		}
		//USES_CONVERSION;
		char szName[256];

		WideCharToMultiByte(CP_UTF8,0,var.bstrVal,-1,szName,256,0,0);
		VariantClear(&var); 

		if (strcmp(szName, s->dev)==0)
			break;

		pMoniker->Release();
		pBag->Release();
		pMoniker=NULL;
		pBag=NULL;
	}

	if(pMoniker==NULL)
	{
		int pos=0;
		while(S_OK == pEnumMoniker->Next(1, &pMoniker, &nFetched) )
		{
			IPropertyBag *pBag;
			hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
			if( hr != S_OK )
				continue; 
		}

	}

	if(pMoniker==NULL)
	{
		return -6;
	}

	hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&s->m_pDeviceFilter );
	if(FAILED(hr))
	{
		return -7;
	}

	s->m_pGraph->AddFilter(s->m_pDeviceFilter, L"Device Filter");

	pMoniker->Release();
	pEnumMoniker->Release();
	pCreateDevEnum->Release();


	GUID pPinCategory;

	if (try_format(s->m_pDeviceFilter, s->pix_fmt, &pPinCategory)==0)
		s->pix_fmt = s->pix_fmt;
	else if (try_format(s->m_pDeviceFilter,MS_YUV420P, &pPinCategory)==0)
		s->pix_fmt = MS_YUV420P;
	else if (try_format(s->m_pDeviceFilter,MS_YUY2, &pPinCategory)==0)
		s->pix_fmt = MS_YUY2;
	else if (try_format(s->m_pDeviceFilter,MS_YUYV, &pPinCategory)==0)
		s->pix_fmt = MS_YUYV;
	else if (try_format(s->m_pDeviceFilter,MS_UYVY, &pPinCategory)==0)
		s->pix_fmt = MS_UYVY;
	else if (try_format(s->m_pDeviceFilter,MS_RGB24, &pPinCategory)==0)
		s->pix_fmt = MS_RGB24;
	else
	{
		ms_error("Unsupported video pixel format.");
		return -8;
	}

	if (s->pix_fmt == MS_YUV420P)
		ms_message("Driver supports YUV420P, using that format.");
	else if (s->pix_fmt == MS_YUY2)
		ms_message("Driver supports YUY2 (UYVY), using that format.");
	else if (s->pix_fmt == MS_YUYV)
		ms_message("Driver supports YUV422, using that format.");
	else if (s->pix_fmt == MS_UYVY)
		ms_message("Driver supports UYVY, using that format.");
	else if (s->pix_fmt == MS_RGB24)
		ms_message("Driver supports RGB24, using that format.");

	if (try_format_size(s, s->pix_fmt, s->vsize.width, s->vsize.height, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", s->vsize.width, s->vsize.height);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QCIF_W, MS_VIDEO_SIZE_QCIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QCIF_W, MS_VIDEO_SIZE_QCIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QVGA_W, MS_VIDEO_SIZE_QVGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QVGA_W, MS_VIDEO_SIZE_QVGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QQVGA_W, MS_VIDEO_SIZE_QQVGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QQVGA_W, MS_VIDEO_SIZE_QQVGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_NS1_W, MS_VIDEO_SIZE_NS1_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_NS1_W, MS_VIDEO_SIZE_NS1_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QSIF_W, MS_VIDEO_SIZE_QSIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QSIF_W, MS_VIDEO_SIZE_QSIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_SIF_W, MS_VIDEO_SIZE_SIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_SIF_W, MS_VIDEO_SIZE_SIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_VGA_W, MS_VIDEO_SIZE_VGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_VGA_W, MS_VIDEO_SIZE_VGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_SDTV_W, MS_VIDEO_SIZE_SDTV_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_SDTV_W, MS_VIDEO_SIZE_SDTV_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_288P_W, MS_VIDEO_SIZE_288P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_288P_W, MS_VIDEO_SIZE_288P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_448P_W, MS_VIDEO_SIZE_448P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_448P_W, MS_VIDEO_SIZE_448P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_576P_W, MS_VIDEO_SIZE_576P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_576P_W, MS_VIDEO_SIZE_576P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_1080P_W, MS_VIDEO_SIZE_1080P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_1080P_W, MS_VIDEO_SIZE_1080P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_4SIF_W, MS_VIDEO_SIZE_4SIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_4SIF_W, MS_VIDEO_SIZE_4SIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_SVGA_W, MS_VIDEO_SIZE_SVGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_SVGA_W, MS_VIDEO_SIZE_SVGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_XGA_W, MS_VIDEO_SIZE_XGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_XGA_W, MS_VIDEO_SIZE_XGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_WXGA_W, MS_VIDEO_SIZE_WXGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_WXGA_W, MS_VIDEO_SIZE_WXGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_HDTVP_W, MS_VIDEO_SIZE_HDTVP_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_HDTVP_W, MS_VIDEO_SIZE_HDTVP_H);
	else
	{
		ms_error("No supported size found for format.");
		/* size not supported? */
		return -9;
	}

	return 0;
}

static int v4w_open_videodevice(V4wState *s)
{
	// Initialize COM
	CoInitialize(NULL);

	// get a Graph
	HRESULT hr= CoCreateInstance (CLSID_FilterGraph,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, //IID_IBaseFilter,
		(void **)&s->m_pGraph);
	if(FAILED(hr))
	{
		return -1;
	}

	// get a CaptureGraphBuilder2
	hr= CoCreateInstance (CLSID_CaptureGraphBuilder2,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ICaptureGraphBuilder2, //IID_IBaseFilter,
		(void **)&s->m_pBuilder);
	if(FAILED(hr))
	{
		return -2;
	}

	// connect capture graph builder with the graph
	s->m_pBuilder->SetFiltergraph(s->m_pGraph);

	// get mediacontrol so we can start and stop the filter graph
	hr=s->m_pGraph->QueryInterface (IID_IMediaControl, (void **)&s->m_pControl);
	if(FAILED(hr))
	{
		return -3;
	}


	ICreateDevEnum *pCreateDevEnum = NULL;
	IEnumMoniker *pEnumMoniker = NULL;
	IMoniker *pMoniker = NULL;

	ULONG nFetched = 0;

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
		IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);
	if(FAILED(hr))
	{
		return -4;
	}

	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumMoniker, 0);
	if (FAILED(hr) || pEnumMoniker == NULL) {
		//printf("no device\n");
		return -5;
	}

	pEnumMoniker->Reset();

	int pos=0;
	while(S_OK == pEnumMoniker->Next(1, &pMoniker, &nFetched) )
	{
		IPropertyBag *pBag;
		hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
		if( hr != S_OK )
			continue; 

		if (s->dev[0]=='\0')
			break;

		VARIANT var;
		VariantInit(&var);
		hr = pBag->Read( L"FriendlyName", &var, NULL ); 
		if( hr != S_OK )
		{
			pMoniker->Release();
			continue;
		}
		//USES_CONVERSION;
		char szName[256];

		WideCharToMultiByte(CP_UTF8,0,var.bstrVal,-1,szName,256,0,0);
		VariantClear(&var); 

		if (strcmp(szName, s->dev)==0)
			break;

		pMoniker->Release();
		pBag->Release();
		pMoniker=NULL;
		pBag=NULL;
	}

	if(pMoniker==NULL)
	{
		return -6;
	}

	hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&s->m_pDeviceFilter );
	if(FAILED(hr))
	{
		return -7;
	}

	s->m_pGraph->AddFilter(s->m_pDeviceFilter, L"Device Filter");

	pMoniker->Release();
	pEnumMoniker->Release();
	pCreateDevEnum->Release();


	GUID pPinCategory;

	if (try_format(s->m_pDeviceFilter, s->pix_fmt, &pPinCategory)==0)
		s->pix_fmt = s->pix_fmt;
	else if (try_format(s->m_pDeviceFilter,MS_YUV420P, &pPinCategory)==0)
		s->pix_fmt = MS_YUV420P;
	else if (try_format(s->m_pDeviceFilter,MS_YUY2, &pPinCategory)==0)
		s->pix_fmt = MS_YUY2;
	else if (try_format(s->m_pDeviceFilter,MS_YUYV, &pPinCategory)==0)
		s->pix_fmt = MS_YUYV;
	else if (try_format(s->m_pDeviceFilter,MS_UYVY, &pPinCategory)==0)
		s->pix_fmt = MS_UYVY;
	else if (try_format(s->m_pDeviceFilter,MS_RGB24, &pPinCategory)==0)
		s->pix_fmt = MS_RGB24;
	else
	{
		ms_error("Unsupported video pixel format.");
		return -8;
	}

	if (s->pix_fmt == MS_YUV420P)
		ms_message("Driver supports YUV420P, using that format.");
	else if (s->pix_fmt == MS_YUY2)
		ms_message("Driver supports YUY2 (UYVY), using that format.");
	else if (s->pix_fmt == MS_YUYV)
		ms_message("Driver supports YUV422, using that format.");
	else if (s->pix_fmt == MS_UYVY)
		ms_message("Driver supports UYVY, using that format.");
	else if (s->pix_fmt == MS_RGB24)
		ms_message("Driver supports RGB24, using that format.");

	if (try_format_size(s, s->pix_fmt, s->vsize.width, s->vsize.height, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", s->vsize.width, s->vsize.height);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QCIF_W, MS_VIDEO_SIZE_QCIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QCIF_W, MS_VIDEO_SIZE_QCIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_CIF_W, MS_VIDEO_SIZE_CIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_4CIF_W, MS_VIDEO_SIZE_4CIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QVGA_W, MS_VIDEO_SIZE_QVGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QVGA_W, MS_VIDEO_SIZE_QVGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QQVGA_W, MS_VIDEO_SIZE_QQVGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QQVGA_W, MS_VIDEO_SIZE_QQVGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_NS1_W, MS_VIDEO_SIZE_NS1_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_NS1_W, MS_VIDEO_SIZE_NS1_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_QSIF_W, MS_VIDEO_SIZE_QSIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_QSIF_W, MS_VIDEO_SIZE_QSIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_SIF_W, MS_VIDEO_SIZE_SIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_SIF_W, MS_VIDEO_SIZE_SIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_VGA_W, MS_VIDEO_SIZE_VGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_VGA_W, MS_VIDEO_SIZE_VGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_SDTV_W, MS_VIDEO_SIZE_SDTV_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_SDTV_W, MS_VIDEO_SIZE_SDTV_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_288P_W, MS_VIDEO_SIZE_288P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_288P_W, MS_VIDEO_SIZE_288P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_448P_W, MS_VIDEO_SIZE_448P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_448P_W, MS_VIDEO_SIZE_448P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_576P_W, MS_VIDEO_SIZE_576P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_576P_W, MS_VIDEO_SIZE_576P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_720P_W, MS_VIDEO_SIZE_720P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_1080P_W, MS_VIDEO_SIZE_1080P_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_1080P_W, MS_VIDEO_SIZE_1080P_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_4SIF_W, MS_VIDEO_SIZE_4SIF_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_4SIF_W, MS_VIDEO_SIZE_4SIF_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_SVGA_W, MS_VIDEO_SIZE_SVGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_SVGA_W, MS_VIDEO_SIZE_SVGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_XGA_W, MS_VIDEO_SIZE_XGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_XGA_W, MS_VIDEO_SIZE_XGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_WXGA_W, MS_VIDEO_SIZE_WXGA_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_WXGA_W, MS_VIDEO_SIZE_WXGA_H);
	else if (try_format_size(s, s->pix_fmt, MS_VIDEO_SIZE_HDTVP_W, MS_VIDEO_SIZE_HDTVP_H, &pPinCategory)==0)
		ms_message("Selected Size: %ix%i.", MS_VIDEO_SIZE_HDTVP_W, MS_VIDEO_SIZE_HDTVP_H);
	else
	{
		ms_error("No supported size found for format.");
		/* size not supported? */
		return -9;
	}

	// get DXFilter
	s->m_pDXFilter = new CDXFilter(NULL, &hr, FALSE);
	if(s->m_pDXFilter==NULL)
	{
		return -10;
	}
	s->m_pDXFilter->AddRef();

	CMediaType mt;
	mt.SetType(&MEDIATYPE_Video);

	GUID m = MEDIASUBTYPE_RGB24;
	if (s->pix_fmt == MS_YUV420P)
		m = (GUID)FOURCCMap(MAKEFOURCC('I','4','2','0'));
	else if (s->pix_fmt == MS_YUY2)
		m = MEDIASUBTYPE_YUY2;
	else if (s->pix_fmt == MS_YUYV)
		m = MEDIASUBTYPE_YUYV;
	else if (s->pix_fmt == MS_UYVY)
		m = MEDIASUBTYPE_UYVY;
	else if (s->pix_fmt == MS_RGB24)
		m = MEDIASUBTYPE_RGB24;
	mt.SetSubtype(&m);

	mt.formattype = FORMAT_VideoInfo;
	mt.SetTemporalCompression(FALSE);

	VIDEOINFO *pvi = (VIDEOINFO *)
		mt.AllocFormatBuffer(sizeof(VIDEOINFO));
	if (NULL == pvi)
		return -11;
	ZeroMemory(pvi, sizeof(VIDEOINFO));

	if (s->pix_fmt == MS_YUV420P)
		pvi->bmiHeader.biCompression = MAKEFOURCC('I','4','2','0');
	else if (s->pix_fmt == MS_YUY2)
		pvi->bmiHeader.biCompression = MAKEFOURCC('Y','U','Y','2');
	else if (s->pix_fmt == MS_YUYV)
		pvi->bmiHeader.biCompression = MAKEFOURCC('Y','U','Y','V');
	else if (s->pix_fmt == MS_UYVY)
		pvi->bmiHeader.biCompression = MAKEFOURCC('U','Y','V','Y');
	else if (s->pix_fmt == MS_RGB24)
		pvi->bmiHeader.biCompression = BI_RGB;

	if (s->pix_fmt == MS_YUV420P)
		pvi->bmiHeader.biBitCount = 12;
	else if (s->pix_fmt == MS_YUY2)
		pvi->bmiHeader.biBitCount = 16;
	else if (s->pix_fmt == MS_YUYV)
		pvi->bmiHeader.biBitCount = 16;
	else if (s->pix_fmt == MS_UYVY)
		pvi->bmiHeader.biBitCount = 16;
	else if (s->pix_fmt == MS_RGB24)
		pvi->bmiHeader.biBitCount = 24;

	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth = s->vsize.width;
	pvi->bmiHeader.biHeight = s->vsize.height;
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;
	mt.SetSampleSize(pvi->bmiHeader.biSizeImage);

	mt.SetFormat((BYTE*)pvi, sizeof(VIDEOINFO));

	hr = s->m_pDXFilter->SetAcceptedMediaType(&mt);
	if(FAILED(hr))
	{
		return -12;
	}

	hr = s->m_pDXFilter->SetCallback(Callback); 
	if(FAILED(hr))
	{
		return -13;
	}

	hr = s->m_pDXFilter->QueryInterface(IID_IBaseFilter,
		(LPVOID *)&s->m_pIDXFilter);
	if(FAILED(hr))
	{
		return -14;
	}

	hr = s->m_pGraph->AddFilter(s->m_pIDXFilter, L"DXFilter Filter");
	if(FAILED(hr))
	{
		return -15;
	}


	// get null renderer
	hr=CoCreateInstance (CLSID_NullRenderer,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IBaseFilter,
		(void **)&s->m_pNullRenderer);
	if(FAILED(hr))
	{
		return -16;
	}
	if (s->m_pNullRenderer!=NULL)
	{
		s->m_pGraph->AddFilter(s->m_pNullRenderer, L"Null Renderer");
	}

	hr = s->m_pBuilder->RenderStream(&pPinCategory,
		&MEDIATYPE_Video, s->m_pDeviceFilter, s->m_pIDXFilter, s->m_pNullRenderer);
	if (FAILED(hr))
	{
		return -17;
	}

	IAMStreamConfig *pConfig = NULL;
	hr = s->m_pBuilder->FindInterface(
		&pPinCategory, // Preview pin.
		&MEDIATYPE_Video,    // Any media type.
		s->m_pDeviceFilter, // Pointer to the capture filter.
		IID_IAMStreamConfig, (void**)&pConfig); 
	if (pConfig!=NULL)
	{
		AM_MEDIA_TYPE *pType = NULL;
		int iCount, iSize;
		pConfig->GetNumberOfCapabilities(&iCount, &iSize);

		for (int i = 0; i < iCount; i++) {
			VIDEO_STREAM_CONFIG_CAPS scc;
			pType = NULL;
			pConfig->GetStreamCaps(i, &pType, (BYTE *)&scc);

			if (!((pType->formattype == FORMAT_VideoInfo) &&
				(pType->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
				(pType->pbFormat != NULL)))
				continue;

			VIDEOINFOHEADER & videoInfo = *(VIDEOINFOHEADER *)pType->pbFormat;

			if (m != pType->subtype)
				continue;

			if (videoInfo.bmiHeader.biWidth != s->vsize.width)
				continue;

			if (videoInfo.bmiHeader.biHeight != s->vsize.height)
				continue;

			if (videoInfo.bmiHeader.biBitCount != pvi->bmiHeader.biBitCount)
				continue;

			if (videoInfo.bmiHeader.biCompression != pvi->bmiHeader.biCompression)
				continue;

			videoInfo.AvgTimePerFrame = UNITS / (LONGLONG)s->fps;
			pConfig->SetFormat(pType);    
		}

		pConfig->GetFormat(&pType);
		if (pType!=NULL)
		{
			VIDEOINFO *pvi;
			pvi = (VIDEOINFO *)pType->pbFormat;
			ms_message("v4w: camera asked fps=%i // real fps=%i", (int)(UNITS / (LONGLONG)s->fps), pvi->AvgTimePerFrame);
		}

		pConfig->Release();
	}

	//m_pDXFilter->SetBufferSamples(TRUE);

	s_callback = s;
	hr = s->m_pControl->Run();
	if(FAILED(hr))
	{
		return -18;
	}


	s->rotregvalue=1;
	return 0;
}

static void v4w_init(MSFilter *f){
	V4wState *s=(V4wState *)ms_new0(V4wState,1);
	int idx;
	s->devidx=0;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	s->pix_fmt=MS_YUV420P;

	s->rotregvalue = 0;
	s->m_pGraph=NULL;
	s->m_pBuilder=NULL;
	s->m_pControl=NULL;
	s->m_pDXFilter=NULL;
	s->m_pIDXFilter=NULL;
	s->m_pDeviceFilter=NULL;

	qinit(&s->rq);
	for (idx=0;idx<10;idx++)
	{
		s->mire[idx]=NULL;
	}
	memset(s->nowebcamimage, 0, sizeof(s->nowebcamimage));
	ms_mutex_init(&s->mutex,NULL);
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
	memset(s->dev, 0, sizeof(s->dev));

	f->data=s;
}

static int _v4w_test(V4wState *s, void *arg)
{
	int i;
	i = v4w_configure_videodevice(s);

	if (i!=0)
	{
		s->pix_fmt = MS_YUV420P;
		s->vsize.width = MS_VIDEO_SIZE_CIF_W;
		s->vsize.height = MS_VIDEO_SIZE_CIF_H;
	}

	if (s->m_pGraph!=NULL)
	{
		if (s->m_pNullRenderer!=NULL)
			s->m_pGraph->RemoveFilter(s->m_pNullRenderer);
		if (s->m_pIDXFilter!=NULL)
			s->m_pGraph->RemoveFilter(s->m_pIDXFilter);
		if (s->m_pDeviceFilter!=NULL)
			s->m_pGraph->RemoveFilter(s->m_pDeviceFilter);
	}

	if (s->m_pNullRenderer)
		s->m_pNullRenderer->Release();
	if (s->m_pIDXFilter)
		s->m_pIDXFilter->Release();
	if (s->m_pDeviceFilter)
		s->m_pDeviceFilter->Release();

	if (s->m_pBuilder)
		s->m_pBuilder->Release();
	if (s->m_pControl)
		s->m_pControl->Release();
	if (s->m_pGraph)
		s->m_pGraph->Release();

	if (s->m_pDXFilter!=NULL)
		s->m_pDXFilter->Release();

	s->m_pNullRenderer=NULL;
	s->m_pIDXFilter=NULL;
	s->m_pDeviceFilter=NULL;
	s->m_pBuilder=NULL;
	s->m_pControl=NULL;
	s->m_pGraph=NULL;
	s->m_pDXFilter=NULL;

	CoUninitialize();
	s_callback = NULL;
	flushq(&s->rq,0);
	ms_message("v4w: checked device size=%ix%i format=%i (err=%i)", s->vsize.width, s->vsize.height, s->pix_fmt, i);

	return i;
}

static int _v4w_start(V4wState *s, void *arg)
{
	int i;
	s->frame_count=-1;

	i = v4w_open_videodevice(s);

	if (s->rotregvalue==0){
		if (s->m_pGraph!=NULL)
		{
			if (s->m_pNullRenderer!=NULL)
				s->m_pGraph->RemoveFilter(s->m_pNullRenderer);
			if (s->m_pIDXFilter!=NULL)
				s->m_pGraph->RemoveFilter(s->m_pIDXFilter);
			if (s->m_pDeviceFilter!=NULL)
				s->m_pGraph->RemoveFilter(s->m_pDeviceFilter);
		}

		if (s->m_pNullRenderer)
			s->m_pNullRenderer->Release();
		if (s->m_pIDXFilter)
			s->m_pIDXFilter->Release();
		if (s->m_pDeviceFilter)
			s->m_pDeviceFilter->Release();

		if (s->m_pBuilder)
			s->m_pBuilder->Release();
		if (s->m_pControl)
			s->m_pControl->Release();
		if (s->m_pGraph)
			s->m_pGraph->Release();

		if (s->m_pDXFilter!=NULL)
			s->m_pDXFilter->Release();

		s->m_pNullRenderer=NULL;
		s->m_pIDXFilter=NULL;
		s->m_pDeviceFilter=NULL;
		s->m_pBuilder=NULL;
		s->m_pControl=NULL;
		s->m_pGraph=NULL;
		s->m_pDXFilter=NULL;

		CoUninitialize();
		s_callback = NULL;
		flushq(&s->rq,0);
		ms_message("v4w: graph not started (err=%i)", i);
		s->rotregvalue=0;
		s->pix_fmt = MS_YUV420P;
	}
	return i;
}

static void v4w_uninit(MSFilter *f){
	V4wState *s=(V4wState*)f->data;
	int idx;
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	for (idx=0;idx<10;idx++)
	{
		if (s->mire[idx]==NULL)
			break;
		freemsg(s->mire[idx]);
	}
	if (s->rotregvalue>0){
		HRESULT hr = s->m_pControl->Stop();
		if(FAILED(hr))
		{
			ms_message("v4w: could not stop graph");
		}

		if (s->m_pGraph!=NULL)
		{
			if (s->m_pNullRenderer!=NULL)
				s->m_pGraph->RemoveFilter(s->m_pNullRenderer);
			if (s->m_pIDXFilter!=NULL)
				s->m_pGraph->RemoveFilter(s->m_pIDXFilter);
			if (s->m_pDeviceFilter!=NULL)
				s->m_pGraph->RemoveFilter(s->m_pDeviceFilter);
		}

		if (s->m_pNullRenderer)
			s->m_pNullRenderer->Release();
		if (s->m_pIDXFilter)
			s->m_pIDXFilter->Release();
		if (s->m_pDeviceFilter)
			s->m_pDeviceFilter->Release();

		if (s->m_pBuilder)
			s->m_pBuilder->Release();
		if (s->m_pControl)
			s->m_pControl->Release();
		if (s->m_pGraph)
			s->m_pGraph->Release();

		if (s->m_pDXFilter!=NULL)
			s->m_pDXFilter->Release();

		s->m_pNullRenderer=NULL;
		s->m_pIDXFilter=NULL;
		s->m_pDeviceFilter=NULL;
		s->m_pBuilder=NULL;
		s->m_pControl=NULL;
		s->m_pGraph=NULL;
		s->m_pDXFilter=NULL;

		CoUninitialize();
		s_callback = NULL;
		flushq(&s->rq,0);
		ms_message("v4w: graph destroyed");
		s->rotregvalue=0;
	}
	ms_free(s);
}

static mblk_t * v4w_make_nowebcam(V4wState *s){
#if defined(_WIN32_WCE)
	return NULL;
#else
	int idx;
	int count;
	if(s->mire[0]==NULL &&  s->frame_ind==0 && s->nowebcamimage[0] != '\0')
	{
		s->mire[0] = ms_load_jpeg_as_yuv(s->nowebcamimage,&s->vsize);
	}
	if (s->mire[0]==NULL && s->frame_ind==0){
		/* load several images to fake a movie */
		for (idx=0;idx<10;idx++)
		{
			s->mire[idx]=ms_load_nowebcam(&s->vsize, idx);
			if (s->mire[idx]==NULL)
				break;
		}
		if (idx==0)
			s->mire[0]=ms_load_nowebcam(&s->vsize, -1);
	}
	for (count=0;count<10;count++)
	{
		if (s->mire[count]==NULL)
			break;
	}

	s->frame_ind++;
	if (count==0)
		return NULL;

	idx = s->frame_ind%count;
	if (s->mire[idx]!=NULL)
		return s->mire[idx];
	return s->mire[0];
#endif
}

static void v4w_preprocess(MSFilter * obj){
	V4wState *s=(V4wState*)obj->data;
	if (s->rotregvalue==0)
		_v4w_start(s, NULL);
	if (s->rotregvalue==0)
		s->fps=1;
}

static void v4w_postprocess(MSFilter * obj){
	V4wState *s=(V4wState*)obj->data;
	s->start_time=0;
	s->frame_count=-1;
	flushq(&s->rq,0);
}

static void v4w_process(MSFilter * obj){
	V4wState *s=(V4wState*)obj->data;
	mblk_t *m;
	uint32_t timestamp;
	int cur_frame;

	if (s->frame_count==-1){
		s->start_time=obj->ticker->time;
		s->frame_count=0;
	}

	cur_frame=(int)((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>s->frame_count){
		mblk_t *om=NULL;
		ms_mutex_lock(&s->mutex);
		/*keep the most recent frame if several frames have been captured */
		if (s->rotregvalue!=0){
			while((m=getq(&s->rq))!=NULL){
				if (om!=NULL) freemsg(om);
				om=m;
			}
		}else {
			mblk_t *nowebcam = v4w_make_nowebcam(s);
			if (nowebcam!=NULL){
				om=dupmsg(nowebcam);
				mblk_set_precious_flag(om,1);
			}
		}
		ms_mutex_unlock(&s->mutex);
		if (om!=NULL){
			timestamp=(uint32_t)obj->ticker->time*90;/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			ms_queue_put(obj->outputs[0],om);
			/*ms_message("picture sent");*/
		}
		s->frame_count++;
	}
}



static int v4w_set_fps(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	s->fps=*((float*)arg);
	s->frame_count=-1; /* reset counter used for fps */
	return 0;
}


static int v4w_set_pix_fmt(MSFilter *f,void *arg){
	V4wState *s=(V4wState*)f->data;
	s->pix_fmt=*((MSPixFmt*)arg);
	return 0;
}

static int v4w_get_pix_fmt(MSFilter *f,void *arg){
	V4wState *s=(V4wState*)f->data;
	if (s->rotregvalue==0){
		_v4w_test(s, NULL); /* check supported format */
		*((MSPixFmt*)arg) = (MSPixFmt)s->pix_fmt;
		return 0;
	}
	*((MSPixFmt*)arg) = (MSPixFmt)s->pix_fmt;
	return 0;
}

static int v4w_set_vsize(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	s->vsize=*((MSVideoSize*)arg);
	return 0;
}

static int v4w_get_vsize(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	MSVideoSize *vs=(MSVideoSize*)arg;
	vs->width=s->vsize.width;
	vs->height=s->vsize.height;
	return 0;
}

static int v4w_set_device(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	s->devidx=*((int*)arg);
	return 0;
}

static int v4w_set_image(MSFilter *f, void *arg){
	int idx;
	V4wState *s=(V4wState*)f->data;
	char *image = (char *)arg;
	ms_mutex_lock(&s->mutex);
	if (image!=NULL && image[0]!='\0')
		snprintf(s->nowebcamimage, sizeof(s->nowebcamimage), "%s", image);
	else
		s->nowebcamimage[0] = '\0';
	for (idx=0;idx<10;idx++)
	{
		if (s->mire[idx]==NULL)
			break;
		freemsg(s->mire[idx]);
		s->mire[idx]=NULL;
	}
	s->frame_ind=0;
	ms_mutex_unlock(&s->mutex);
	return 0;
}

static int v4w_set_name(MSFilter *f, void *arg){
	V4wState *s=(V4wState*)f->data;
	snprintf(s->dev, sizeof(s->dev), (char*)arg);
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	v4w_set_fps	},
	{	MS_FILTER_SET_PIX_FMT	,	v4w_set_pix_fmt	},
	{	MS_FILTER_GET_PIX_FMT	,	v4w_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, v4w_set_vsize	},
	{	MS_FILTER_GET_VIDEO_SIZE, v4w_get_vsize	},
	{	MS_V4L_SET_DEVICE,	v4w_set_device },
	{	MS_FILTER_SET_IMAGE, v4w_set_image },
	{	0,	NULL }
};

#if defined(_MSC_VER) || defined(__cplusplus) 

MSFilterDesc ms_v4w_desc={
	MS_V4L_ID,
	"MSV4w",
	N_("A video4windows compatible source filter to stream pictures."),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	v4w_init,
	v4w_preprocess,
	v4w_process,
	v4w_postprocess,
	v4w_uninit,
	methods
};

#else

MSFilterDesc ms_v4w_desc={
	.id=MS_V4L_ID,
	.name="MSV4w",
	.text=N_("A video4windows compatible source filter to stream pictures."),
	.ninputs=0,
	.noutputs=1,
	.category=MS_FILTER_OTHER,
	.init=v4w_init,
	.preprocess=v4w_preprocess,
	.process=v4w_process,
	.postprocess=v4w_postprocess,
	.uninit=v4w_uninit,
	.methods=methods
};

#endif

MS_FILTER_DESC_EXPORT(ms_v4w_desc)

static MSFilter *vfw_create_reader(MSWebCam *obj){
	MSFilter *f=ms_filter_new_from_desc(&ms_v4w_desc);
	v4w_set_name(f,obj->name);
	return f;
}

static void vfw_detect(MSWebCamManager *obj);

static void vfw_cam_init(MSWebCam *cam){
}

MSWebCamDesc ms_directx_cam_desc={
	"DirectX Video Grabber",
	&vfw_detect,
	&vfw_cam_init,
	&vfw_create_reader,
	NULL
};

static void vfw_detect(MSWebCamManager *obj){
	ICreateDevEnum *pCreateDevEnum = NULL;
	IEnumMoniker *pEnumMoniker = NULL;
	IMoniker *pMoniker = NULL;
	HRESULT hr;

	ULONG nFetched = 0;

	// Initialize COM
	CoInitialize(NULL);

	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
		IID_ICreateDevEnum, (PVOID *)&pCreateDevEnum);
	if(FAILED(hr))
	{
		CoUninitialize();
		return ;
	}

	hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
		&pEnumMoniker, 0);
	if (FAILED(hr) || pEnumMoniker == NULL) {
		//printf("no device\n");
		CoUninitialize();
		return ;
	}

	pEnumMoniker->Reset();

	int pos=0;
	while(S_OK == pEnumMoniker->Next(1, &pMoniker, &nFetched) )
	{
		IPropertyBag *pBag;
		hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
		if( hr != S_OK )
			continue; 

		VARIANT var;
		VariantInit(&var);
		hr = pBag->Read( L"FriendlyName", &var, NULL ); 
		if( hr != S_OK )
		{
			pMoniker->Release();
			continue;
		}
		//USES_CONVERSION;
		char szName[256];

		WideCharToMultiByte(CP_UTF8,0,var.bstrVal,-1,szName,256,0,0);
		VariantClear(&var); 

		IBaseFilter *m_pDeviceFilter;
		hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&m_pDeviceFilter );
		if(SUCCEEDED(hr))
		{
			GUID pPinCategory;
			int fmt_supported = 0;

			//basic testing for the device.
			if (try_format(m_pDeviceFilter,MS_YUV420P, &pPinCategory)==0)
				fmt_supported = 1;
			else if (try_format(m_pDeviceFilter,MS_YUY2, &pPinCategory)==0)
				fmt_supported = 1;
			else if (try_format(m_pDeviceFilter,MS_YUYV, &pPinCategory)==0)
				fmt_supported = 1;
			else if (try_format(m_pDeviceFilter,MS_UYVY, &pPinCategory)==0)
				fmt_supported = 1;
			else if (try_format(m_pDeviceFilter,MS_RGB24, &pPinCategory)==0)
				fmt_supported = 1;
			else
			{
				ms_warning("Unsupported video pixel format/refuse camera (%s).", szName);
			}

			if (fmt_supported==1)
			{
				MSWebCam *cam=ms_web_cam_new(&ms_directx_cam_desc);
				cam->name=ms_strdup(szName);
				ms_web_cam_manager_add_cam(obj,cam);
			}
			m_pDeviceFilter->Release();
			m_pDeviceFilter=NULL;
		}


		pMoniker->Release();
		pBag->Release();
		pMoniker=NULL;
		pBag=NULL;
	}

	pEnumMoniker->Release();
	pCreateDevEnum->Release();
	CoUninitialize();
}
