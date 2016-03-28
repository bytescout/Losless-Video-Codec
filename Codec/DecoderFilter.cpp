//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#include "stdafx.h"
#include "DecoderFilter.h"
#include "Codec.h"

#define MyCheckPointer(p, ret)  { if ((p) == NULL){ /*m_lastError = g_sInvalidPointer;*/ return (ret); } }

CUnknown* WINAPI CDecoderFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT* phr)
{
    CDecoderFilter* pNewFilter = new CDecoderFilter(lpunk, phr);

    if (phr)
    {
        if (pNewFilter == NULL)
            *phr = E_OUTOFMEMORY;
        else
            *phr = S_OK;
    }

    return pNewFilter;
}

CDecoderFilter::CDecoderFilter(LPUNKNOWN lpunk, HRESULT* phr) : 
    CTransformFilter(g_sDecoderShortName, lpunk, CLSID_BytescoutLosslessVideoDecoder)
{
    ASSERT(phr);
    m_codec = new Codec();
}

CDecoderFilter::~CDecoderFilter()
{
    m_codec->DecompressEnd(0, 0);
    delete m_codec;
}

STDMETHODIMP CDecoderFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv)
{
    MyCheckPointer(ppv, E_POINTER);

    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDecoderFilter::CheckInputType(const CMediaType* mtInput)
{
    MyCheckPointer(mtInput, E_POINTER);

    if (mtInput->majortype != MEDIATYPE_Video)
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (mtInput->subtype != MEDIASUBTYPE_BLVC)
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (mtInput->formattype != FORMAT_VideoInfo || mtInput->cbFormat < sizeof(VIDEOINFOHEADER))
        return VFW_E_TYPE_NOT_ACCEPTED;

    VIDEOINFOHEADER* pviInput = reinterpret_cast<VIDEOINFOHEADER*>(mtInput->Format());
    MyCheckPointer(pviInput, E_UNEXPECTED);

    if (pviInput->bmiHeader.biBitCount != 32 && pviInput->bmiHeader.biBitCount != 24)
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (m_codec->DecompressQuery(&pviInput->bmiHeader, NULL) != ICERR_OK)
        return VFW_E_TYPE_NOT_ACCEPTED;

    return S_OK;
}

HRESULT CDecoderFilter::GetMediaType(int iPosition, CMediaType* pMediaType)
{
    if (!m_pInput->IsConnected())
        return E_UNEXPECTED;

    if (iPosition < 0)
        return E_INVALIDARG;

    if (iPosition > 2)
        return VFW_S_NO_MORE_ITEMS;

    MyCheckPointer(pMediaType, E_POINTER);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    pMediaType->InitMediaType();

    VIDEOINFO* pviOutput = (VIDEOINFO*)pMediaType->AllocFormatBuffer(sizeof(VIDEOINFO));
    MyCheckPointer(pviOutput, E_UNEXPECTED);
    ZeroMemory(pviOutput, sizeof(VIDEOINFO));

    pviOutput->bmiHeader.biCompression = BI_RGB;

    if (iPosition == 0)
        pviOutput->bmiHeader.biBitCount = pviInput->bmiHeader.biBitCount;
    else if (iPosition == 1)
        pviOutput->bmiHeader.biBitCount = 24;
    else
        pviOutput->bmiHeader.biBitCount = 32;

    pviOutput->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pviOutput->bmiHeader.biWidth = pviInput->bmiHeader.biWidth;
    pviOutput->bmiHeader.biHeight = pviInput->bmiHeader.biHeight;
    pviOutput->bmiHeader.biPlanes = 1;
    pviOutput->bmiHeader.biSizeImage = GetBitmapSize(&(pviOutput->bmiHeader));
    pviOutput->bmiHeader.biXPelsPerMeter = pviInput->bmiHeader.biXPelsPerMeter;
    pviOutput->bmiHeader.biYPelsPerMeter = pviInput->bmiHeader.biYPelsPerMeter;

    pviOutput->AvgTimePerFrame = pviInput->AvgTimePerFrame;

    pMediaType->SetType(&MEDIATYPE_Video);
    GUID subTypeGUID = GetBitmapSubtype(&(pviOutput->bmiHeader));
    pMediaType->SetSubtype(&subTypeGUID);
    pMediaType->SetFormatType(&FORMAT_VideoInfo);
    pMediaType->SetTemporalCompression(FALSE);
    pMediaType->SetSampleSize(pviOutput->bmiHeader.biSizeImage);

    return S_OK;
}

HRESULT CDecoderFilter::CheckTransform(const CMediaType* mtInput, const CMediaType* mtOutput)
{
    MyCheckPointer(mtInput, E_POINTER);
    MyCheckPointer(mtOutput, E_POINTER);

    HRESULT hr = CheckInputType(mtInput);
    if (hr != S_OK)
        return hr;

    if (mtOutput->majortype != MEDIATYPE_Video || 
        (mtOutput->subtype != MEDIASUBTYPE_RGB24 && mtOutput->subtype != MEDIASUBTYPE_RGB32))
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (mtOutput->formattype != FORMAT_VideoInfo || mtOutput->cbFormat < sizeof(VIDEOINFO))
        return VFW_E_TYPE_NOT_ACCEPTED;

    ASSERT(mtInput->formattype == FORMAT_VideoInfo && mtInput->cbFormat >= sizeof(VIDEOINFOHEADER));

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)mtInput->Format();
    VIDEOINFO* pviOutput = (VIDEOINFO*)mtOutput->Format();

    if (pviOutput->bmiHeader.biPlanes != 1 || 
        (pviOutput->bmiHeader.biBitCount != 24 && pviOutput->bmiHeader.biBitCount != 32) ||
        pviOutput->bmiHeader.biCompression != BI_RGB || 
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

HRESULT CDecoderFilter::DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProp)
{
    if (!m_pInput->IsConnected() || !m_pOutput->IsConnected())
        return E_UNEXPECTED;

    ASSERT(m_pOutput->CurrentMediaType().formattype == FORMAT_VideoInfo);

    VIDEOINFOHEADER* pviOutput = (VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format();
    MyCheckPointer(pviOutput, E_UNEXPECTED);

    pProp->cBuffers = 1;
    pProp->cbBuffer = GetBitmapSize(&(pviOutput->bmiHeader));
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

HRESULT CDecoderFilter::Transform(IMediaSample* pInput, IMediaSample* pOutput)
{
    MyCheckPointer(pInput, E_POINTER);
    MyCheckPointer(pOutput, E_POINTER);

    VIDEOINFOHEADER* pviOutput = (VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format();
    MyCheckPointer(pviOutput, E_UNEXPECTED);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    ICDECOMPRESS decomp;
    ZeroMemory(&decomp, sizeof(ICDECOMPRESS));

    decomp.dwFlags = ICDECOMPRESS_NOTKEYFRAME;
    if (pInput->IsSyncPoint() == S_OK)
        decomp.dwFlags = 0;

    decomp.lpbiOutput = &pviOutput->bmiHeader;
    pOutput->GetPointer((BYTE**)&decomp.lpOutput);

    BITMAPINFOHEADER inputBmi;
    memcpy(&inputBmi, &pviInput->bmiHeader, sizeof(BITMAPINFOHEADER));
    inputBmi.biSizeImage = pInput->GetActualDataLength();
    decomp.lpbiInput = &inputBmi;
    pInput->GetPointer((BYTE**)&decomp.lpInput);

    int res = m_codec->Decompress(&decomp, 0);
    if (res != ICERR_OK)
        return S_FALSE;

    return S_OK;
}

HRESULT CDecoderFilter::StartStreaming()
{
    VIDEOINFOHEADER* pviOutput = (VIDEOINFOHEADER*)m_pOutput->CurrentMediaType().Format();
    MyCheckPointer(pviOutput, E_UNEXPECTED);

    VIDEOINFOHEADER* pviInput = (VIDEOINFOHEADER*)m_pInput->CurrentMediaType().Format();
    MyCheckPointer(pviInput, E_UNEXPECTED);

    m_codec->DecompressBegin(&pviInput->bmiHeader, &pviOutput->bmiHeader);
    return CTransformFilter::StartStreaming();
}

HRESULT CDecoderFilter::StopStreaming()
{
    m_codec->DecompressEnd(0, 0);
    return CTransformFilter::StopStreaming();
}
