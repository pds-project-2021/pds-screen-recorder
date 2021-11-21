//
// Created by gabriele on 31/10/21.
//

#include "include/Format.h"

/* Private methods */

void Format::source_audio_context() {
	auto audioFormat = get_audio_input_format();
	auto audio = av_find_input_format(audioFormat.c_str());
	input.set_audio(audio);

//	auto options = get_audio_options();
	auto audioInputDevice = get_audio_input_device();
	auto audioCtx = avformat_alloc_context();
	auto ret = avformat_open_input(&audioCtx, audioInputDevice.c_str(), audio, nullptr);
	if (ret != 0) {
		throw avException("Couldn't open audio input stream");
	}
	inputContext.set_audio(audioCtx);
//	inputOptions.set_audio(options);

	ret = avformat_find_stream_info(audioCtx, nullptr);
	if (ret < 0) {
		throw avException("Unable to find the audio stream information");
	}
}

void Format::source_video_context() {
	auto videoFormat = get_video_input_format();
	auto video = av_find_input_format(videoFormat.c_str());
	input.set_video(video);

	auto options = get_video_options();
	auto videoDevice = get_video_input_device();
	auto videoCtx = avformat_alloc_context();
	auto ret = avformat_open_input(&videoCtx, videoDevice.c_str(), video, &options);
	if (ret != 0) {
		throw avException("Couldn't open video input stream");
	}
	inputContext.set_video(videoCtx);
	inputOptions.set_video(options);

	ret = avformat_find_stream_info(videoCtx, nullptr);
	if (ret < 0) {
		throw avException("Unable to find the video stream information");
	}
}

void Format::destination_audio_context(const string &dest) {
	auto ctx = outputContext.get_audio();
	avformat_alloc_output_context2(&ctx, nullptr, nullptr, dest.c_str());
	if (!ctx) {
		throw avException("Error in allocating av format output context");
	}
	outputContext.set_audio(ctx); // todo vedere se si può togliere

	auto fmt = av_guess_format(nullptr, dest.c_str(), nullptr);
	if(!fmt){
		throw avException(
			"Error in guessing the audio format. try with correct format");
	}
	output.set_audio(fmt);

}

void Format::destination_video_context(const string& dest) {
	auto ctx = outputContext.get_video();
	avformat_alloc_output_context2(&ctx, nullptr, nullptr, dest.c_str());
	if (!ctx) {
		throw avException("Error in allocating av format output context");
	}
	outputContext.set_video(ctx); // todo vedere se si può togliere

	auto fmt = av_guess_format(nullptr, dest.c_str(), nullptr);
	if(!fmt){
		throw avException(
			"Error in guessing the video format. try with correct format");
	}
	output.set_video(fmt);
}

void Format::find_source_audio_stream_info() {
	auto audio = inputContext.get_audio();
	audioStreamIndex = av_find_best_stream(audio,AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (audioStreamIndex == -1) {
		throw avException("Unable to find the audio stream index. (-1)");
	}
}

void Format::find_source_video_stream_info() {
	auto video = inputContext.get_video();
	videoStreamIndex = av_find_best_stream(video, AVMEDIA_TYPE_VIDEO, -1,-1, nullptr, 0);
	if (videoStreamIndex == -1) {
		throw avException("Unable to find the video stream index. (-1)");
	}
}

/* Public methods */
void Format::setup_source() {
	source_audio_context();
	source_video_context();

	find_source_audio_stream_info();
	find_source_video_stream_info();
}

void Format::setup_destination(const string& dest) {
	destination_audio_context(dest);
	destination_video_context(dest);
}

AVCodecParameters* Format::get_source_audio_codec() {
	auto audio = inputContext.get_audio();
	return audio->streams[audioStreamIndex]->codecpar;
}

AVCodecParameters* Format::get_source_video_codec() {
	auto video = inputContext.get_video();
	return video->streams[videoStreamIndex]->codecpar;
}

