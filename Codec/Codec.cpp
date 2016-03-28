//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#include "stdafx.h"
#include "Codec.h"
#include "zlib.h"
#include "resource.h"
#include "version_number.h"

#ifndef _T
#define _T(x)      L ## x
#endif

extern HINSTANCE g_hInst;

Codec::Codec()
{
    m_processedFrames = 0;
    m_conversionBuffer = NULL;
    m_keyFrameBuffer = NULL;
    m_deltaFrameBuffer = NULL;
    SetDefaults();
}

Codec::~Codec()
{
}

//////////////////////////////////////////////////////////////////////////
// Config functions
//////////////////////////////////////////////////////////////////////////

void Codec::About(HWND hParentWindow)
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUT), hParentWindow, AboutDialogProc);  
}

INT_PTR CALLBACK Codec::AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    if (uMsg == WM_INITDIALOG)
    {
        WCHAR s[MAX_PATH];
        wsprintf(s, L"%s%s", L"Bytescout Lossless Codec ", T_VERSION_STRING);
        SetDlgItemText(hwndDlg, IDC_NAME, s);
    }
    else if (uMsg == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
            case IDOK:
                EndDialog(hwndDlg, 0);
                break;
        }
    }

    return FALSE;
}

void Codec::Configure(HWND hParentWindow)
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_CONFIGURE), hParentWindow, ConfigureDialogProc);
}

INT_PTR CALLBACK Codec::ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    if (uMsg == WM_INITDIALOG)
    {
        WCHAR s[MAX_PATH];
        wsprintf(s, L"%s%s", L"Bytescout Lossless Codec ", T_VERSION_STRING);
        SetDlgItemText(hwndDlg, IDC_CONFIGURE_NAME, s);
    }
    else if (uMsg == WM_COMMAND)
    {
        switch (LOWORD(wParam))
        {
            case IDOK:
            case IDCANCEL:
                EndDialog(hwndDlg, 0);
                break;
        }
    }

    return FALSE;
}

void Codec::SetDefaults()
{
    m_reportedCompressedSize = 1;
}

//////////////////////////////////////////////////////////////////////////
// Compress functions
//////////////////////////////////////////////////////////////////////////

int Codec::Compress(ICCOMPRESS* lParam1, DWORD /*lParam2*/)
{
    bool isKeyFrame = false;
    if (*lParam1->lpdwFlags == AVIIF_KEYFRAME || m_processedFrames == 0 || (m_processedFrames % 10) == 0)
        isKeyFrame = true;

    byte* frameData = NULL;

    if (isKeyFrame)
    {
        memcpy(m_keyFrameBuffer, lParam1->lpInput, m_frameBufferSize);
        frameData = m_keyFrameBuffer;
    }
    else
    {
        createDeltaFrame((byte*)lParam1->lpInput);
        frameData = m_deltaFrameBuffer;
    }

    int res = compressFrame(lParam1, frameData);
    if (res == ICERR_OK && !isKeyFrame)
        *lParam1->lpdwFlags &= ~AVIIF_KEYFRAME;

    return res;
}

int Codec::CompressQuery(BITMAPINFOHEADER* input, BITMAPINFOHEADER* /*output*/)
{
    if (input->biCompression == BI_RGB && (input->biBitCount == 24 || input->biBitCount == 32)) 
        return ICERR_OK;

    return ICERR_BADFORMAT;
}

int Codec::CompressGetFormat(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
    if (output == NULL)
        return sizeof(BITMAPINFOHEADER) + sizeof(CodecCoreSettings);

    output->biSize = sizeof(BITMAPINFOHEADER) + sizeof(CodecCoreSettings);
    output->biWidth = input->biWidth;
    output->biHeight = input->biHeight;
    output->biCompression = CODEC_FOURCC;
    output->biPlanes = input->biPlanes;
    output->biBitCount = input->biBitCount;

    CodecCoreSettings* codecCore = (CodecCoreSettings*)(((byte*)output) + sizeof(BITMAPINFOHEADER));
    codecCore->wSize = sizeof(CodecCoreSettings);
    return ICERR_OK;
}

int Codec::CompressGetSize(BITMAPINFOHEADER* input, BITMAPINFOHEADER* /*output*/)
{
    // pretend that we need 150% of uncompressed size
    m_inputStrideLength = (input->biWidth * input->biBitCount + 31) / 32 * 4;
    m_reportedCompressedSize = (int)(abs(input->biHeight * m_inputStrideLength) * 1.5);
    return m_reportedCompressedSize;
}

int Codec::CompressFramesInfo(ICCOMPRESSFRAMES* /*lParam1*/)
{
    return ICERR_OK;
}

int Codec::CompressBegin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* /*output*/)
{
    m_processedFrames = 0;

    int strideLength = (input->biWidth * input->biBitCount + 31) / 32 * 4;
    m_frameBufferSize = abs(input->biHeight * strideLength);
    m_keyFrameBuffer = new byte[m_frameBufferSize];
    m_deltaFrameBuffer = new byte[m_frameBufferSize];
    return ICERR_OK;
}

int Codec::CompressEnd(int /*lParam1*/, int /*lParam2*/)
{
    delete[] m_keyFrameBuffer;
    m_keyFrameBuffer = NULL;

    delete m_deltaFrameBuffer;
    m_deltaFrameBuffer = NULL;

    return ICERR_OK;
}

//////////////////////////////////////////////////////////////////////////
// Decompress functions
//////////////////////////////////////////////////////////////////////////

int Codec::Decompress(ICDECOMPRESS* lParam1, DWORD /*lParam2*/)
{
    int res = ICERR_OK;

    bool isKeyFrame = !(lParam1->dwFlags == ICDECOMPRESS_NOTKEYFRAME);
    
    if (isKeyFrame)
        res = decompressFrame(lParam1, m_keyFrameBuffer);
    else
        res = decompressFrame(lParam1, m_deltaFrameBuffer);

    if (res == ICERR_OK)
    {
        if (isKeyFrame)
            memcpy(lParam1->lpOutput, m_keyFrameBuffer, m_frameBufferSize);
        else
            produceFullFrame((byte*)lParam1->lpOutput);
    }

    return res;
}

int Codec::DecompressQuery(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
    if (input->biCompression != CODEC_FOURCC)
        return ICERR_BADFORMAT;

    if (output != NULL)
    {
        if (output->biCompression != BI_RGB)
        {
            // sorry, only RGB output
            return ICERR_BADFORMAT;
        }

        if (output->biBitCount != 24 && output->biBitCount != 32)
        {
            // unsupported bit depth
            return ICERR_BADFORMAT;
        }
    }

    return ICERR_OK;
}

int Codec::DecompressGetFormat(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
    if (input->biCompression != CODEC_FOURCC)
        return ICERR_BADFORMAT;

    if (output != NULL)
    {
        output->biSize = input->biSize;
        output->biWidth = input->biWidth;
        output->biHeight = input->biHeight;
        output->biPlanes = input->biPlanes;
        output->biBitCount = input->biBitCount;
        output->biSizeImage = input->biSizeImage;
        output->biCompression = BI_RGB;
    }
    
    return ICERR_OK;
}

int Codec::DecompressBegin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output)
{
    if (output->biCompression != BI_RGB)
        return ICERR_BADFORMAT;
        
    m_inputStrideLength = (input->biWidth * input->biBitCount + 31) / 32 * 4;
    m_outputStrideLength = (output->biWidth * output->biBitCount + 31) / 32 * 4;

    if (input->biBitCount != output->biBitCount)
    {
        // we create temporary buffer here.
        // this buffer equal to full uncompressed frame of input image.
        // this buffer will be used for decompression of input data
        // real output buffer will be constructed via this buffer conversion
        m_bufferSize = abs(input->biHeight * m_inputStrideLength);
        m_conversionBuffer = new byte[m_bufferSize];
    }

    m_frameBufferSize = abs(output->biHeight * m_outputStrideLength);
    m_keyFrameBuffer = new byte[m_frameBufferSize];
    m_deltaFrameBuffer = new byte[m_frameBufferSize];

    return ICERR_OK;
}

int Codec::DecompressEnd(int /*lParam1*/, int /*lParam2*/)
{
    delete[] m_conversionBuffer;
    m_conversionBuffer = NULL;

    delete[] m_keyFrameBuffer;
    m_keyFrameBuffer = NULL;

    delete[] m_deltaFrameBuffer;
    m_deltaFrameBuffer = NULL;

    return ICERR_OK;
}

//////////////////////////////////////////////////////////////////////////

int Codec::compressFrame(ICCOMPRESS* lParam1, byte* frameData)
{
    DWORD lActual = lParam1->lpbiInput->biSizeImage;
    m_processedFrames++;

    uLongf compressedSize = 0;
    if (lParam1->lpOutput != NULL)
    {
        compressedSize = m_reportedCompressedSize;
        int res = compress2((Bytef*)lParam1->lpOutput, &compressedSize, (Bytef*)frameData, lActual, 1);

        if (res == Z_MEM_ERROR)
            return ICERR_MEMORY;
        else if (res == Z_BUF_ERROR)
            return ICERR_BADSIZE;
    }

    lParam1->lpbiOutput->biSizeImage = compressedSize;
    *lParam1->lpdwFlags = AVIIF_KEYFRAME;
    return ICERR_OK;
}

int Codec::decompressFrame(ICDECOMPRESS* lParam1, byte* frameBuffer)
{
    DWORD lActual = lParam1->lpbiInput->biSizeImage;

    int res = Z_OK;
    if (lParam1->lpbiInput->biBitCount == lParam1->lpbiOutput->biBitCount)
    {
        // input and output image have the same bit count value
        // so just uncompress input image into output buffer
        res = uncompress(
            (Bytef*)frameBuffer, (uLongf*)&lParam1->lpbiOutput->biSizeImage,
            (Bytef*)lParam1->lpInput, lActual);
    }
    else
    {
        // input and output image have different bit count value
        // we need to uncompress input image into temporary buffer
        // and convert values from this buffer into output buffer
        uLongf size = m_bufferSize;
        res = uncompress(
            (Bytef*)m_conversionBuffer, &size,
            (Bytef*)lParam1->lpInput, lActual);

        assert((int)size == m_bufferSize);

        if (lParam1->lpbiInput->biBitCount == 24 && 
            lParam1->lpbiOutput->biBitCount == 32)
        {
            convert24to32(lParam1, frameBuffer);
        }
        else if (lParam1->lpbiInput->biBitCount == 32 && 
            lParam1->lpbiOutput->biBitCount == 24)
        {
            convert32to24(lParam1, frameBuffer);
        }
    }

    if (res == Z_MEM_ERROR)
        return ICERR_MEMORY;
    else if (res == Z_BUF_ERROR)
        return ICERR_BADSIZE;
    else if (res == Z_DATA_ERROR)
        return ICERR_BADFORMAT;

    return ICERR_OK;
}

void Codec::convert24to32(ICDECOMPRESS* lParam1, byte* frameBuffer)
{
    // note, that 32bit image should be made upside-down
    // relative to 24bit image if output height is negative

    int height = lParam1->lpbiOutput->biHeight;
    bool upsideDown = false;
    if (height < 0)
    {
        height = -height;
        upsideDown = true;
    }

    lParam1->lpbiOutput->biSizeImage = height * m_outputStrideLength;

    for (int line = 0; line < height; line++)
    {
        byte* outputBuffer = NULL;
        if (upsideDown)
            outputBuffer = frameBuffer + (height - line - 1) * m_outputStrideLength;
        else
            outputBuffer = frameBuffer + line * m_outputStrideLength;

        byte* inputBuffer = m_conversionBuffer + line * m_inputStrideLength;
        byte* stop = outputBuffer + m_outputStrideLength;
        while (outputBuffer != stop)
        {
            *outputBuffer++ = *inputBuffer++;
            *outputBuffer++ = *inputBuffer++;
            *outputBuffer++ = *inputBuffer++;
            *outputBuffer++ = 0;
        }
    }
}

void Codec::convert32to24(ICDECOMPRESS* lParam1, byte* frameBuffer)
{
    int height = abs(lParam1->lpbiOutput->biHeight);
    lParam1->lpbiOutput->biSizeImage = height * m_outputStrideLength;

    for (int line = 0; line < height; line++)
    {
        byte* outputBuffer = frameBuffer + line * m_outputStrideLength;
        byte* inputBuffer = m_conversionBuffer + line * m_inputStrideLength;
        for (int pos = 0; pos < lParam1->lpbiInput->biWidth; pos++)
        {
            *outputBuffer++ = *inputBuffer++;
            *outputBuffer++ = *inputBuffer++;
            *outputBuffer++ = *inputBuffer++;
            inputBuffer++;
        }
    }
}

void Codec::createDeltaFrame(byte* currentFrameData)
{
    byte* key = m_keyFrameBuffer;
    byte* delta = m_deltaFrameBuffer;
    byte* current = currentFrameData;
    byte* stop = current + m_frameBufferSize;

    while (current != stop)
    {
        *delta++ = *key++ ^ *current++;
    }
}

void Codec::produceFullFrame(byte* currentFrameData)
{
    byte* key = m_keyFrameBuffer;
    byte* delta = m_deltaFrameBuffer;
    byte* current = currentFrameData;
    byte* stop = current + m_frameBufferSize;

    while (current != stop)
    {
         *current++ = *key++ ^ *delta++;
    }
}
