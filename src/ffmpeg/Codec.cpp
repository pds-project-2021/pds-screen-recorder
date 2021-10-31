//
// Created by gabriele on 31/10/21.
//

#include "include/Codec.h"

void Codec::init_input(Format &format) {
	auto videoCtx = format.inputContext.get_video();
	auto videoIndex = format.videoStreamIndex;

	auto videoPar = (*videoCtx)->streams[videoIndex]->codecpar;
	inputPar.set_video(videoPar);
	//inputCodecPar->format = AV_PIX_FMT_BGR0;

	auto audioCtx = format.inputContext.get_video();
	auto audioIndex = format.audioStreamIndex;

	auto audioPar = (*audioCtx)->streams[audioIndex]->codecpar;
	inputPar.set_audio(audioPar);

	audioPar->format = AV_SAMPLE_FMT_S16;
	audioPar->sample_rate = AUDIO_SAMPLE_RATE;
	audioPar->channel_layout = AUDIO_CHANNELS==2?AV_CH_LAYOUT_STEREO:AV_CH_LAYOUT_MONO;
	audioPar->channels = AUDIO_CHANNELS;
	audioPar->codec_id = AV_CODEC_ID_PCM_S16LE;
	audioPar->codec_type = AVMEDIA_TYPE_AUDIO;
	audioPar->frame_size = 22050; // set number of audio samples in each frame

	auto videoCodec = avcodec_find_decoder(videoPar->codec_id);
	input.set_video(videoCodec);
	if (videoCodec == nullptr) {
		throw avException("Unable to find the video decoder");
	}

	auto audioCodec = avcodec_find_decoder(audioPar->codec_id);
	input.set_audio(audioCodec);
	if (audioCodec == nullptr) {
		throw avException("Unable to find the audio decoder");
	}

	auto videoCodecCtx = avcodec_alloc_context3(videoCodec);
	inputContext.set_video(videoCodecCtx);
	if (videoCodecCtx == nullptr) {
		throw avException("Unable to get input video codec context");
	}
	avcodec_parameters_to_context(videoCodecCtx, videoPar);

	auto audioCodecCtx = avcodec_alloc_context3(audioCodec);
	inputContext.set_audio(audioCodecCtx);
	if (audioCodecCtx == nullptr) {
		throw avException("Unable to get input audio codec context");
	}
	avcodec_parameters_to_context(audioCodecCtx, audioPar);

	// Initialize the AVCodecContext to use the given video AVCodec.
	auto ret = avcodec_open2(videoCodecCtx, videoCodec, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the video av codec");
	}

	// Initialize the AVCodecContext to use the given audio AVCodec.
	ret = avcodec_open2(audioCodecCtx, audioCodec, nullptr);
	if (ret < 0) {
		throw avException("Unable to open the audio av codec");
	}

}
