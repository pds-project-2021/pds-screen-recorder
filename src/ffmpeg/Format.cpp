//
// Created by gabriele on 31/10/21.
//

#include "include/Format.h"

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

	ret = avformat_find_stream_info(*videoCtx, &options);
	if (ret < 0) {
		throw avException("Unable to find the video stream information");
	}

	ret = avformat_find_stream_info(*audioCtx, nullptr);
	if (ret < 0) {
		throw avException("Unable to find the audio stream information");
	}

	videoStreamIndex = av_find_best_stream(*videoCtx, AVMEDIA_TYPE_VIDEO, -1,-1, nullptr, 0);
	if (videoStreamIndex == -1) {
		throw avException("Unable to find the video stream index. (-1)");
	}

	audioStreamIndex = av_find_best_stream(*audioCtx,AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (audioStreamIndex == -1) {
		throw avException("Unable to find the audio stream index. (-1)");
	}
}

void Format::init_output() {

}
