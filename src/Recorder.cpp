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
	format.init_input();
	codec.init_input(format);
//	format.init_output();
	format.print_info();
}

void Recorder::print_info(){
	av_dump_format(*format.inputContext.get_video(), 0 , VIDEO_INPUT_FORMAT_CONTEXT, 0);
	av_dump_format(*format.inputContext.get_audio(), 0, AUDIO_INPUT_FORMAT_CONTEXT, 0);
}

