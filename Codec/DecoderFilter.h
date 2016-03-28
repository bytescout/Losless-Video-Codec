//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#pragma once

class Codec;

class CDecoderFilter : public CTransformFilter
{
public:
    DECLARE_IUNKNOWN;

    ~CDecoderFilter();

    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);

    // ISpecifyPropertyPages
    //STDMETHODIMP GetPages(CAUUID* pPages);

    // CTransformFilter methods
    HRESULT CheckInputType(const CMediaType *mtInput);
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
    HRESULT CheckTransform(const CMediaType* mtInput, const CMediaType* mtOutput);
    HRESULT DecideBufferSize(IMemAllocator* pAllocator, ALLOCATOR_PROPERTIES* pProp);
    HRESULT Transform(IMediaSample* pInput, IMediaSample* pOutput);

    virtual HRESULT StartStreaming();
    virtual HRESULT StopStreaming();

private:
    Codec* m_codec;

private:
    CDecoderFilter(LPUNKNOWN lpunk, HRESULT* phr);
};
