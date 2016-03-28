//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#include "stdafx.h"

#include "Codec.h"
#include "EncoderFilter.h"
#include "DecoderFilter.h"

CAppModule _Module;
CMessageLoop _loop;
DWORD _MessageLoopThreadID;

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

//////////////////////////////////////////////////////////////////////////
// Encoder filter setup data

const AMOVIESETUP_MEDIATYPE sudEncoderInputPinTypes[] =
{
    { 
        &MEDIATYPE_Video,           // Major type
        &MEDIASUBTYPE_RGB32         // Minor type
    },
    { 
        &MEDIATYPE_Video,           // Major type
        &MEDIASUBTYPE_RGB24         // Minor type
    },
};

const AMOVIESETUP_MEDIATYPE sudEncoderOutputPinTypes[] =
{
    {
        &MEDIATYPE_Video,       // Major type
        &MEDIASUBTYPE_BLVC      // Minor type
    }
};

const AMOVIESETUP_PIN sudEncoderPins[] =
{
    {
        L"Input",                       // Obsolete, not used.
        FALSE,                          // Is this pin rendered?
        FALSE,                          // Is it an output pin?
        FALSE,                          // Can the filter create zero instances?
        FALSE,                          // Does the filter create multiple instances?
        &CLSID_NULL,                    // Obsolete.
        NULL,                           // Obsolete.
        _countof(sudEncoderInputPinTypes),     // Number of media types.
        sudEncoderInputPinTypes                // Pointer to media types.
    },
    {
        L"Output",                      // Obsolete, not used.
        FALSE,                          // Is this pin rendered?
        TRUE,                           // Is it an output pin?
        FALSE,                          // Can the filter create zero instances?
        FALSE,                          // Does the filter create multiple instances?
        &CLSID_NULL,                    // Obsolete.
        NULL,                           // Obsolete.
        _countof(sudEncoderOutputPinTypes),    // Number of media types.
        sudEncoderOutputPinTypes               // Pointer to media types.
    }
};

const AMOVIESETUP_FILTER sudEncoderFilter =
{
    &CLSID_BytescoutLosslessVideoEncoder,       // Filter CLSID
    g_sEncoderLongName,                         // String name
    MERIT_DO_NOT_USE,                           // Filter merit
    _countof(sudEncoderPins),                   // Number pins
    sudEncoderPins                              // Pin details
};

//////////////////////////////////////////////////////////////////////////
// Decoder filter setup data

const AMOVIESETUP_MEDIATYPE sudDecoderInputPinTypes[] =
{
    {
        &MEDIATYPE_Video,       // Major type
        &MEDIASUBTYPE_BLVC      // Minor type
    }
};

const AMOVIESETUP_MEDIATYPE sudDecoderOutputPinTypes[] =
{
    { 
        &MEDIATYPE_Video,           // Major type
        &MEDIASUBTYPE_RGB32         // Minor type
    },
    { 
        &MEDIATYPE_Video,           // Major type
        &MEDIASUBTYPE_RGB24         // Minor type
    },
};

const AMOVIESETUP_PIN sudDecoderPins[] =
{
    {
        L"Input",                       // Obsolete, not used.
        FALSE,                          // Is this pin rendered?
        FALSE,                          // Is it an output pin?
        FALSE,                          // Can the filter create zero instances?
        FALSE,                          // Does the filter create multiple instances?
        &CLSID_NULL,                    // Obsolete.
        NULL,                           // Obsolete.
        _countof(sudDecoderInputPinTypes),     // Number of media types.
        sudDecoderInputPinTypes                // Pointer to media types.
    },
    {
        L"Output",                      // Obsolete, not used.
        FALSE,                          // Is this pin rendered?
        TRUE,                           // Is it an output pin?
        FALSE,                          // Can the filter create zero instances?
        FALSE,                          // Does the filter create multiple instances?
        &CLSID_NULL,                    // Obsolete.
        NULL,                           // Obsolete.
        _countof(sudDecoderOutputPinTypes),    // Number of media types.
        sudDecoderOutputPinTypes               // Pointer to media types.
    }
};

const AMOVIESETUP_FILTER sudDecoderFilter =
{
    &CLSID_BytescoutLosslessVideoDecoder,       // Filter CLSID
    g_sDecoderLongName,                         // String name
    MERIT_DO_NOT_USE,                           // Filter merit
    _countof(sudDecoderPins),                   // Number pins
    sudDecoderPins                              // Pin details
};

CFactoryTemplate g_Templates[] =
{
    { 
        g_sEncoderLongName,                     // Name
        &CLSID_BytescoutLosslessVideoEncoder,   // CLSID
        CEncoderFilter::CreateInstance,         // Method to create an instance of filter
        NULL,                                   // Initialization function
        &sudEncoderFilter                       // Set-up information (for filters)
    },
    { 
        g_sDecoderLongName,                     // Name
        &CLSID_BytescoutLosslessVideoDecoder,   // CLSID
        CDecoderFilter::CreateInstance,         // Method to create an instance of filter
        NULL,                                   // Initialization function
        &sudDecoderFilter                       // Set-up information (for filters)
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);    

// Encoder filter should be registered in Video Compressors category.
// In order to accomplish this task we need to implement customized
// DllRegisterServer and DllUnregisterServer.
// Customized versions will register|unregister encoder|decoder filters in
// general Directshow filters category AND register|unregister encoder
// filter in Video Compressors category.

REGFILTER2 rf2FilterReg = 
{
    1,                                          // Version 1 (no pin mediums or pin category).
    MERIT_NORMAL,                               // Merit.
    _countof(sudEncoderPins),                   // Number of pins.
    (REGFILTERPINS*)&sudEncoderPins             // Pointer to pin information.
};

STDAPI DllRegisterServer(void)
{
    HRESULT hr = AMovieDllRegisterServer2(TRUE);
    if (FAILED(hr))
        return hr;

    IFilterMapper2* pFM2 = NULL;
    hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void **)&pFM2);
    if (FAILED(hr))
        return hr;

    hr = pFM2->RegisterFilter(
        CLSID_BytescoutLosslessVideoEncoder,    // Filter CLSID. 
        g_sEncoderLongName,                     // Filter name.
        NULL,                                   // Device moniker. 
        &CLSID_VideoCompressorCategory,         // Video compressor category.
        g_sEncoderLongName,                     // Instance data.
        &rf2FilterReg                           // Pointer to filter information.
        );

    pFM2->Release();
    return hr;
}

STDAPI DllUnregisterServer()
{
    HRESULT hr = AMovieDllRegisterServer2(FALSE);
    if (FAILED(hr))
        return hr;
    
    IFilterMapper2* pFM2 = NULL;
    hr = CoCreateInstance(CLSID_FilterMapper2, NULL, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void **)&pFM2);
    if (FAILED(hr))
        return hr;

    hr = pFM2->UnregisterFilter(&CLSID_VideoCompressorCategory, g_sEncoderLongName, CLSID_BytescoutLosslessVideoEncoder);
    pFM2->Release();
    return hr;
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    g_hInst = (HINSTANCE)hModule;
    HRESULT hRes = S_OK;
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            hRes = _Module.Init(NULL, (HINSTANCE)hModule);
            ATLASSERT(SUCCEEDED(hRes));
            _MessageLoopThreadID = ::GetCurrentThreadId();
            ATLASSERT(_Module.AddMessageLoop(&_loop));
            break;
        case DLL_PROCESS_DETACH:
            _Module.RemoveMessageLoop();
            _Module.Term();
            break;
    }

    return DllEntryPoint((HINSTANCE) (hModule), dwReason, lpReserved);
}

extern "C" LRESULT WINAPI DriverProc(DWORD dwDriverId, HDRVR hDriver, UINT uMsg, LPARAM lParam1, LPARAM lParam2) 
{
#pragma warning(push)
#pragma warning(disable: 4312)
    Codec* codec = (Codec*)dwDriverId;
#pragma warning(pop)

    ICINFO* icinfo = NULL;

    switch (uMsg)
    {
        case DRV_LOAD:
        case DRV_FREE:
        case DRV_DISABLE:
        case DRV_ENABLE:
        case DRV_INSTALL:
        case DRV_REMOVE:
            return DRV_OK;
        
        case DRV_QUERYCONFIGURE:
        case DRV_CONFIGURE:
            return DRV_OK;

        case DRV_OPEN:
            // This is where a new codec instance is created.
            codec = new Codec();		
            return (LRESULT)codec;

        case DRV_CLOSE:
            delete codec;
            return DRV_OK;

        case ICM_GETINFO:
            // queries a video compression driver to return a description of
            // itself in an ICINFO structure.
            icinfo = (ICINFO*)lParam1;

            icinfo->fccType = ICTYPE_VIDEO;
            icinfo->fccHandler = CODEC_FOURCC;
            icinfo->dwFlags = VIDCF_FASTTEMPORALC | VIDCF_FASTTEMPORALD | VIDCF_TEMPORAL;
            icinfo->dwVersion = 0;
            icinfo->dwVersionICM = ICVERSION;

            wcscpy(icinfo->szName, CODEC_NAME); 
            wcscpy(icinfo->szDescription, CODEC_DESCRIPTION);

            return lParam2;

        case ICM_ABOUT:
            // notifies a video compression driver to display its About dialog 
            // box or queries a video compression driver to determine if it has
            // an About dialog box

            // If the app calls this with -1, it just wants to know if you have an
            // About box, so just return ok.

            if (lParam1 != -1)
                codec->About((HWND)lParam1);
            return ICERR_OK;

        case ICM_CONFIGURE:
            // notifies a video compression driver to display its configuration
            // dialog box or queries a video compression driver to determine if
            // it has a configuration dialog box. 

            // If the app calls this with -1, it just wants to know if you have a
            // configure box, so just return ok.

            if (lParam1 != -1)
                codec->Configure((HWND)lParam1);
            return ICERR_OK;

        case ICM_GETSTATE:
            // queries a video compression driver to return its current 
            // configuration in a block of memory or to determine the amount of
            // memory required to retrieve the configuration information. 

            // If the first parameter is NULL, it needs to return the size
            // needed to store the config info. Otherwise the first parameter
            // is a pointer to the config info, and the 2nd is the size in bytes.

            if (lParam1 == NULL)
                return sizeof(CodecCoreSettings);
            else
                memcpy((void*)lParam1, codec, min(lParam2, sizeof(CodecCoreSettings)));
            return ICERR_OK;

        case ICM_SETSTATE:
            // notifies a video compression driver to set the state of 
            // the compressor.

            // if the first parameter is NULL, it should reset things to 
            // defaults. Otherwise the first one is a pointer to a set of
            // config info, and the second its size in bytes.

            if (lParam1 == NULL)
            {
                codec->SetDefaults();
            }
            else
            {
                memcpy((void*)codec, (void*)lParam1, codec->wSize);
                lParam2 = codec->wSize;
            }
            return ICERR_OK;

        case ICM_GET:
        case ICM_SET:
            return ICERR_OK;

        //////////////////////////////////////////////////////////////////////////
        // Compression section.
        //////////////////////////////////////////////////////////////////////////

        case ICM_COMPRESS:
            return codec->Compress((ICCOMPRESS*)lParam1, (DWORD)lParam2);

        case ICM_COMPRESS_QUERY:
            return codec->CompressQuery((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_COMPRESS_GET_FORMAT:
            return codec->CompressGetFormat((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_COMPRESS_GET_SIZE:
            return codec->CompressGetSize((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_COMPRESS_FRAMES_INFO:
            // notifies a compression driver to set the parameters for the 
            // pending compression.
            return codec->CompressFramesInfo((ICCOMPRESSFRAMES*)lParam1);

        case ICM_COMPRESS_BEGIN:
            return codec->CompressBegin((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_COMPRESS_END:
            return codec->CompressEnd((int)lParam1, (int)lParam2);

        //////////////////////////////////////////////////////////////////////////
        // Decompression section.
        //////////////////////////////////////////////////////////////////////////

        case ICM_DECOMPRESS:
            return codec->Decompress((ICDECOMPRESS*)lParam1, (DWORD)lParam2);

        case ICM_DECOMPRESS_QUERY:
            return codec->DecompressQuery((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_DECOMPRESS_GET_FORMAT:
            return codec->DecompressGetFormat((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_DECOMPRESS_BEGIN:
            return codec->DecompressBegin((BITMAPINFOHEADER*)lParam1, (BITMAPINFOHEADER*)lParam2);

        case ICM_DECOMPRESS_END:
            return codec->DecompressEnd((int)lParam1, (int)lParam2);
    }

    if (uMsg < DRV_USER)
        return DefDriverProc(dwDriverId, hDriver, uMsg, lParam1, lParam2);

    return ICERR_UNSUPPORTED;
}
