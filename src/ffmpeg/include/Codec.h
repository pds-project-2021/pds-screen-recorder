//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "wrapper.h"
#include "exceptions.h"
#include "Format.h"
#include "platform.h"


class Codec {
  public:
//	AVCodec *inputCodec = nullptr;
//	AVCodec *audioInputCodec = nullptr;

	wrapper<AVCodec> input;

//	AVCodec *outputCodec = nullptr;
//	AVCodec *audioOutputCodec = nullptr;

	wrapper<AVCodec> output;


//	AVCodecContext *inputCodecContext = nullptr;
//	AVCodecContext *audioInputCodecContext = nullptr;

	wrapper<AVCodecContext>  inputContext;

//	AVCodecContext *outputCodecContext = nullptr;
//	AVCodecContext *audioOutputCodecContext = nullptr;

	wrapper<AVCodecContext> outputContext;

//	AVCodecParameters *inputCodecPar = nullptr;
//	AVCodecParameters *audioInputCodecPar = nullptr;

	wrapper<AVCodecParameters> inputPar;

//	AVCodecParameters *outputCodecPar = nullptr;
//	AVCodecParameters *audioOutputCodecPar = nullptr;

	wrapper<AVCodecParameters> outputPar;

	void init_input(Format &format);
};



