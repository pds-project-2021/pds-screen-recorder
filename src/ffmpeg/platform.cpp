//
// Created by gabriele on 31/10/21.
//
#include "platform.h"

#ifdef _WIN32
AVDictionary* get_audio_options(){
	AVDictionary* options = nullptr;

	return options;
}

AVDictionary* get_video_options(){
	AVDictionary* options = nullptr;

	return options;
}

#elif defined linux

AVDictionary* get_audio_options(){
	AVDictionary* options = nullptr;
	av_dict_set(&options, "rtbufsize", "10M", 0);

	return options;
}

AVDictionary* get_video_options(){
	AVDictionary* options = nullptr;

	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "preset", "medium", 0);
	av_dict_set(&options, "offset_x", "0", 0);
	av_dict_set(&options, "offset_y", "0", 0);
//	av_dict_set(&options, "video_size", "1920x1080", 0);
	av_dict_set(&options, "show_region", "1", 0);
	return options;
}

#else

AVDictionary* get_audio_options(){
	AVDictionary* options = nullptr;

	return options;
}

AVDictionary* get_video_options(){
	AVDictionary* options = nullptr;

	return options;
}

#endif
