//
// Created by gabriele on 31/10/21.
//

/**
 * Class wrapper for Codec and Context
 */

#pragma once

#include "wrapper.h"
#include "exceptions.h"
#include "platform.h"

class Format {
  public:
	wrapper<AVInputFormat> input;
//    AVInputFormat *inputFormat = nullptr;
//	AVInputFormat *audioInputFormat = nullptr;

	wrapper<AVOutputFormat> output;

//	AVOutputFormat *outputFormat = nullptr;
//	AVOutputFormat *audioOutputFormat = nullptr;

	wrapper<AVFormatContext> inputContext;
	wrapper<AVFormatContext> outputContext;

//	AVFormatContext *inputFormatContext = nullptr;
//	AVFormatContext *audioInputFormatContext = nullptr;
//
//	AVFormatContext *outputFormatContext = nullptr;
//	AVFormatContext *audioOutputFormatContext = nullptr;

	int videoStreamIndex = 0;
	int audioStreamIndex = 0;
  public:
	Format() = default;
	~Format() = default;

	void init_input();
	void init_output();
};



