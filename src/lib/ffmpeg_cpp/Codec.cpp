#include "Codec.h"

/* Private methods */

void Codec::source_audio_context() {
	auto audio = input.get_audio();
	auto audioCtx = avcodec_alloc_context3(audio);
	if (audioCtx == nullptr) {
		throw avException("Unable to get input audio codec context");
	}
	inputContext.set_audio(audioCtx);

	auto audioPar = inputPar.get_audio();
	auto ret = avcodec_parameters_to_context(audioCtx, audioPar);
	if (ret < 0) {
		throw avException("Unable to set input audio context parameters");
	}

	// Initialize the AVCodecContext to use the given audio AVCodec.
	ret = avcodec_open2(audioCtx, audio, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the audio av codec");
	}
}

void Codec::source_video_context() {
	auto video = input.get_video();
	auto videoCtx = avcodec_alloc_context3(video);
	if (videoCtx == nullptr) {
		throw avException("Unable to get input video codec context");
	}
	inputContext.set_video(videoCtx);

	auto videoPar = inputPar.get_video();
	auto ret = avcodec_parameters_to_context(videoCtx, videoPar);
	if (ret < 0) {
		throw avException("Unable to set input video context parameters");
	}

	// Initialize the AVContext to use the given video AVCodec.
	ret = avcodec_open2(videoCtx, video, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the video av codec");
	}
}

void Codec::destination_audio_context() {
	auto audio = output.get_audio();
	auto audioCtx = avcodec_alloc_context3(audio);
	if (!audioCtx) {
		throw avException("Error in allocating the audio codec context");
	}
	outputContext.set_audio(audioCtx);

	auto audioPar = outputPar.get_audio();
	auto ret = avcodec_parameters_to_context(audioCtx, audioPar);
	if (ret < 0) {
		throw avException("Unable to set output codec context parameters");
	}

	// Initialize the AVContext to use the given video AVCodec.
	ret = avcodec_open2(audioCtx, audio, nullptr);
	if (ret < 0) {
		throw avException("Error in opening the audio avcodec");
	}
}

void Codec::destination_video_context() {
	auto video = output.get_video();
	if (!video) {
		throw avException("Error in finding the video av codecs.");
	}
	output.set_video(video);

	auto videoCtx = avcodec_alloc_context3(video);
	if (!videoCtx) {
		throw avException("Error in allocating the video codec context");
	}
	videoCtx->gop_size = VIDEO_GOP_SIZE;
	videoCtx->max_b_frames = VIDEO_MAX_B_FRAMES;
	videoCtx->time_base = {1, VIDEO_FRAMERATE};

	outputContext.set_video(videoCtx);

	auto videoPar = outputPar.get_video();
	auto ret = avcodec_parameters_to_context(videoCtx, videoPar);
	if (ret < 0) {
		throw avException("Unable to set output codec context parameters");
	}

	// Initialize the AVContext to use the given video AVCodec.
	ret = avcodec_open2(videoCtx, video, nullptr);
	if (ret < 0) {
		throw avException("Error in opening the video avcodec");
	}
}

void Codec::find_audio_encoder(const std::string &codec_name) {
#ifdef linux
	auto audio = avcodec_find_encoder_by_name(codec_name.c_str());
#elif WIN32
    auto audio = avcodec_find_encoder_by_name(codec_name.c_str());
#else
	auto audio = (AVCodec*) avcodec_find_encoder_by_name(codec_name.c_str());
#endif
	if (!audio) {
		throw avException("Error in finding the audio av codecs.");
	}
	output.set_audio(audio);
}

void Codec::find_video_encoder(const std::string &codec_name) {
	auto video = (AVCodec*) avcodec_find_encoder_by_name(codec_name.c_str());
    if (!video) {
        video = (AVCodec*) avcodec_find_encoder_by_name(FALLBACK_VIDEO_CODEC);
        std::cerr << "Requested codec was not found, selecting default fallback codec: " << FALLBACK_VIDEO_CODEC <<std::endl;
        if (!video) {
            throw avException("Error in finding the video av codecs");
        }
    }
	output.set_video(video);
}

/* Public methods */

void Codec::setup_source() {
	source_audio_context();
	source_video_context();
}

void Codec::setup_destination() {
	destination_audio_context();
	destination_video_context();
}

void Codec::find_encoders(const std::string &audio_codec, const std::string &video_codec) {
	find_audio_encoder(audio_codec);
	find_video_encoder(video_codec);
}

void Codec::open_streams(const Format &format) {
	auto video = avformat_new_stream(format.outputContext.get_video(), output.get_video());
	if (!video) {
		throw avException("Error in creating a av format new video streams");
	}
	video->time_base = {1, VIDEO_FRAMERATE};
	streams.set_video(video);

	auto audio = avformat_new_stream(format.outputContext.get_audio(), output.get_audio());
	if (!audio) {
		throw avException("Error in creating a av format new audio streams");
	}
	audio->time_base = {1, inputContext.get_audio()->sample_rate};
	streams.set_audio(audio);
}


void Codec::set_source_audio_layout(enum AudioLayout layout) {
	if (layout == MONO) {
		channel_layout = AV_CH_LAYOUT_MONO;
		channels = 1;
	} else if (layout == STEREO) {
		channel_layout = AV_CH_LAYOUT_STEREO;
		channels = 2;
	}
}

void Codec::set_source_audio_parameters(AVCodecParameters *par) {
	inputPar.set_audio(par);

	par->format = AV_SAMPLE_FMT_S16;
	par->sample_rate = AUDIO_SAMPLE_RATE;
	par->codec_id = AV_CODEC_ID_PCM_S16LE;
	par->codec_type = AVMEDIA_TYPE_AUDIO;
	par->frame_size = AUDIO_SAMPLE_RATE/2; // set number of audio samples in each frame
	par->channel_layout = channel_layout;
	par->channels = channels;

	auto audioCodec = (AVCodec*)  avcodec_find_decoder(par->codec_id);
	if (audioCodec == nullptr) {
		throw avException("Unable to find the audio decoder");
	}
	input.set_audio(audioCodec);
}

void Codec::set_source_video_parameters(AVCodecParameters *par) {
	inputPar.set_video(par);

	par->format = AV_PIX_FMT_BGR0;

	auto videoCodec = (AVCodec*) avcodec_find_decoder(par->codec_id);
	if (videoCodec == nullptr) {
		throw avException("Unable to find the video decoder");
	}
	input.set_video(videoCodec);
}

void Codec::set_destination_audio_parameters(AVCodecParameters *par) {
	outputPar.set_audio(par);

	par->codec_id = output.get_audio()->id;
	par->codec_type = AVMEDIA_TYPE_AUDIO;
	par->bit_rate = AUDIO_BITRATE;
	par->channels = inputContext.get_audio()->channels;
	par->channel_layout = inputContext.get_audio()->channel_layout;
	par->sample_rate = inputContext.get_audio()->sample_rate;
	par->format = output.get_audio()->sample_fmts[0];
	par->frame_size = AUDIO_SAMPLE_RATE/2;
}

void Codec::set_destination_video_parameters(AVCodecParameters *par) {
	outputPar.set_video(par);

	par->codec_id = output.get_video()->id;
	par->codec_type = AVMEDIA_TYPE_VIDEO;
	par->format = AV_PIX_FMT_YUV420P;
	par->bit_rate = VIDEO_BITRATE;
	par->width = inputContext.get_video()->width;
	par->height = inputContext.get_video()->height;
}

void Codec::reset() {
	inputContext.set_audio(nullptr);
	inputContext.set_video(nullptr);
	outputContext.set_audio(nullptr);
	outputContext.set_video(nullptr);
//    input.set_audio(nullptr);
//    input.set_video(nullptr);
//    output.set_audio(nullptr);
//    output.set_video(nullptr);
//    streams.set_audio(nullptr);
//    streams.set_video(nullptr);
}


