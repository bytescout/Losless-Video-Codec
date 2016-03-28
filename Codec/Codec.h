//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#pragma once

struct CodecCoreSettings
{
    CodecCoreSettings()
    {
        wSize = sizeof(CodecCoreSettings);
    }

    WORD wSize;
};

class Codec : public CodecCoreSettings
{
public:
    Codec();
    ~Codec();

    // Config functions
    void About(HWND hParentWindow);
    void Configure(HWND hParentWindow);
    void SetDefaults();

    // Compress functions
    int Compress(ICCOMPRESS* lParam1, DWORD lParam2);
    int CompressQuery(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int CompressGetFormat(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int CompressGetSize(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int CompressFramesInfo(ICCOMPRESSFRAMES* lParam1);
    int CompressBegin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int CompressEnd(int lParam1, int lParam2);

    // Decompress functions
    int Decompress(ICDECOMPRESS* lParam1, DWORD lParam2);
    int DecompressQuery(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int DecompressGetFormat(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int DecompressBegin(BITMAPINFOHEADER* input, BITMAPINFOHEADER* output);
    int DecompressEnd(int lParam1, int lParam2);

private:
    // compressed size of a frame (as reported to user).
    // NOTE: usually this value is incorrect (just bigger then required)   
    int m_reportedCompressedSize;
    int m_processedFrames;

    byte* m_conversionBuffer;
    int m_bufferSize;

    byte* m_keyFrameBuffer;
    byte* m_deltaFrameBuffer;
    int m_frameBufferSize;

    int m_inputStrideLength;
    int m_outputStrideLength;

private:
    int compressFrame(ICCOMPRESS* lParam1, byte* frameData);
    int decompressFrame(ICDECOMPRESS* lParam1, byte* frameBuffer);

    void convert24to32(ICDECOMPRESS* lParam1, byte* frameBuffer);
    void convert32to24(ICDECOMPRESS* lParam1, byte* frameBuffer);

    void createDeltaFrame(byte* currentFrameData);
    void produceFullFrame(byte* currentFrameData);

    static INT_PTR CALLBACK AboutDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK ConfigureDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};
