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

// {4D6410BE-7643-4f43-B55F-8821A6FFB50A}
DEFINE_GUID(CLSID_DXFilter, 
0x4d6410be, 0x7643, 0x4f43, 0xb5, 0x5f, 0x88, 0x21, 0xa6, 0xff, 0xb5, 0xa);

// {52A7F345-CD92-442c-89C1-632C16AD5003}
DEFINE_GUID(IID_IDXFilter, 
0x52a7f345, 0xcd92, 0x442c, 0x89, 0xc1, 0x63, 0x2c, 0x16, 0xad, 0x50, 0x3);


// We define a callback typedef for this example. 
// Normally, you would make the DXFilter support a COM interface, 
// and in one of its methods you would pass in a pointer to a COM interface 
// used for calling back. See the DirectX documentation for the DXFilter
// for more information.

typedef HRESULT (*SAMPLECALLBACK) (
    IMediaSample * pSample, 
    REFERENCE_TIME * StartTime, 
    REFERENCE_TIME * StopTime,
    BOOL TypeChanged );


// We define the interface the app can use to program us
MIDL_INTERFACE("6B652FFF-11FE-4FCE-92AD-0266B5D7C78F")
IDXFilter : public IUnknown
{
    public:
        
        virtual HRESULT STDMETHODCALLTYPE SetAcceptedMediaType( 
            const CMediaType *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType( 
            CMediaType *pType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            SAMPLECALLBACK Callback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDeliveryBuffer( 
            ALLOCATOR_PROPERTIES props,
            BYTE *pBuffer) = 0;
};
        

class CDXFilterInPin;
class CDXFilter;

//----------------------------------------------------------------------------
// This is a special allocator that KNOWS that the person who is creating it
// will only create one of them. It allocates CMediaSamples that only 
// reference the buffer location that is set in the pin's renderer's
// data variable
//----------------------------------------------------------------------------

class CDXFilterAllocator : public CMemAllocator
{
    friend class CDXFilterInPin;
    friend class CDXFilter;

protected:

    // our pin who created us
    //
    CDXFilterInPin * m_pPin;

public:

    CDXFilterAllocator( CDXFilterInPin * pParent, HRESULT *phr ) 
        : CMemAllocator( TEXT("DXFilterAllocator\0"), NULL, phr ) 
        , m_pPin( pParent )
    {
    };

    ~CDXFilterAllocator( )
    {
        // wipe out m_pBuffer before we try to delete it. It's not an allocated
        // buffer, and the default destructor will try to free it!
        m_pBuffer = NULL;
    }

    HRESULT Alloc( );

    void ReallyFree();

    // Override this to reject anything that does not match the actual buffer
    // that was created by the application
    STDMETHODIMP SetProperties(ALLOCATOR_PROPERTIES *pRequest, ALLOCATOR_PROPERTIES *pActual);

};

//----------------------------------------------------------------------------
// we override the input pin class so we can provide a media type
// to speed up connection times. When you try to connect a filesourceasync
// to a transform filter, DirectShow will insert a splitter and then
// start trying codecs, both audio and video, video codecs first. If
// your sample grabber's set to connect to audio, unless we do this, it
// will try all the video codecs first. Connection times are sped up x10
// for audio with just this minor modification!
//----------------------------------------------------------------------------

class CDXFilterInPin : public CTransInPlaceInputPin
{
    friend class CDXFilterAllocator;
    friend class CDXFilter;

    CDXFilterAllocator * m_pPrivateAllocator;
    ALLOCATOR_PROPERTIES m_allocprops;
    BYTE * m_pBuffer;
    BOOL m_bMediaTypeChanged;

protected:

    CDXFilter * DXFilter( ) { return (CDXFilter*) m_pFilter; }
    HRESULT SetDeliveryBuffer( ALLOCATOR_PROPERTIES props, BYTE * m_pBuffer );

public:

    CDXFilterInPin( CTransInPlaceFilter * pFilter, HRESULT * pHr ) 
        : CTransInPlaceInputPin( TEXT("DXFilterInputPin\0"), pFilter, pHr, L"Input\0" )
        , m_pPrivateAllocator( NULL )
        , m_pBuffer( NULL )
        , m_bMediaTypeChanged( FALSE )
    {
        memset( &m_allocprops, 0, sizeof( m_allocprops ) );
    }

    ~CDXFilterInPin( )
    {
        if( m_pPrivateAllocator ) delete m_pPrivateAllocator;
    }

    // override to provide major media type for fast connects

    HRESULT GetMediaType( int iPosition, CMediaType *pMediaType );

    // override this or GetMediaType is never called

    STDMETHODIMP EnumMediaTypes( IEnumMediaTypes **ppEnum );

    // override this to refuse any allocators besides
    // the one the user wants, if this is set

    STDMETHODIMP NotifyAllocator( IMemAllocator *pAllocator, BOOL bReadOnly );

    // override this so we always return the special allocator, if necessary

    STDMETHODIMP GetAllocator( IMemAllocator **ppAllocator );

    HRESULT SetMediaType( const CMediaType *pmt );

    // we override this to tell whoever's upstream of us what kind of
    // properties we're going to demand to have
    //
    STDMETHODIMP GetAllocatorRequirements( ALLOCATOR_PROPERTIES *pProps );



};

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

class CDXFilter : public CTransInPlaceFilter,
                       public IDXFilter
{
    friend class CDXFilterInPin;
    friend class CDXFilterAllocator;

protected:

    CMediaType m_mtAccept;
    SAMPLECALLBACK m_callback;
    CCritSec m_Lock; // serialize access to our data

#if !defined(_WIN32_WCE)
    BOOL IsReadOnly( ) { return !m_bModifiesData; }
#endif

    // PURE, override this to ensure we get 
    // connected with the right media type
    HRESULT CheckInputType( const CMediaType * pmt );

    // PURE, override this to callback 
    // the user when a sample is received
    HRESULT Transform( IMediaSample * pms );

    // override this so we can return S_FALSE directly. 
    // The base class CTransInPlace
    // Transform( ) method is called by it's 
    // Receive( ) method. There is no way
    // to get Transform( ) to return an S_FALSE value 
    // (which means "stop giving me data"),
    // to Receive( ) and get Receive( ) to return S_FALSE as well.

    HRESULT Receive( IMediaSample * pms );

public:

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Expose IDXFilter
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);
    DECLARE_IUNKNOWN;

    CDXFilter( IUnknown * pOuter, HRESULT * pHr, BOOL ModifiesData );

    // IDXFilter
    STDMETHODIMP SetAcceptedMediaType( const CMediaType * pmt );
    STDMETHODIMP GetConnectedMediaType( CMediaType * pmt );
    STDMETHODIMP SetCallback( SAMPLECALLBACK Callback );
    STDMETHODIMP SetDeliveryBuffer( ALLOCATOR_PROPERTIES props, BYTE * m_pBuffer );
};
