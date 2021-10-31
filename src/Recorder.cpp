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


// todo: add params for recording: widht, heigth, offsets
void Recorder::init() {
	format.init_input();
//	format.init_output();
	format.print_info();
}

