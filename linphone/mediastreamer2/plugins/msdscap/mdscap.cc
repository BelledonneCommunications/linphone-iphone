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

#define FILTER_NAME L"HornetsEye Capture Filter"
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
  COERRORMACRO( filter->EnumPins( &enumPins ), Error, ,
                "Error getting pin enumerator" );
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

struct DscapState;

class MSCallback: public ISampleGrabberCB
{
public:
  MSCallback(DscapState *s);
  virtual ~MSCallback(void);
  STDMETHODIMP QueryInterface( REFIID riid, void **ppv );
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);
  STDMETHODIMP SampleCB(double,IMediaSample*);
  STDMETHODIMP BufferCB(double,BYTE*,long);
protected:
  long m_refCount;
  DscapState *mState;
};

#if 1
using namespace std;

int toto(void)
{
  int retVal = 0;
  bool initialized = false;
  MSCallback *callback = NULL;
  try {
    COERRORMACRO( CoInitialize(NULL), Error, , "CoInitialize failed" );
    initialized = true;
    ComPtr< IGraphBuilder > graphBuilder;
    graphBuilder.coCreateInstance( CLSID_FilterGraph, IID_IGraphBuilder,
                                   "Could not create graph builder "
                                   "interface" );
    cerr << "graphBuilder is " << graphBuilder.get() << endl;
    ComPtr< ICreateDevEnum > createDevEnum;
    createDevEnum.coCreateInstance( CLSID_SystemDeviceEnum,
                                    IID_ICreateDevEnum, "Could not create "
                                    "device enumerator" );
    ComPtr< IEnumMoniker > enumMoniker;
    COERRORMACRO( createDevEnum->CreateClassEnumerator
                  ( CLSID_VideoInputDeviceCategory, &enumMoniker, 0 ), Error, ,
                  "Error requesting moniker enumerator" );
    createDevEnum.reset();
    COERRORMACRO( enumMoniker->Reset(), Error, ,
                  "Error resetting moniker enumerator" );
    int index = 0;
    ComPtr< IMoniker > moniker;
    for ( int i=0; i<=index; i++ ) {
      ULONG fetched = 0;
      COERRORMACRO( enumMoniker->Next( 1, &moniker, &fetched ), Error, ,
                    "Error fetching next moniker" );
    };
    enumMoniker.reset();
    ComPtr< IBaseFilter > source;
    COERRORMACRO( moniker->BindToObject( 0, 0, IID_IBaseFilter,
                                         (void **)&source ), Error, ,
                  "Error binding moniker to base filter" );
    moniker.reset();
    COERRORMACRO( graphBuilder->AddFilter( source.get(), L"Source" ),
                  Error, , "Error adding camera source to filter graph" );
    ComPtr< IPin > sourceOut = getPin( source.get(), PINDIR_OUTPUT, 0 );
    ERRORMACRO( sourceOut.get() != NULL, Error, ,
                "Error getting output pin of camera source" );
    ComPtr< IAMStreamConfig > streamConfig;
    COERRORMACRO( sourceOut->
                  QueryInterface( IID_IAMStreamConfig,
                                  (void **)&streamConfig ),
                  Error, , "Error requesting stream configuration API" );
    int count, size;
    COERRORMACRO( streamConfig->GetNumberOfCapabilities( &count, &size ),
                  Error, , "Error getting number of capabilities" );
    bool ok = false;
    for ( int i=0; i<count; i++ ) {
      VIDEO_STREAM_CONFIG_CAPS videoConfig;
      AM_MEDIA_TYPE *mediaType;
      COERRORMACRO( streamConfig->GetStreamCaps( i, &mediaType,
                                                 (BYTE *)&videoConfig ),
                    Error, , "Error getting stream capabilities" );
      if ( mediaType->majortype == MEDIATYPE_Video &&
           mediaType->cbFormat != 0 ) {
        VIDEOINFOHEADER *infoHeader = (VIDEOINFOHEADER*)mediaType->pbFormat;
        // TODO: choose format here !!!
        ms_message("Setting format %ix%i",infoHeader->bmiHeader.biWidth,infoHeader->bmiHeader.biHeight);
        streamConfig->SetFormat( mediaType );
        ok = true;
      };
      if ( mediaType->cbFormat != 0 )
        CoTaskMemFree( (PVOID)mediaType->pbFormat );
      if ( mediaType->pUnk != NULL ) mediaType->pUnk->Release();
      CoTaskMemFree( (PVOID)mediaType );
      if ( ok )
        break;
    };
    streamConfig.reset();
    ERRORMACRO( ok, Error, , "Could not find any video format" );

    ComPtr< IBaseFilter > grabberBase;
    COERRORMACRO( CoCreateInstance( CLSID_SampleGrabber, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&grabberBase ),
                  Error, , "Error creating sample grabber" );
    COERRORMACRO( graphBuilder->AddFilter( grabberBase.get(), L"Grabber" ),
                  Error, , "Error adding sample grabber to filter graph" );
    ComPtr< ISampleGrabber > sampleGrabber;
    COERRORMACRO( grabberBase->QueryInterface( IID_ISampleGrabber,
                                               (void **)&sampleGrabber ),
                  Error, , "Error requesting sample grabber interface" );
    COERRORMACRO( sampleGrabber->SetOneShot( FALSE ), Error, ,
                  "Error disabling one-shot mode" );
    COERRORMACRO( sampleGrabber->SetBufferSamples( TRUE ), Error, ,
                  "Error enabling buffer sampling" );
    callback = new MSCallback(NULL);
    COERRORMACRO( sampleGrabber->SetCallBack( callback, 0 ), Error, ,
                  "Error setting callback interface for grabbing" );
    ComPtr< IPin > grabberIn = getPin( grabberBase.get(), PINDIR_INPUT, 0 );
    ERRORMACRO( grabberIn.get() != NULL, Error, ,
                "Error getting input of sample grabber" );
    ComPtr< IPin > grabberOut = getPin( grabberBase.get(), PINDIR_OUTPUT, 0 );
    ERRORMACRO( grabberOut.get() != NULL, Error, ,
                "Error getting output of sample grabber" );

    ComPtr< IBaseFilter > nullRenderer;
    COERRORMACRO( CoCreateInstance( CLSID_NullRenderer, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&nullRenderer ),
                  Error, , "Error creating Null Renderer" );
    COERRORMACRO( graphBuilder->AddFilter( nullRenderer.get(), L"Sink" ),
                  Error, , "Error adding null renderer to filter graph" );
    ComPtr< IPin > nullIn = getPin( nullRenderer.get(), PINDIR_INPUT, 0 );

    cerr << endl << "Attempting to connect" << endl;
    COERRORMACRO( graphBuilder->Connect( sourceOut.get(), grabberIn.get() ),
                  Error, , "Error connecting source to sample grabber" );
    COERRORMACRO( graphBuilder->Connect( grabberOut.get(), nullIn.get() ),
                  Error, , "Error connecting sample grabber to sink" );

    cerr << "Success!!!!!!!!!!!!!!!!!!!!!" << endl;

    ComPtr< IMediaControl > mediaControl;
    COERRORMACRO( graphBuilder->QueryInterface( IID_IMediaControl,
                                                (void **)&mediaControl ),
                  Error, , "Error requesting media control interface" );
    COERRORMACRO( mediaControl->Run(), Error, , "Error running graph" );

    ComPtr< IMediaEvent > mediaEvent;
    COERRORMACRO( graphBuilder->QueryInterface( IID_IMediaEvent,
                                                (void **)&mediaEvent ),
                  Error, , "Error requesting event interface" );

    cin.get();

    mediaControl->Stop();

    long evCode = 0;
    mediaEvent->WaitForCompletion( INFINITE, &evCode );

    // sourceOut.reset();
    // sinkIn.reset();
    // source.reset();
  } catch ( Error &e ) {
    cerr << e.what() << endl;
    retVal = 1;
  };
  if ( callback != NULL ) callback->Release();
  if ( initialized ) CoUninitialize();
  return retVal;
}

#endif 

struct DscapState{
	DscapState(){
		callback=0;
	}
	~DscapState(){
		if (callback) callback->Release();
	}
	int devid;
	MSVideoSize vsize;
	queue_t rq;
	ms_mutex_t mutex;
	int frame_ind;
	int frame_max;
	float fps;
	float start_time;
	int frame_count;
	MSPixFmt fmt;
	ComPtr< IBaseFilter > source;
	ComPtr< IBaseFilter > nullRenderer;
	ComPtr< IBaseFilter > grabberBase;
	ComPtr< IMediaControl > mediaControl;
	ComPtr< IMediaEvent > mediaEvent;
	MSCallback * callback;
};



MSCallback::MSCallback(DscapState *s): m_refCount(1), mState(s)
{
}

MSCallback::~MSCallback(void)
{
  ms_message("MSCallback::~MSCallback");
}

STDMETHODIMP MSCallback::QueryInterface(REFIID riid, void **ppv)
{
#ifndef NDEBUG
	ms_message("MSCallback::QueryInterface");
#endif
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

STDMETHODIMP_(ULONG) MSCallback::AddRef(void)
{
	ms_message("MSCallback::AddRef");
  m_refCount++;
  return m_refCount;
}

STDMETHODIMP_(ULONG) MSCallback::Release(void)
{
  ms_message("MSCallback::Release");

  if ( !InterlockedDecrement( &m_refCount ) ) {
		int refcnt=m_refCount;
		delete this;
		return refcnt;
	}
  return m_refCount;
}

static void dummy(void*p){
}

STDMETHODIMP MSCallback::SampleCB( double par1 , IMediaSample * sample)
{
	uint8_t *p;
	unsigned int size;
	if (sample->GetPointer(&p)!=S_OK){
		ms_error("error in GetPointer()");
		return S_OK;
	}
	size=sample->GetSize();
	ms_message( "MSCallback::SampleCB pointer=%p, size=%i",p,size);
	mblk_t *m=esballoc(p,size,0,dummy);
	m->b_wptr+=size;
	ms_mutex_lock(&mState->mutex);
	putq(&mState->rq,m);
	ms_mutex_unlock(&mState->mutex);
  	return S_OK;
}



STDMETHODIMP MSCallback::BufferCB( double, BYTE *b, long len)
{
	ms_message("MSCallback::BufferCB");

	return S_OK;
}

static void dscap_init(MSFilter *f){
	DscapState *s=new DscapState;
	s->vsize.width=MS_VIDEO_SIZE_CIF_W;
	s->vsize.height=MS_VIDEO_SIZE_CIF_H;
	qinit(&s->rq);
	ms_mutex_init(&s->mutex,NULL);
	s->start_time=0;
	s->frame_count=-1;
	s->fps=15;
	s->fmt=MS_YUV420P;
	f->data=s;
}



static void dscap_uninit(MSFilter *f){
	DscapState *s=(DscapState*)f->data;
	flushq(&s->rq,0);
	ms_mutex_destroy(&s->mutex);
	delete s;
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

static int select_best_format(DscapState *s, ComPtr<IAMStreamConfig> streamConfig, int count){
	int index;
	s->fmt=MS_YUV420P;
	index=find_best_format(streamConfig, count, &s->vsize, s->fmt);
	if (index!=-1) goto success;
	s->fmt=MS_YUY2;
	index=find_best_format(streamConfig, count, &s->vsize, s->fmt);
	if (index!=-1) goto success;
	s->fmt=MS_YUYV;
	index=find_best_format(streamConfig, count, &s->vsize, s->fmt);
	if (index!=-1) goto success;
	s->fmt=MS_RGB24;
	index=find_best_format(streamConfig, count, &s->vsize, s->fmt);
	if (index!=-1) {
		s->fmt=MS_RGB24_REV;
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

static void create_dshow_graph(DscapState *s){
	ComPtr< ICreateDevEnum > createDevEnum;
	COERRORMACRO( CoInitialize(NULL), Error, , "CoInitialize failed" );
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

    int index = 0;
    ULONG fetched = 0;
	ComPtr< IGraphBuilder > graphBuilder;
	graphBuilder.coCreateInstance( CLSID_FilterGraph, IID_IGraphBuilder,
                                   "Could not create graph builder "
                                   "interface" );
    ComPtr< IMoniker > moniker;
    for ( int i=0;enumMoniker->Next( 1, &moniker, &fetched )==S_OK;++i ) {
		if (i==s->devid){
			if (moniker->BindToObject( 0, 0, IID_IBaseFilter, (void **)&s->source )!=S_OK){
				ms_error("Error binding moniker to base filter" );
				return;
			}
		}
	}
	if (s->source.get()==0){
		ms_error("Could not interface with webcam devid=%i",s->devid);
		return;
	}
	moniker.reset();
    enumMoniker.reset();
    s->callback = new MSCallback(s);
    ms_message("Callback created");
    fflush(NULL);
    try{
    COERRORMACRO( graphBuilder->AddFilter( s->source.get(), L"Source" ),
                  Error, , "Error adding camera source to filter graph" );
    ComPtr< IPin > sourceOut = getPin( s->source.get(), PINDIR_OUTPUT, 0 );
    ERRORMACRO( sourceOut.get() != NULL, Error, ,
                "Error getting output pin of camera source" );
    ComPtr< IAMStreamConfig > streamConfig;
    COERRORMACRO( sourceOut->
                  QueryInterface( IID_IAMStreamConfig,
                                  (void **)&streamConfig ),
                  Error, , "Error requesting stream configuration API" );
    int count, size;
    COERRORMACRO( streamConfig->GetNumberOfCapabilities( &count, &size ),
                  Error, , "Error getting number of capabilities" );
    select_best_format(s,streamConfig,count);
    
    streamConfig.reset();

    COERRORMACRO( CoCreateInstance( CLSID_SampleGrabber, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&s->grabberBase ),
                  Error, , "Error creating sample grabber" );
    COERRORMACRO( graphBuilder->AddFilter( s->grabberBase.get(), L"Grabber" ),
                  Error, , "Error adding sample grabber to filter graph" );
    ComPtr< ISampleGrabber > sampleGrabber;
    COERRORMACRO( s->grabberBase->QueryInterface( IID_ISampleGrabber,
                                               (void **)&sampleGrabber ),
                  Error, , "Error requesting sample grabber interface" );
    COERRORMACRO( sampleGrabber->SetOneShot( FALSE ), Error, ,
                  "Error disabling one-shot mode" );
    COERRORMACRO( sampleGrabber->SetBufferSamples( TRUE ), Error, ,
                  "Error enabling buffer sampling" );

    COERRORMACRO( sampleGrabber->SetCallBack( s->callback, 0 ), Error, ,
                  "Error setting callback interface for grabbing" );
    ComPtr< IPin > grabberIn = getPin( s->grabberBase.get(), PINDIR_INPUT, 0 );
    ERRORMACRO( grabberIn.get() != NULL, Error, ,
                "Error getting input of sample grabber" );
    ComPtr< IPin > grabberOut = getPin( s->grabberBase.get(), PINDIR_OUTPUT, 0 );
    ERRORMACRO( grabberOut.get() != NULL, Error, ,
                "Error getting output of sample grabber" );

    
    COERRORMACRO( CoCreateInstance( CLSID_NullRenderer, NULL,
                                    CLSCTX_INPROC, IID_IBaseFilter,
                                    (void **)&s->nullRenderer ),
                  Error, , "Error creating Null Renderer" );
    COERRORMACRO( graphBuilder->AddFilter( s->nullRenderer.get(), L"Sink" ),
                  Error, , "Error adding null renderer to filter graph" );
    ComPtr< IPin > nullIn = getPin( s->nullRenderer.get(), PINDIR_INPUT, 0 );

    ms_message("Attempting to connect");
    COERRORMACRO( graphBuilder->Connect( sourceOut.get(), grabberIn.get() ),
                  Error, , "Error connecting source to sample grabber" );
    COERRORMACRO( graphBuilder->Connect( grabberOut.get(), nullIn.get() ),
                  Error, , "Error connecting sample grabber to sink" );

    ms_message("Success!!!!!!!!!!!!!!!!!!!!!");


	ms_message("Querying control interface");
    COERRORMACRO( graphBuilder->QueryInterface( IID_IMediaControl,
                                                (void **)&s->mediaControl ),
                  Error, , "Error requesting media control interface" );
	ms_message("Got the control interface (%p).",s->mediaControl.get());
    ms_message("Going to start the graph");
    HRESULT r;
	if ((r=s->mediaControl->Run())!=S_OK){
		ms_error("Error starting graph (%i)",r);
	}
    ms_message("Graph started");
    COERRORMACRO( graphBuilder->QueryInterface( IID_IMediaEvent,
                                                (void **)&s->mediaEvent ),
                  Error, , "Error requesting event interface" );
    ms_message("Graph created");
	} catch ( Error &e ) {
    	ms_error(e.what());
	};
}

static void dscap_preprocess(MSFilter * obj){
	DscapState *s=(DscapState*)obj->data;
	create_dshow_graph(s);
	//toto();
    ms_message("preprocess done.");
}

static void dscap_postprocess(MSFilter * obj){
	DscapState *s=(DscapState*)obj->data;
	if (s->mediaControl.get()!=NULL){
		s->mediaControl->Stop();
    	long evCode = 0;
    	s->mediaEvent->WaitForCompletion( INFINITE, &evCode );
	}
	flushq(&s->rq,0);
}

static void dscap_process(MSFilter * obj){
	DscapState *s=(DscapState*)obj->data;
	mblk_t *m;
	uint32_t timestamp;
	int cur_frame;

	if (s->frame_count==-1){
		s->start_time=(float)obj->ticker->time;
		s->frame_count=0;
	}

	cur_frame=(int)((obj->ticker->time-s->start_time)*s->fps/1000.0);
	if (cur_frame>s->frame_count){
		mblk_t *om=NULL;
		/*keep the most recent frame if several frames have been captured */
		
		ms_mutex_lock(&s->mutex);
		while((m=getq(&s->rq))!=NULL){
			ms_mutex_unlock(&s->mutex);
			if (om!=NULL) freemsg(om);
			om=m;
			ms_mutex_lock(&s->mutex);
		}
		ms_mutex_unlock(&s->mutex);
		if (om!=NULL){
			timestamp=(uint32_t)(obj->ticker->time*90);/* rtp uses a 90000 Hz clockrate for video*/
			mblk_set_timestamp_info(om,timestamp);
			ms_queue_put(obj->outputs[0],om);
		}
		s->frame_count++;
	}
}

static int dscap_set_fps(MSFilter *f, void *arg){
	DscapState *s=(DscapState*)f->data;
	s->fps=*((float*)arg);
	return 0;
}

static int dscap_get_pix_fmt(MSFilter *f,void *arg){
	DscapState *s=(DscapState*)f->data;
	*((MSPixFmt*)arg)=s->fmt;
	return 0;
}

static int dscap_set_vsize(MSFilter *f, void *arg){
	DscapState *s=(DscapState*)f->data;
	s->vsize=*((MSVideoSize*)arg);
	return 0;
}

static int dscap_get_vsize(MSFilter *f, void *arg){
	DscapState *s=(DscapState*)f->data;
	MSVideoSize *vs=(MSVideoSize*)arg;
	*vs=s->vsize;
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
	DscapState *s=(DscapState*)f->data;
	s->devid=(int)obj->data;
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
	int i;
	MSWebCam *cam;
	ComPtr<IPropertyBag> pBag;
	
	COERRORMACRO( CoInitialize(NULL), Error, , "CoInitialize failed" );
 
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
    
    int index = 0;
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
