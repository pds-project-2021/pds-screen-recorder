//
// Created by gabriele on 31/10/21.
//

#include "Recorder.h"


/** Initialize ffmpeg codecs and devices */
Recorder::Recorder() {

//	av_register_all(); // todo: maybe useless
//	avcodec_register_all(); // todo: maybe useless
	avdevice_register_all();

	record_video = false;
	frame_count = 0;

}


// todo: add params for recording: width, height, offsets
void Recorder::init() {
	format.setup();

	auto audioPar = format.get_audio_codec();
	auto videoPar = format.get_video_codec();

	codec.set_audio_parameters(audioPar);
	codec.set_video_parameters(videoPar);
	codec.setup();

	this->print_info();
}

void Recorder::print_info(){
	// todo cambiare nomi device
	av_dump_format(format.inputContext.get_video(), 0 , DEFAULT_VIDEO_INPUT_DEVICE, 0);
	av_dump_format(format.inputContext.get_audio(), 0, DEFAULT_AUDIO_INPUT_DEVICE, 0);
}

