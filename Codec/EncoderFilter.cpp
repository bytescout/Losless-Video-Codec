//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#include "stdafx.h"
#include "EncoderFilter.h"
#include "Codec.h"

#define MyCheckPointer(p, ret)  { if ((p) == NULL){ /*m_lastError = g_sInvalidPointer;*/ return (ret); } }

CUnknown* WINAPI CEncoderFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CEncoderFilter* pNewFilter = new CEncoderFilter(lpunk, phr);

    if (phr)
    {
        if (pNewFilter == NULL)
            *phr = E_OUTOFMEMORY;
        else
            *phr = S_OK;
    }

    return pNewFilter;
}

CEncoderFilter::CEncoderFilter(LPUNKNOWN lpunk, HRESULT* phr) : 
    CTransformFilter(g_sEncoderShortName, lpunk, CLSID_BytescoutLosslessVideoEncoder)
{
    ASSERT(phr);
    m_codec = new Codec();
}

CEncoderFilter::~CEncoderFilter()
{
    m_codec->CompressEnd(0, 0);
    delete m_codec;
}

STDMETHODIMP CEncoderFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    MyCheckPointer(ppv, E_POINTER);
   
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CEncoderFilter::CheckInputType(const CMediaType* mtInput)
{
    MyCheckPointer(mtInput, E_POINTER);

    if (mtInput->majortype != MEDIATYPE_Video)
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (mtInput->subtype != MEDIASUBTYPE_RGB32 && mtInput->subtype != MEDIASUBTYPE_RGB24)
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (mtInput->formattype != FORMAT_VideoInfo || mtInput->cbFormat < sizeof(VIDEOINFOHEADER))
        return VFW_E_TYPE_NOT_ACCEPTED;

    VIDEOINFOHEADER* pviInput = reinterpret_cast<VIDEOINFOHEADER*>(mtInput->Format());
    MyCheckPointer(pviInput, E_UNEXPECTED);

    if ((mtInput->subtype == MEDIASUBTYPE_RGB32 && pviInput->bmiHeader.biBitCount != 32) ||
        (mtInput->subtype == MEDIASUBTYPE_RGB24 && pviInput->bmiHeader.biBitCount != 24))
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (m_codec->CompressQuery(&pviInput->bmiHeader, NULL) != ICERR_OK)
        return VFW_E_TYPE_NOT_ACCEPTED;

    return S_OK;
}

HRESULT CEncoderFilter::GetMediaType(int iPosition, CMediaType* pMediaType)
{
    if (!m_pInput->IsConnected())
        return E_UNEXPECTED;

    if (iPosition < 0)
        return E_INVALIDARG;

    if (iPosition > 0)
        return VFW_S_NO_MORE_ITEMS;

    MyCheckPointer(pMediaType, E_POINTER);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    pMediaType->InitMediaType();

    VIDEOINFO* pviOutput = (VIDEOINFO*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFO));
    MyCheckPointer(pviOutput, E_UNEXPECTED);
    ZeroMemory(pviOutput, sizeof(VIDEOINFO));

    pviOutput->bmiHeader.biCompression = CODEC_FOURCC;
    pviOutput->bmiHeader.biBitCount = pviInput->bmiHeader.biBitCount;
    pviOutput->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pviOutput->bmiHeader.biWidth = pviInput->bmiHeader.biWidth;
    pviOutput->bmiHeader.biHeight = pviInput->bmiHeader.biHeight;
    pviOutput->bmiHeader.biPlanes = 1;
    pviOutput->bmiHeader.biSizeImage = GetBitmapSize(&(pviOutput->bmiHeader));
    pviOutput->bmiHeader.biXPelsPerMeter = pviInput->bmiHeader.biXPelsPerMeter;
    pviOutput->bmiHeader.biYPelsPerMeter = pviInput->bmiHeader.biYPelsPerMeter;

    pviOutput->AvgTimePerFrame = pviInput->AvgTimePerFrame;

    pMediaType->SetType(&MEDIATYPE_Video);
    pMediaType->SetSubtype(&MEDIASUBTYPE_BLVC);
    pMediaType->SetFormatType(&FORMAT_VideoInfo);
    pMediaType->SetTemporalCompression(TRUE);
    pMediaType->SetVariableSize(); 

    return S_OK;
}

HRESULT CEncoderFilter::CheckTransform(const CMediaType* mtInput, const CMediaType* mtOutput)
{
    MyCheckPointer(mtInput, E_POINTER);
    MyCheckPointer(mtOutput, E_POINTER);

    HRESULT hr = CheckInputType(mtInput);
    if (hr != S_OK)
        return hr;

    if (mtOutput->majortype != MEDIATYPE_Video || mtOutput->subtype != MEDIASUBTYPE_BLVC)
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (mtOutput->formattype != FORMAT_VideoInfo || mtOutput->cbFormat < sizeof(VIDEOINFO))
        return VFW_E_TYPE_NOT_ACCEPTED;

    ASSERT(mtInput->formattype == FORMAT_VideoInfo && mtInput->cbFormat >= sizeof(VIDEOINFOHEADER));

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)mtInput->Format();
    VIDEOINFO* pviOutput = (VIDEOINFO*)mtOutput->Format();

    if (pviOutput->bmiHeader.biPlanes != 1 || 
        pviOutput->bmiHeader.biCompression != CODEC_FOURCC || 
        pviOutput->bmiHeader.biWidth != pviInput->bmiHeader.biWidth || 
        pviOutput->bmiHeader.biHeight != pviInput->bmiHeader.biHeight)
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    RECT rcImage;
    SetRect(&rcImage, 0, 0, pviInput->bmiHeader.biWidth, pviInput->bmiHeader.biHeight);

    if ((!IsRectEmpty(&(pviInput->rcSource)) && !EqualRect(&(pviInput->rcSource), &rcImage)) ||
        (!IsRectEmpty(&(pviOutput->rcTarget)) && !EqualRect(&(pviOutput->rcTarget), &rcImage)))
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (pviOutput->bmiHeader.biClrUsed != 0)
        return VFW_E_TYPE_NOT_ACCEPTED;

    return S_OK;
}

HRESULT CEncoderFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProp)
{
    if (!m_pInput->IsConnected() || !m_pOutput->IsConnected())
        return E_UNEXPECTED;

    ASSERT(m_pOutput->CurrentMediaType().formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER* pviOutput = (VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format();
    MyCheckPointer(pviOutput, E_UNEXPECTED);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    pProp->cBuffers = 1;
    pProp->cbBuffer = m_codec->CompressGetSize(&pviInput->bmiHeader, &pviOutput->bmiHeader);
    pProp->cbAlign = 1;
    pProp->cbPrefix = 0;

    ALLOCATOR_PROPERTIES actProp;
    HRESULT hr = pAllocator->SetProperties(pProp, &actProp);
    if (FAILED(hr))
        return hr;

    if (pProp->cBuffers > actProp.cBuffers || pProp->cbBuffer > actProp.cbBuffer)
        return E_FAIL;

    return S_OK;
}

HRESULT CEncoderFilter::Transform(IMediaSample* pInput, IMediaSample* pOutput)
{
    MyCheckPointer(pInput, E_POINTER);
    MyCheckPointer(pOutput, E_POINTER);

    VIDEOINFOHEADER* pviOutput = (VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format();
    MyCheckPointer(pviOutput, E_UNEXPECTED);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    ICCOMPRESS comp;
    ZeroMemory(&comp, sizeof(ICCOMPRESS));
    
    comp.lpbiOutput = &pviOutput->bmiHeader;
    pOutput->GetPointer((BYTE**)&comp.lpOutput);
    comp.lpbiInput = &pviInput->bmiHeader;
    pInput->GetPointer((BYTE**)&comp.lpInput);
    
    DWORD dwFlags = 0;
    comp.lpdwFlags = &dwFlags;

    int res = m_codec->Compress(&comp, 0);
    if (res != ICERR_OK)
        return S_FALSE;

    if (dwFlags == AVIIF_KEYFRAME)
        pOutput->SetSyncPoint(TRUE);
    else
        pOutput->SetSyncPoint(FALSE);

    pOutput->SetActualDataLength(comp.lpbiOutput->biSizeImage);
    return S_OK;
}

HRESULT CEncoderFilter::StartStreaming()
{
    VIDEOINFOHEADER* pviOutput = (VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format();
    MyCheckPointer(pviOutput, E_UNEXPECTED);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    m_codec->CompressBegin(&pviInput->bmiHeader, &pviOutput->bmiHeader);
    return CTransformFilter::StartStreaming();
}

HRESULT CEncoderFilter::StopStreaming()
{
    m_codec->CompressEnd(0, 0);
    return CTransformFilter::StopStreaming();
}
