//
// Created by gabriele on 31/10/21.
//

#include "include/Format.h"
#include "platform.h"
#include "exceptions.h"

void Format::init_input() {
	auto audio = av_find_input_format(AUDIO_INPUT_FORMAT);
	input.set_audio(audio);

//	auto options = get_audio_options();
	auto audioCtx = inputContext.get_audio();
	auto ret = avformat_open_input(audioCtx.get(), AUDIO_INPUT_FORMAT_CONTEXT, audio, nullptr);
	if (ret != 0) {
		throw avException("Couldn't open audio input stream");
	}

	auto options = get_video_options();
	auto video = av_find_input_format(VIDEO_INPUT_FORMAT);
	input.set_video(video);

	auto videoCtx = inputContext.get_video();
	ret = avformat_open_input(videoCtx.get(), VIDEO_INPUT_FORMAT_CONTEXT, video, &options);
	if (ret != 0) {
		throw avException("Couldn't open video input stream");
	}
}
void Format::init_output() {
	// todo
}

void Format::print_info() {
	auto video = inputContext.get_video().get();
	av_dump_format(*video, 0 , VIDEO_INPUT_FORMAT_CONTEXT, 0);
	auto audio = inputContext.get_audio();
	av_dump_format(*audio, 0, AUDIO_INPUT_FORMAT_CONTEXT, 0);
}
