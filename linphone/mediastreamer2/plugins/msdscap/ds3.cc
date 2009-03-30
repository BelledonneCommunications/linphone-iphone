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
};
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

class Callback: public ISampleGrabberCB
{
public:
  Callback(void);
  virtual ~Callback(void);
  STDMETHODIMP QueryInterface( REFIID riid, void **ppv );
  STDMETHODIMP_(ULONG) AddRef(void);
  STDMETHODIMP_(ULONG) Release(void);
  STDMETHODIMP SampleCB(double,IMediaSample*);
  STDMETHODIMP BufferCB(double,BYTE*,long);
protected:
  long m_refCount;
};

Callback::Callback(void): m_refCount(1)
{
#ifndef NDEBUG
  cerr << "Callback::Callback" << endl;
#endif
}

Callback::~Callback(void)
{
#ifndef NDEBUG
  cerr << "Callback::~Callback" << endl;
#endif
}

STDMETHODIMP Callback::QueryInterface(REFIID riid, void **ppv)
{
#ifndef NDEBUG
  cerr << "Callback::QueryInterface" << endl;
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

STDMETHODIMP_(ULONG) Callback::AddRef(void)
{
#ifndef NDEBUG
  cerr << "Callback::AddRef" << endl;
#endif
  m_refCount++;
  return m_refCount;
}

STDMETHODIMP_(ULONG) Callback::Release(void)
{
#ifndef NDEBUG
  cerr << "Callback::Release" << endl;
#endif
  if ( !InterlockedDecrement( &m_refCount ) ) delete this;
  return m_refCount;
}

STDMETHODIMP Callback::SampleCB( double, IMediaSample * )
{
#ifndef NDEBUG
  cerr << "Callback::SampleCB" << endl;
#endif
  return S_OK;
}

STDMETHODIMP Callback::BufferCB( double, BYTE *, long )
{
#ifndef NDEBUG
  cerr << "Callback::BufferCB" << endl;
#endif
  return E_NOTIMPL;
}

using namespace std;

int main(void)
{
  int retVal = 0;
  bool initialized = false;
  Callback *callback = NULL;
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
        cerr << "Setting format " << infoHeader->bmiHeader.biWidth
             << "x" << infoHeader->bmiHeader.biHeight << endl;
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
    callback = new Callback;
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

static void ms_dshow_detect(MSWebCamManager *obj);

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
}

extern "C" void libmsdscap_init(void){
		ms_web_cam_desc_register(&ms_dshow_cam_desc);
}
