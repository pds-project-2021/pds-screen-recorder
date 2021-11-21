//
// Created by gabriele on 31/10/21.
//

#pragma once

#include "wrapper.h"
#include "exceptions.h"
#include "platform.h"


class Codec {
  public:
	wrapper<AVCodec> input;
	wrapper<AVCodec> output;

	wrapper<AVCodecContext>  inputContext;
	wrapper<AVCodecContext> outputContext;

	wrapper<AVCodecParameters> inputPar;
	wrapper<AVCodecParameters> outputPar;

  private:
	void source_audio_context();
	void source_video_context();
	void destination_audio_context();
	void destination_video_context();
	void find_audio_encoder(const string& codec_name);
	void find_video_encoder(const string& codec_name);

  public:
	void setup_source();
	void setup_destination();
	void find_encoders(const string& audio_codec, const string& video_codec);
	void set_source_audio_parameters(AVCodecParameters *par);
	void set_source_video_parameters(AVCodecParameters *par);
	void set_destination_audio_parameters(AVCodecParameters *par);
	void set_destination_video_parameters(AVCodecParameters *par);
	void set_time_base(AVRational time_base);
};



