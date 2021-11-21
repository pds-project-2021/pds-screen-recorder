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
#include "Dictionary.h"

class Format {
  public:
	wrapper<AVInputFormat> input;
	wrapper<AVDictionary> inputOptions;

	wrapper<AVOutputFormat> output;
	wrapper<AVDictionary> outputOptions;

	wrapper<AVFormatContext> inputContext;
	wrapper<AVFormatContext> outputContext;

	int videoStreamIndex = 0;
	int audioStreamIndex = 0;
  private:
	void source_audio_context();
	void source_video_context();
	void destination_audio_context(const string &dest);
	void destination_video_context(const string &dest);
	void find_source_audio_stream_info();
	void find_source_video_stream_info();

  public:
	Format() = default;
	~Format() = default;

	void setup_source();
	void setup_destination(const string &dest);

	AVCodecParameters* get_source_audio_codec();
	AVCodecParameters* get_source_video_codec();

	void write_header(const Dictionary& options);

};



