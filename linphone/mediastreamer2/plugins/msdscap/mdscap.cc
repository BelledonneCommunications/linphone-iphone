/* msdscap - mediastreamer2 plugin for video capture using directshow 
   Copyright (C) 2009 Simon Morlat

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/
/* 
This plugin has been written by Simon Morlat based on the work made by
Jan Wedekind, posted on mingw tracker here:
http://sourceforge.net/tracker/index.php?func=detail&aid=1819367&group_id=2435&atid=302435
He wrote all the declarations missing to get directshow capture working
with mingw, and provided a demo code that worked great with minimal code.
*/

// This is a DirectShow interface. But maybe you'll find that it's easier to
// access the camera directly ;)

// http://www.codeguru.com/cpp/g-m/multimedia/video/article.php/c9551/
// http://lists.mplayerhq.hu/pipermail/ffmpeg-devel/2007-April/027965.html
// http://msdn2.microsoft.com/en-us/library/ms787594.aspx
// http://msdn2.microsoft.com/en-us/library/ms787867.aspx
// NullRenderer wih reference clock set to NULL
// http://www.videolan.org/
// http://git.videolan.org/gitweb.cgi?p=vlc.git;f=modules/access/dshow;hb=0.8.6

// #include <wtypes.h>
// #include <unknwn.h>
// #include <ole2.h>
// #include <limits.h>
// #include <dshow.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <windows.h>
#include <winnls.h>
#include <errors.h>
#include <initguid.h>
#include <ocidl.h>
#include <malloc.h>
#include "comptr.hh"
#include "error.hh"

#include <mediastreamer2/mswebcam.h>
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msticker.h>
#include <mediastreamer2/msvideo.h>

#define FILTER_NAME L"Mediasatreamer2 plugin for video capture"
#define PIN_NAME L"Capture"

using namespace Hornetseye;

DEFINE_GUID( CLSID_VideoInputDeviceCategory, 0x860BB310, 0x5D01,
             0x11d0, 0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86);
DEFINE_GUID( CLSID_SystemDeviceEnum, 0x62BE5D10, 0x60EB, 0x11d0,
             0xBD, 0x3B, 0x00, 0xA0, 0xC9, 0x11, 0xCE, 0x86 );
DEFINE_GUID( CLSID_FilterGraph, 0xe436ebb3, 0x524f, 0x11ce,
             0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID( CLSID_SampleGrabber, 0xc1f400a0, 0x3f08, 0x11d3,
             0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37 );
DEFINE_GUID( CLSID_NullRenderer,0xc1f400a4, 0x3f08, 0x11d3,
             0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37 );
DEFINE_GUID( CLSID_VfwCapture, 0x1b544c22, 0xfd0b, 0x11ce,
             0x8c, 0x63, 0x0, 0xaa, 0x00, 0x44, 0xb5, 0x1e);
DEFINE_GUID( IID_IGraphBuilder, 0x56a868a9, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70);
DEFINE_GUID( IID_IBaseFilter, 0x56a86895, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_ICreateDevEnum, 0x29840822, 0x5b84, 0x11d0,
             0xbd, 0x3b, 0x00, 0xa0, 0xc9, 0x11, 0xce, 0x86 );
DEFINE_GUID( IID_IEnumFilters, 0x56a86893, 0xad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IEnumPins, 0x56a86892, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IMediaSample, 0x56a8689a, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IMediaFilter, 0x56a86899, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IPin, 0x56a86891, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_ISampleGrabber, 0x6b652fff, 0x11fe, 0x4fce,
             0x92, 0xad, 0x02, 0x66, 0xb5, 0xd7, 0xc7, 0x8f );
DEFINE_GUID( IID_ISampleGrabberCB, 0x0579154a, 0x2b53, 0x4994,
             0xb0, 0xd0, 0xe7, 0x73, 0x14, 0x8e, 0xff, 0x85 );
DEFINE_GUID( IID_IMediaEvent, 0x56a868b6, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IMediaControl, 0x56a868b1, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IMemInputPin, 0x56a8689d, 0x0ad4, 0x11ce,
             0xb0, 0x3a, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );
DEFINE_GUID( IID_IAMStreamConfig, 0xc6e13340, 0x30ac, 0x11d0,
             0xa1, 0x8c, 0x00, 0xa0, 0xc9, 0x11, 0x89, 0x56 );
DEFINE_GUID( IID_IVideoProcAmp, 0x4050560e, 0x42a7, 0x413a,
             0x85, 0xc2, 0x09, 0x26, 0x9a, 0x2d, 0x0f, 0x44 );
DEFINE_GUID( MEDIATYPE_Video, 0x73646976, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( MEDIASUBTYPE_I420, 0x30323449, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);
DEFINE_GUID( MEDIASUBTYPE_YV12, 0x32315659, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( MEDIASUBTYPE_IYUV, 0x56555949, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( MEDIASUBTYPE_YUYV, 0x56595559, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( MEDIASUBTYPE_YUY2, 0x32595559, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( MEDIASUBTYPE_UYVY, 0x59565955, 0x0000, 0x0010,
             0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 );
DEFINE_GUID( MEDIASUBTYPE_RGB24, 0xe436eb7d, 0x524f, 0x11ce,
             0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70 );

using namespace std;

typedef LONGLONG REFERENCE_TIME;

typedef struct tagVIDEOINFOHEADER {
  RECT rcSource;
  RECT rcTarget;
  DWORD dwBitRate;
  DWORD dwBitErrorRate;
  REFERENCE_TIME AvgTimePerFrame;
  BITMAPINFOHEADER bmiHeader;
} VIDEOINFOHEADER;

typedef struct _AMMediaType {
  GUID majortype;
  GUID subtype;
  BOOL bFixedSizeSamples;
  BOOL bTemporalCompression;
  ULONG lSampleSize;
  GUID formattype;
  IUnknown *pUnk;
  ULONG cbFormat;
  BYTE *pbFormat;
} AM_MEDIA_TYPE;

DECLARE_ENUMERATOR_(IEnumMediaTypes,AM_MEDIA_TYPE*);

typedef struct _VIDEO_STREAM_CONFIG_CAPS
{
  GUID guid;
  ULONG VideoStandard;
  SIZE InputSize;
  SIZE MinCroppingSize;
  SIZE MaxCroppingSize;
  int CropGranularityX;
  int CropGranularityY;
  int CropAlignX;
  int CropAlignY;
  SIZE MinOutputSize;
  SIZE MaxOutputSize;
  int OutputGranularityX;
  int OutputGranularityY;
  int StretchTapsX;
  int StretchTapsY;
  int ShrinkTapsX;
  int ShrinkTapsY;
  LONGLONG MinFrameInterval;
  LONGLONG MaxFrameInterval;
  LONG MinBitsPerSecond;
  LONG MaxBitsPerSecond;
} VIDEO_STREAM_CONFIG_CAPS;

typedef LONGLONG REFERENCE_TIME;

typedef interface IBaseFilter IBaseFilter;
typedef interface IReferenceClock IReferenceClock;
typedef interface IFilterGraph IFilterGraph;

typedef enum _FilterState {
  State_Stopped,
  State_Paused,
  State_Running
} FILTER_STATE;

#define MAX_FILTER_NAME 128
typedef struct _FilterInfo {
  WCHAR achName[MAX_FILTER_NAME]; 
  IFilterGraph *pGraph;
} FILTER_INFO;

typedef enum _PinDirection {
  PINDIR_INPUT,
  PINDIR_OUTPUT
} PIN_DIRECTION;

#define MAX_PIN_NAME 128
typedef struct _PinInfo {
  IBaseFilter *pFilter;
  PIN_DIRECTION dir;
  WCHAR achName[MAX_PIN_NAME];
} PIN_INFO;

#define INTERFACE IPin
DECLARE_INTERFACE_(IPin,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Connect)(THIS_ IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(ReceiveConnection)(THIS_ IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(Disconnect)(THIS) PURE;
  STDMETHOD(ConnectedTo)(THIS_ IPin**) PURE;
  STDMETHOD(ConnectionMediaType)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(QueryPinInfo)(THIS_ PIN_INFO*) PURE;
  STDMETHOD(QueryDirection)(THIS_ PIN_DIRECTION*) PURE;
};
#undef INTERFACE

DECLARE_ENUMERATOR_(IEnumPins,IPin*);

typedef struct _AllocatorProperties {
  long cBuffers;
  long cbBuffer;
  long cbAlign;
  long cbPrefix;
} ALLOCATOR_PROPERTIES;

typedef LONG_PTR OAEVENT;

#define INTERFACE IMediaEvent
DECLARE_INTERFACE_(IMediaEvent,IDispatch)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetEventHandle)(THIS_ OAEVENT*) PURE;
  STDMETHOD(GetEvent)(THIS_ long*,LONG_PTR,LONG_PTR,long) PURE;
  STDMETHOD(WaitForCompletion)(THIS_ long,long*) PURE;
  STDMETHOD(CancelDefaultHandling)(THIS_ long) PURE;
  STDMETHOD(RestoreDefaultHandling)(THIS_ long) PURE;
  STDMETHOD(FreeEventParams)(THIS_ long,LONG_PTR,LONG_PTR) PURE;
};
#undef INTERFACE

typedef long OAFilterState;

#define INTERFACE IMediaControl
DECLARE_INTERFACE_(IMediaControl,IDispatch)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Run)(THIS) PURE;
  STDMETHOD(Pause)(THIS) PURE;
  STDMETHOD(Stop)(THIS) PURE;
  STDMETHOD(GetState)(THIS_ LONG,OAFilterState*) PURE;
  STDMETHOD(RenderFile)(THIS_ BSTR) PURE;
  STDMETHOD(AddSourceFilter)(THIS_ BSTR,IDispatch**) PURE;
  STDMETHOD(get_FilterCollection)(THIS_ IDispatch**) PURE;
  STDMETHOD(get_RegFilterCollection)(THIS_ IDispatch**) PURE;
  STDMETHOD(StopWhenReady)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IVideoProcAmp
DECLARE_INTERFACE_(IVideoProcAmp,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IAMStreamConfig
DECLARE_INTERFACE_(IAMStreamConfig,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetFormat)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(GetFormat)(THIS_ AM_MEDIA_TYPE**) PURE;
  STDMETHOD(GetNumberOfCapabilities)(THIS_ int*,int*) PURE;
  STDMETHOD(GetStreamCaps)(THIS_ int,AM_MEDIA_TYPE**,BYTE*) PURE;
};
#undef INTERFACE

#define INTERFACE IMediaFilter
DECLARE_INTERFACE_(IMediaFilter,IPersist)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Stop)(THIS) PURE;
  STDMETHOD(Pause)(THIS) PURE;
  STDMETHOD(Run)(THIS_ REFERENCE_TIME) PURE;
  STDMETHOD(GetState)(THIS_ DWORD,FILTER_STATE*) PURE;
  STDMETHOD(SetSyncSource)(THIS_ IReferenceClock*) PURE;
  STDMETHOD(GetSyncSource)(THIS_ IReferenceClock**) PURE;
};
#undef INTERFACE

#define INTERFACE IBaseFilter
DECLARE_INTERFACE_(IBaseFilter,IMediaFilter)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(EnumPins)(THIS_ IEnumPins**) PURE;
  STDMETHOD(FindPin)(THIS_ LPCWSTR,IPin**) PURE;
  STDMETHOD(QueryFilterInfo)(THIS_ FILTER_INFO*) PURE;
  STDMETHOD(JoinFilterGraph)(THIS_ IFilterGraph*,LPCWSTR) PURE;
  STDMETHOD(QueryVendorInfo)(THIS_ LPWSTR*) PURE;
};
#undef INTERFACE

DECLARE_ENUMERATOR_(IEnumFilters,IBaseFilter*);

// #define INTERFACE IEnumFilters
// DECLARE_INTERFACE_(IEnumFilters,IUnknown)
// {
//   STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
//   STDMETHOD_(ULONG,AddRef)(THIS) PURE;
//   STDMETHOD_(ULONG,Release)(THIS) PURE;
//   STDMETHOD(Next)(THIS_ ULONG,IBaseFilter**,ULONG*) PURE;
//   STDMETHOD(Skip)(THIS_ ULONG) PURE;
//   STDMETHOD(Reset)(THIS) PURE;
//   STDMETHOD(Clone)(THIS_ IEnumFilters**) PURE;
// };
// #undef INTERFACE

#define INTERFACE IFilterGraph
DECLARE_INTERFACE_(IFilterGraph,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(AddFilter)(THIS_ IBaseFilter*,LPCWSTR) PURE;
  STDMETHOD(RemoveFilter)(THIS_ IBaseFilter*) PURE;
  STDMETHOD(EnumFilters)(THIS_ IEnumFilters**) PURE;
  STDMETHOD(FindFilterByName)(THIS_ LPCWSTR,IBaseFilter**) PURE;
  STDMETHOD(ConnectDirect)(THIS_ IPin*,IPin*,const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(Reconnect)(THIS_ IPin*) PURE;
  STDMETHOD(Disconnect)(THIS_ IPin*) PURE;
  STDMETHOD(SetDefaultSyncSource)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IGraphBuilder
DECLARE_INTERFACE_(IGraphBuilder,IFilterGraph)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(Connect)(THIS_ IPin*,IPin*) PURE;
  STDMETHOD(Render)(THIS_ IPin*) PURE;
  STDMETHOD(RenderFile)(THIS_ LPCWSTR,LPCWSTR) PURE;
  STDMETHOD(AddSourceFilter)(THIS_ LPCWSTR,LPCWSTR,IBaseFilter**) PURE;
  STDMETHOD(SetLogFile)(THIS_ DWORD_PTR) PURE;
  STDMETHOD(Abort)(THIS) PURE;
  STDMETHOD(ShouldOperationContinue)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE ICreateDevEnum
DECLARE_INTERFACE_(ICreateDevEnum,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(CreateClassEnumerator)(THIS_ REFIID,IEnumMoniker**,DWORD) PURE;
};
#undef INTERFACE

#define INTERFACE IMediaSample
DECLARE_INTERFACE_(IMediaSample,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetPointer)(THIS_ BYTE **) PURE;
  STDMETHOD_(long, GetSize)(THIS) PURE;
};
/*
#define INTERFACE IMediaSample
DECLARE_INTERFACE_(IMediaSample, IUnknown)
{
    STDMETHOD(GetPointer)(THIS_ BYTE **) PURE;
    STDMETHOD_(long, GetSize)(THIS) PURE;
    STDMETHOD(GetTime)(THIS_ REFERENCE_TIME *, REFERENCE_TIME *) PURE;
    STDMETHOD(SetTime)(THIS_ REFERENCE_TIME *, REFERENCE_TIME *) PURE;
    STDMETHOD(IsSyncPoint)(THIS) PURE;
    STDMETHOD(SetSyncPoint)(THIS_ BOOL) PURE;
    STDMETHOD(IsPreroll)(THIS) PURE;
    STDMETHOD(SetPreroll)(THIS_ BOOL) PURE;
    STDMETHOD_(long, GetActualDataLength)(THIS) PURE;
    STDMETHOD(SetActualDataLength)(THIS_ long) PURE;
    STDMETHOD(GetMediaType)(THIS_ AM_MEDIA_TYPE **) PURE;
    STDMETHOD(SetMediaType)(THIS_ AM_MEDIA_TYPE *) PURE;
    STDMETHOD(IsDiscontinuity)(THIS) PURE;
    STDMETHOD(SetDiscontinuity)(THIS_ BOOL) PURE;
    STDMETHOD(GetMediaTime)(THIS_ LONGLONG *, LONGLONG *) PURE;
    STDMETHOD(SetMediaTime)(THIS_ LONGLONG *, LONGLONG *) PURE;
};
*/


#undef INTERFACE

#define INTERFACE IMemAllocator
DECLARE_INTERFACE_(IMemAllocator,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetProperties)(THIS_ ALLOCATOR_PROPERTIES*,ALLOCATOR_PROPERTIES*) PURE;
  STDMETHOD(GetProperties)(THIS_ ALLOCATOR_PROPERTIES*) PURE;
  STDMETHOD(Commit)(THIS) PURE;
  STDMETHOD(Decommit)(THIS) PURE;
  STDMETHOD(GetBuffer)(THIS_ IMediaSample **,REFERENCE_TIME*,REFERENCE_TIME*,DWORD) PURE;
  STDMETHOD(ReleaseBuffer)(THIS_ IMediaSample*) PURE;
};
#undef INTERFACE

#define INTERFACE IMemInputPin
DECLARE_INTERFACE_(IMemInputPin,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(GetAllocator)(THIS_ IMemAllocator**) PURE;
  STDMETHOD(NotifyAllocator)(THIS_ IMemAllocator*,BOOL) PURE;
  STDMETHOD(GetAllocatorRequirements)(THIS_ ALLOCATOR_PROPERTIES*) PURE;
  STDMETHOD(Receive)(THIS_ IMediaSample*) PURE;
  STDMETHOD(ReceiveMultiple)(THIS_ IMediaSample**,LONG,LONG*) PURE;
  STDMETHOD(ReceiveCanBlock)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE ISampleGrabberCB
DECLARE_INTERFACE_(ISampleGrabberCB,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SampleCB)(THIS_ double,IMediaSample*) PURE;
  STDMETHOD(BufferCB)(THIS_ double,BYTE*,long) PURE;
};
#undef INTERFACE

#define INTERFACE ISampleGrabber
DECLARE_INTERFACE_(ISampleGrabber,IUnknown)
{
  STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD(SetOneShot)(THIS_ BOOL) PURE;
  STDMETHOD(SetMediaType)(THIS_ const AM_MEDIA_TYPE*) PURE;
  STDMETHOD(GetConnectedMediaType)(THIS_ AM_MEDIA_TYPE*) PURE;
  STDMETHOD(SetBufferSamples)(THIS_ BOOL) PURE;
  STDMETHOD(GetCurrentBuffer)(THIS_ long*,long*) PURE;
  STDMETHOD(GetCurrentSample)(THIS_ IMediaSample**) PURE;
  STDMETHOD(SetCallBack)(THIS_ ISampleGrabberCB *,long) PURE;
};
#undef INTERFACE

ComPtr< IPin > getPin( IBaseFilter *filter, PIN_DIRECTION direction, int num )
{
	ComPtr< IPin > retVal;
	ComPtr< IEnumPins > enumPins;
	if (filter->EnumPins( &enumPins )!=S_OK){
		ms_error("Error getting pin enumerator" );
		return retVal;
	}
	ULONG found;
	ComPtr< IPin > pin;
	while ( enumPins->Next( 1, &pin, &found ) == S_OK ) {
		PIN_DIRECTION pinDirection = (PIN_DIRECTION)( -1 );
		pin->QueryDirection( &pinDirection );
		if ( pinDirection == direction ) {
			if ( num == 0 ) {
				retVal = pin;
				break;
			};
			num--;
		};
	};
	return retVal;
}



class DSCapture : public ISampleGrabberCB{
public:
	DSCapture(){
		qinit(&_rq);
		ms_mutex_init(&_mutex,NULL);
		_vsize=MS_VIDEO_SIZE_CIF;
		_fps=15;
		_start_time=0;
		_frame_count=0;
		_pixfmt=MS_YUV420P;
		_ready=false;
		m_refCount=1;
	}
	virtual ~DSCapture(){
		if (_ready) stopAndClean();
		flushq(&_rq,0);
		ms_mutex_destroy(&_mutex);
	}
	STDMETHODIMP QueryInterface( REFIID riid, void **ppv );
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP SampleCB(double,IMediaSample*);
	STDMETHODIMP BufferCB(double,BYTE*,long);
	int startDshowGraph();
	void stopAndClean();
	mblk_t *readFrame(){
		mblk_t *ret=NULL;
		ms_mutex_lock(&_mutex);
		ret=getq(&_rq);
		ms_mutex_unlock(&_mutex);
		return ret;
	}
	bool isTimeToSend(uint64_t ticker_time);
	MSVideoSize getVSize()const{
		return _vsize;
	}
	void setVSize(MSVideoSize vsize){
		_vsize=vsize;
	}
	void setFps(float fps){
		_fps=fps;
	}
	MSPixFmt getPixFmt(){
		if (!_ready) createDshowGraph(); /* so that _pixfmt is updated*/
		return _pixfmt;
	}
	void setDeviceIndex(int index){
		_devid=index;
	}
protected:
  	long m_refCount;
private:
	int createDshowGraph();
	int	selectBestFormat(ComPtr<IAMStreamConfig> streamConfig, int count);
	int _devid;
	MSVideoSize _vsize;
	queue_t _rq;
	ms_mutex_t _mutex;
	float _fps;
	float _start_time;
	int _frame_count;
	MSPixFmt _pixfmt;
	ComPtr< IGraphBuilder > _graphBuilder;
	ComPtr< IBaseFilter > _source;
	ComPtr< IBaseFilter > _nullRenderer;
	ComPtr< IBaseFilter > _grabberBase;
	ComPtr< IMediaControl > _mediaControl;
	ComPtr< IMediaEvent > _mediaEvent;
	bool _ready;
};


STDMETHODIMP DSCapture::QueryInterface(REFIID riid, void **ppv)
{
  HRESULT retval;
  if ( ppv == NULL ) return E_POINTER;
  /*
  if ( riid == IID_IUnknown ) {
    *ppv = static_cast< IUnknown * >( this );
    AddRef();
    retval = S_OK;
  } else if ( riid == IID_ISampleGrabberCB ) {
    *ppv = static_cast< ISampleGrabberCB * >( this );
    AddRef();
    retval = S_OK;
    } else */ {
#ifndef NDEBUG
    cerr << setbase( 16 ) << setfill('0')
         << "DEFINE_GUID( ..., 0x" << setw(8) << (int)riid.Data1 << ", 0x"
         << setw(4) << (int)riid.Data2 << "," << endl
         << "             0x"
         << setw(4) << (int)riid.Data3 << ", 0x" << setw(2)
         << (int)riid.Data4[0] << ", 0x"
         << (int)riid.Data4[1] << ", 0x"
         << (int)riid.Data4[2] << ", 0x"
         << (int)riid.Data4[3] << ", 0x"
         << (int)riid.Data4[4] << ", 0x"
         << (int)riid.Data4[5] << ", 0x"
         << (int)riid.Data4[6] << ", 0x"
         << (int)riid.Data4[7] << " ) ?" << endl
         << setfill( ' ' ) << setw( 0 ) << setbase( 10 );
#endif
    retval = E_NOINTERFACE;
  };
  return retval;
};

STDMETHODIMP_(ULONG) DSCapture::AddRef(){
	m_refCount++;
	return m_refCount;
}

STDMETHODIMP_(ULONG) DSCapture::Release()
{
  ms_message("DSCapture::Release");
  if ( !InterlockedDecrement( &m_refCount ) ) {
		int refcnt=m_refCount;
		delete this;
		return refcnt;
	}
  return m_refCount;
}

static void dummy(void*p){
}

STDMETHODIMP DSCapture::SampleCB( double par1 , IMediaSample * sample)
{
	uint8_t *p;
	unsigned int size;
	if (sample->GetPointer(&p)!=S_OK){
		ms_error("error in GetPointer()");
		return S_OK;
	}
	size=sample->GetSize();
	//ms_message( "DSCapture::SampleCB pointer=%p, size=%i",p,size);
	mblk_t *m=esballoc(p,size,0,dummy);
	m->b_wptr+=size;
	ms_mutex_lock(&_mutex);
	putq(&_rq,m);
	ms_mutex_unlock(&_mutex);
  	return S_OK;
}



STDMETHODIMP DSCapture::BufferCB( double, BYTE *b, long len)
{
	ms_message("DSCapture::BufferCB");
	return S_OK;
}

static void dscap_init(MSFilter *f){
	DSCapture *s=new DSCapture();
	f->data=s;
}



static void dscap_uninit(MSFilter *f){
	DSCapture *s=(DSCapture*)f->data;
	s->Release();
}

static char * fourcc_to_char(char *str, uint32_t fcc){
	memcpy(str,&fcc,4);
	str[4]='\0';
	return str;
}

static int find_best_format(ComPtr<IAMStreamConfig> streamConfig, int count, MSVideoSize *requested_size, MSPixFmt requested_fmt ){
	int i;
	MSVideoSize best_found=(MSVideoSize){32768,32768};
	int best_index=-1;
	char fccstr[5];
	char selected_fcc[5];
	for (i=0; i<count; i++ ) {
		VIDEO_STREAM_CONFIG_CAPS videoConfig;
		AM_MEDIA_TYPE *mediaType;
		COERRORMACRO( streamConfig->GetStreamCaps( i, &mediaType,
                                                 (BYTE *)&videoConfig ),
                    Error, , "Error getting stream capabilities" );
		if ( mediaType->majortype == MEDIATYPE_Video &&
           mediaType->cbFormat != 0 ) {
			VIDEOINFOHEADER *infoHeader = (VIDEOINFOHEADER*)mediaType->pbFormat;
			ms_message("Seeing format %ix%i %s",infoHeader->bmiHeader.biWidth,infoHeader->bmiHeader.biHeight,
					fourcc_to_char(fccstr,infoHeader->bmiHeader.biCompression));
			if (ms_fourcc_to_pix_fmt(infoHeader->bmiHeader.biCompression)==requested_fmt){
				MSVideoSize cur;
				cur.width=infoHeader->bmiHeader.biWidth;
				cur.height=infoHeader->bmiHeader.biHeight;
				if (ms_video_size_greater_than(cur,*requested_size)){
					if (ms_video_size_greater_than(best_found,cur)){
						best_found=cur;
						best_index=i;
						fourcc_to_char(selected_fcc,infoHeader->bmiHeader.biCompression);
					}
				}
			}
		};
		if ( mediaType->cbFormat != 0 )
			CoTaskMemFree( (PVOID)mediaType->pbFormat );
		if ( mediaType->pUnk != NULL ) mediaType->pUnk->Release();
			CoTaskMemFree( (PVOID)mediaType );
	}
	if (best_index!=-1) {
		*requested_size=best_found;
		ms_message("Best camera format is %s %ix%i",selected_fcc,best_found.width,best_found.height);
	}
	return best_index;
}

int DSCapture::selectBestFormat(ComPtr<IAMStreamConfig> streamConfig, int count){
	int index;
	_pixfmt=MS_YUV420P;
	index=find_best_format(streamConfig, count, &_vsize, _pixfmt);
	if (index!=-1) goto success;
	_pixfmt=MS_YUY2;
	index=find_best_format(streamConfig, count, &_vsize,_pixfmt);
	if (index!=-1) goto success;
	_pixfmt=MS_YUYV;
	index=find_best_format(streamConfig, count, &_vsize, _pixfmt);
	if (index!=-1) goto success;
	_pixfmt=MS_RGB24;
	index=find_best_format(streamConfig, count, &_vsize, _pixfmt);
	if (index!=-1) {
		_pixfmt=MS_RGB24_REV;
		goto success;
	}
	ms_error("This camera does not support any of our pixel formats.");
	return -1;
	
	success:
	VIDEO_STREAM_CONFIG_CAPS videoConfig;
	AM_MEDIA_TYPE *mediaType;
	COERRORMACRO( streamConfig->GetStreamCaps( index, &mediaType,
                                                 (BYTE *)&videoConfig ),
                                Error, , "Error getting stream capabilities" );
    streamConfig->SetFormat( mediaType );
    return 0;
}

int DSCapture::createDshowGraph(){
	ComPtr< ICreateDevEnum > createDevEnum;
	
	CoInitialize(NULL);
    createDevEnum.coCreateInstance( CLSID_SystemDeviceEnum,
                                    IID_ICreateDevEnum, "Could not create "
                                    "device enumerator" );
    ComPtr< IEnumMoniker > enumMoniker;
    if (createDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &enumMoniker, 0 )!=S_OK){
		ms_error("Fail to create class enumerator.");
		return -1;
	}
    createDevEnum.reset();
    enumMoniker->Reset();

    ULONG fetched = 0;
	_graphBuilder.coCreateInstance( CLSID_FilterGraph, IID_IGraphBuilder,
                                   "Could not create graph builder "
                                   "interface" );
    ComPtr< IMoniker > moniker;
    for ( int i=0;enumMoniker->Next( 1, &moniker, &fetched )==S_OK;++i ) {
		if (i==_devid){
			if (moniker->BindToObject( 0, 0, IID_IBaseFilter, (void **)&_source )!=S_OK){
				ms_error("Error binding moniker to base filter" );
				return -1;
			}
		}
	}
	if (_source.get()==0){
		ms_error("Could not interface with webcam devid=%i",_devid);
		return -1;
	}
	moniker.reset();
    enumMoniker.reset();
    if (_graphBuilder->AddFilter( _source.get(), L"Source" )!=S_OK){
    	ms_error("Error adding camera source to filter graph" );
    	return -1;
	}
    ComPtr< IPin > sourceOut = getPin( _source.get(), PINDIR_OUTPUT, 0 );
    if (sourceOut.get()==NULL){
		ms_error("Error getting output pin of camera source" );
		return -1;
	}
    ComPtr< IAMStreamConfig > streamConfig;
    if (sourceOut->QueryInterface( IID_IAMStreamConfig,
                                  (void **)&streamConfig )!=S_OK){
        ms_error("Error requesting stream configuration API" );
        return -1;
	}
    int count, size;
    if (streamConfig->GetNumberOfCapabilities( &count, &size )!=S_OK){
    	ms_error("Error getting number of capabilities" );
    	return -1;
	}
    if (selectBestFormat(streamConfig,count)!=0){
		return -1;
	}
    streamConfig.reset();

    if (CoCreateInstance( CLSID_SampleGrabber, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&_grabberBase )!=S_OK){
    	ms_error("Error creating sample grabber" );
    	return -1;
	}
    if (_graphBuilder->AddFilter( _grabberBase.get(), L"Grabber" )!=S_OK){
		ms_error("Error adding sample grabber to filter graph");
		return -1;
	}
    ComPtr< ISampleGrabber > sampleGrabber;
    if (_grabberBase->QueryInterface( IID_ISampleGrabber,
                                               (void **)&sampleGrabber )!=S_OK){
		ms_error("Error requesting sample grabber interface");
		return -1;
	}
    if (sampleGrabber->SetOneShot( FALSE )!=S_OK){
        ms_error("Error disabling one-shot mode" );
        return -1;
	}
    if (sampleGrabber->SetBufferSamples( TRUE )!=S_OK){
        ms_error("Error enabling buffer sampling" );
        return -1;
	}
	if (sampleGrabber->SetCallBack(this, 0 )!=S_OK){
		ms_error("Error setting callback interface for grabbing" );
		return -1;
	}
    ComPtr< IPin > grabberIn = getPin( _grabberBase.get(), PINDIR_INPUT, 0 );
    if (grabberIn.get() == NULL){
        ms_error("Error getting input of sample grabber");
		return -1;
	}
    ComPtr< IPin > grabberOut = getPin( _grabberBase.get(), PINDIR_OUTPUT, 0 );
	if (grabberOut.get()==NULL){
		ms_error("Error getting output of sample grabber" );
		return -1;
	}
    if (CoCreateInstance( CLSID_NullRenderer, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&_nullRenderer )!=S_OK){
		ms_error("Error creating Null Renderer" );
		return -1;
	}
	if (_graphBuilder->AddFilter( _nullRenderer.get(), L"Sink" )!=S_OK){
        ms_error("Error adding null renderer to filter graph" );
        return -1;
	}
    ComPtr< IPin > nullIn = getPin( _nullRenderer.get(), PINDIR_INPUT, 0 );
	if (_graphBuilder->Connect( sourceOut.get(), grabberIn.get() )!=S_OK){
    	ms_error("Error connecting source to sample grabber" );
    	return -1;
	}
    if (_graphBuilder->Connect( grabberOut.get(), nullIn.get() )!=S_OK){
        ms_error("Error connecting sample grabber to sink" );
        return -1;
	}
    ms_message("Directshow graph is now ready to run.");

    if (_graphBuilder->QueryInterface( IID_IMediaControl,
                                                (void **)&_mediaControl )!=S_OK){
        ms_error("Error requesting media control interface" );
		return -1;
	}
    if (_graphBuilder->QueryInterface( IID_IMediaEvent,
                                        (void **)&_mediaEvent )!=S_OK){
    	ms_error("Error requesting event interface" );
    	return -1;
	}
	_ready=true;
	return 0;
}

int DSCapture::startDshowGraph(){
	if (!_ready) {
		if (createDshowGraph()!=0) return -1;
	}
	HRESULT r=_mediaControl->Run();
	if (r!=S_OK && r!=S_FALSE){
		ms_error("Error starting graph (%i)",r);
		return -1;
	}
	ms_message("Graph started");
	return 0;
}

void DSCapture::stopAndClean(){
	if (_mediaControl.get()!=NULL){
		HRESULT r;
		r=_mediaControl->Stop();
		if (r!=S_OK){
			ms_error("msdscap: Could not stop graph !");
			fflush(NULL);
		}
    	_graphBuilder->RemoveFilter(_source.get());
    	_graphBuilder->RemoveFilter(_grabberBase.get());
		_graphBuilder->RemoveFilter(_nullRenderer.get());
	}
	_source.reset();
	_grabberBase.reset();
	_nullRenderer.reset();
	_mediaControl.reset();
	_mediaEvent.reset();
	_graphBuilder.reset();
	CoUninitialize();
	ms_mutex_lock(&_mutex);
	flushq(&_rq,0);
	ms_mutex_unlock(&_mutex);
	_ready=false;
}

bool DSCapture::isTimeToSend(uint64_t ticker_time){
	if (_frame_count==-1){
		_start_time=(float)ticker_time;
		_frame_count=0;
	}
	int cur_frame=(int)(((float)ticker_time-_start_time)*_fps/1000.0);
	if (cur_frame>_frame_count){
		_frame_count++;
		return true;
	}
	return false;
}

static void dscap_preprocess(MSFilter * obj){
	DSCapture *s=(DSCapture*)obj->data;
	s->startDshowGraph();
}

static void dscap_postprocess(MSFilter * obj){
	DSCapture *s=(DSCapture*)obj->data;
	s->stopAndClean();
}

static void dscap_process(MSFilter * obj){
	DSCapture *s=(DSCapture*)obj->data;
	mblk_t *m;
	uint32_t timestamp;

	if (s->isTimeToSend(obj->ticker->time)){
		mblk_t *om=NULL;
		/*keep the most recent frame if several frames have been captured */
		while((m=s->readFrame())!=NULL){
			if (om!=NULL) freemsg(om);
			om=m;
		}
		if (om!=NULL){
			timestamp=(uint32_t)(obj->ticker->time*90);/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			ms_queue_put(obj->outputs[0],om);
		}
	}
}

static int dscap_set_fps(MSFilter *f, void *arg){
	DSCapture *s=(DSCapture*)f->data;
	s->setFps(*(float*)arg);
	return 0;
}

static int dscap_get_pix_fmt(MSFilter *f,void *arg){
	DSCapture *s=(DSCapture*)f->data;
	*((MSPixFmt*)arg)=s->getPixFmt();
	return 0;
}

static int dscap_set_vsize(MSFilter *f, void *arg){
	DSCapture *s=(DSCapture*)f->data;
	s->setVSize(*((MSVideoSize*)arg));
	return 0;
}

static int dscap_get_vsize(MSFilter *f, void *arg){
	DSCapture *s=(DSCapture*)f->data;
	MSVideoSize *vs=(MSVideoSize*)arg;
	*vs=s->getVSize();
	return 0;
}

static MSFilterMethod methods[]={
	{	MS_FILTER_SET_FPS	,	dscap_set_fps	},
	{	MS_FILTER_GET_PIX_FMT	,	dscap_get_pix_fmt	},
	{	MS_FILTER_SET_VIDEO_SIZE, dscap_set_vsize	},
	{	MS_FILTER_GET_VIDEO_SIZE, dscap_get_vsize	},
	{	0								,	NULL			}
};

MSFilterDesc ms_dscap_desc={
	MS_FILTER_PLUGIN_ID,
	"MSDsCap",
	N_("A webcam grabber based on directshow."),
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	dscap_init,
	dscap_preprocess,
	dscap_process,
	dscap_postprocess,
	dscap_uninit,
	methods
};


static void ms_dshow_detect(MSWebCamManager *obj);
static MSFilter * ms_dshow_create_reader(MSWebCam *obj){
	MSFilter *f=ms_filter_new_from_desc(&ms_dscap_desc);
	DSCapture *s=(DSCapture*)f->data;
	s->setDeviceIndex((int)obj->data);
	return f;
}

static MSWebCamDesc ms_dshow_cam_desc={
	"Directshow capture",
	&ms_dshow_detect,
	NULL,
	&ms_dshow_create_reader,
	NULL
};

static void ms_dshow_detect(MSWebCamManager *obj){
	ComPtr<IPropertyBag> pBag;
	
	CoInitialize(NULL);
 
    ComPtr< ICreateDevEnum > createDevEnum;
    createDevEnum.coCreateInstance( CLSID_SystemDeviceEnum,
                                    IID_ICreateDevEnum, "Could not create "
                                    "device enumerator" );
    ComPtr< IEnumMoniker > enumMoniker;
    if (createDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &enumMoniker, 0 )!=S_OK){
		ms_error("Fail to create class enumerator.");
		return;
	}
    createDevEnum.reset();
    enumMoniker->Reset();
    
    ULONG fetched = 0;
    ComPtr< IMoniker > moniker;
    for ( int i=0;enumMoniker->Next( 1, &moniker, &fetched )==S_OK;++i ) {
		VARIANT var;
		if (moniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag )!=S_OK)
			continue;
		VariantInit(&var);
		if (pBag->Read( L"FriendlyName", &var, NULL )!=S_OK)
			continue;
    	char szName[256];
		WideCharToMultiByte(CP_ACP,0,var.bstrVal,-1,szName,256,0,0);
		MSWebCam *cam=ms_web_cam_new(&ms_dshow_cam_desc);
    	cam->name=ms_strdup(szName);
    	cam->data=(void*)i;
		ms_web_cam_manager_prepend_cam(obj,cam);
    	VariantClear(&var);
	}
    enumMoniker.reset();
}

extern "C" void libmsdscap_init(void){
		ms_web_cam_manager_register_desc(ms_web_cam_manager_get(),&ms_dshow_cam_desc);
}
