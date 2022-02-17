
#include "Format.h"

/* Private methods */

void Format::source_audio_context() {
	auto audioFormat = get_audio_input_format();
	auto audio = av_find_input_format(audioFormat.c_str());
	input.set_audio(audio);

	auto options = get_audio_options();
	set_audio_parameters(options);

	audioDevice = get_audio_input_device();
	auto audioCtx = avformat_alloc_context();
	auto ret = avformat_open_input(&audioCtx, audioDevice.c_str(), audio, &options);
	if (ret != 0) {
		throw avException("Couldn't open audio input stream");
	}
	inputContext.set_audio(audioCtx);
	inputOptions.set_audio(options);

	ret = avformat_find_stream_info(audioCtx, nullptr);
	if (ret < 0) {
		throw avException("Unable to find the audio stream information");
	}
}

void Format::source_video_context() {
	auto videoFormat = get_video_input_format();
	auto video = av_find_input_format(videoFormat.c_str());
	input.set_video(video);

	AVDictionary *options = get_video_options();
	set_screen_parameters(options);

	videoDevice = get_video_input_device(screen.get_offset_str());
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

void Format::set_screen_parameters(AVDictionary *options) const {
	if (screen.get_width() != "0" && screen.get_height() != "0") {
		av_dict_set(&options, "offset_x", screen.get_offset_x().c_str(), 0);
		av_dict_set(&options, "offset_y", screen.get_offset_y().c_str(), 0);
		av_dict_set(&options, "video_size", screen.get_video_size().c_str(), 0);
	}

	av_dict_set(&options, "show_region", screen.get_show_region().c_str(), 0);
}

void Format::set_audio_parameters(AVDictionary *options) const {
	av_dict_set(&options, "channels", std::to_string(channels).c_str(), 0);
}

void Format::destination_context(const std::string &dest) {
	auto ctx = outputContext.get_video();
	avformat_alloc_output_context2(&ctx, nullptr, nullptr, dest.c_str());
	if (!ctx) {
		throw avException("Error in allocating av format output context");
	}

	/* Some container formats (like MP4) require global headers to be present
        Mark the encoder so that it behaves accordingly. */
	if (ctx->oformat->flags & AVFMT_GLOBALHEADER) {
		ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	}

	outputContext.set_video(ctx);

	auto fmt = av_guess_format(nullptr, dest.c_str(), nullptr);
	if (!fmt) {
		throw avException("Error in guessing the video format. try with correct format");
	}
	output.set_video(fmt);
}

void Format::find_source_audio_stream_info() {
	auto audio = inputContext.get_audio();
	audioStreamIndex = av_find_best_stream(audio, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
	if (audioStreamIndex == -1) {
		throw avException("Unable to find the audio stream index. (-1)");
	}
}

void Format::find_source_video_stream_info() {
	auto video = inputContext.get_video();
	videoStreamIndex = av_find_best_stream(video, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (videoStreamIndex == -1) {
		throw avException("Unable to find the video stream index. (-1)");
	}
}

/* Public methods */

void Format::set_screen_params(const Screen &params) {
	screen = params;
}

void Format::set_audio_layout(enum AudioLayout layout) {
	channels = layout;
}

void Format::setup_source() {
	source_audio_context();
	source_video_context();

	find_source_audio_stream_info();
	find_source_video_stream_info();
}

void Format::setup_destination(const std::string &dest) {
	destination_context(dest);
}

std::string Format::get_audio_device() const {
	return audioDevice;
}

std::string Format::get_video_device() const {
	return videoDevice;
}

AVCodecParameters *Format::get_source_audio_codec() const {
	auto audio = inputContext.get_audio();
	return audio->streams[audioStreamIndex]->codecpar;
}

AVCodecParameters *Format::get_source_video_codec() const {
	auto video = inputContext.get_video();
	return video->streams[videoStreamIndex]->codecpar;
}

void Format::write_header() const {
	/* imp: mp4 container or some advanced container file required header
	 * information */
	auto opt = outputOptions.get_video();
	auto ret = avformat_write_header(outputContext.get_video(), &opt);
	if (ret < 0) {
		throw avException("Error in writing the header context");
	}
}

void Format::reset() {
	inputContext.set_audio(nullptr);
	inputContext.set_video(nullptr);
    outputContext.set_audio(nullptr);
	outputContext.set_video(nullptr);
    input.set_audio(nullptr);
    input.set_video(nullptr);
    output.set_audio(nullptr);
    output.set_video(nullptr);
}
