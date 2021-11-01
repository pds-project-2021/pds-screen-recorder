//
// Created by gabriele on 31/10/21.
//

#include "include/Codec.h"

/* Private methods */
void Codec::init_audio_context() {
	auto audio = input.get_audio();
	auto audioCtx = avcodec_alloc_context3(audio);
	if (audioCtx == nullptr) {
		throw avException("Unable to get input audio codec context");
	}
	inputContext.set_audio(audioCtx);

	auto audioPar = inputPar.get_audio();
	avcodec_parameters_to_context(audioCtx, audioPar);
	// Initialize the AVCodecContext to use the given audio AVCodec.
	auto ret = avcodec_open2(audioCtx, audio, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the audio av codec");
	}
}

void Codec::init_video_context() {
	auto video = input.get_video();
	auto videoCtx = avcodec_alloc_context3(video);
	if (videoCtx == nullptr) {
		throw avException("Unable to get input video codec context");
	}
	inputContext.set_video(videoCtx);

	auto videoPar = inputPar.get_video();
	avcodec_parameters_to_context(videoCtx, videoPar);

	// Initialize the AVContext to use the given video AVCodec.
	auto ret = avcodec_open2(videoCtx, video, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the video av codec");
	}
}

/* Public methods */
void Codec::set_audio_parameters(AVCodecParameters *audioPar) {
	inputPar.set_audio(audioPar);

	audioPar->format = AV_SAMPLE_FMT_S16;
	audioPar->sample_rate = AUDIO_SAMPLE_RATE;
	audioPar->channel_layout = AUDIO_CHANNELS==2?AV_CH_LAYOUT_STEREO:AV_CH_LAYOUT_MONO;
	audioPar->channels = AUDIO_CHANNELS;
	audioPar->codec_id = AV_CODEC_ID_PCM_S16LE;
	audioPar->codec_type = AVMEDIA_TYPE_AUDIO;
	audioPar->frame_size = 22050; // set number of audio samples in each frame

	auto audioCodec = avcodec_find_decoder(audioPar->codec_id);
	if (audioCodec == nullptr) {
		throw avException("Unable to find the audio decoder");
	}
	input.set_audio(audioCodec);
}

void Codec::set_video_parameters(AVCodecParameters *videoPar) {
	inputPar.set_video(videoPar);
	//inputCodecPar->format = AV_PIX_FMT_BGR0;

	auto videoCodec = avcodec_find_decoder(videoPar->codec_id);
	if (videoCodec == nullptr) {
		throw avException("Unable to find the video decoder");
	}
	input.set_video(videoCodec);
}

void Codec::setup() {
	init_audio_context();
	init_video_context();
}

