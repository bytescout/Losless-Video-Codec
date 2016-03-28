//*******************************************************************
//       ByteScout Lossless Video Codec		                                     
//                                                                   
//       Copyright © 2016 ByteScout - http://www.bytescout.com       
//       ALL RIGHTS RESERVED                                         
//                                                                   
//*******************************************************************

#pragma once

#define MAKE_FOURCC(x)      mmioFOURCC(x[0], x[1], x[2], x[3])

// FOURCC of out codec
#define CODEC_FOURCC		MAKE_FOURCC("BLVC") // Bytescout Lossless Video Codec

#define CODEC_NAME          L"Bytescout Lossless Video Codec"
#define CODEC_DESCRIPTION   L"Bytescout Lossless Video Codec"

#define g_sEncoderLongName         L"Bytescout Lossless Video Encoder"
#define g_sEncoderShortName        L"BytescoutLosslessVideoEncoder"

#define g_sDecoderLongName         L"Bytescout Lossless Video Decoder"
#define g_sDecoderShortName        L"BytescoutLosslessVideoDecoder"

// {CF909FF0-7A29-4f8f-9CBF-887442988FE2}
DEFINE_GUID(CLSID_BytescoutLosslessVideoEncoder, 0xcf909ff0, 0x7a29, 0x4f8f, 0x9c, 0xbf, 0x88, 0x74, 0x42, 0x98, 0x8f, 0xe2);

// {7312972D-37C5-44e3-A374-26BD90298C73}
DEFINE_GUID(CLSID_BytescoutLosslessVideoDecoder, 0x7312972d, 0x37c5, 0x44e3, 0xa3, 0x74, 0x26, 0xbd, 0x90, 0x29, 0x8c, 0x73);

const static GUID MEDIASUBTYPE_BLVC = (GUID)FOURCCMap(CODEC_FOURCC);
