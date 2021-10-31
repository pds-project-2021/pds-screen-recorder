//
// Created by gabriele on 31/10/21.
//

/**
 * This file contains all platform specific code and functions
 */

#pragma once

#include "ffmpegc.h"

#ifdef _WIN32

#elif defined linux

#define AUDIO_INPUT_FORMAT          "pulse"
#define AUDIO_INPUT_FORMAT_CONTEXT  "default"
#define VIDEO_INPUT_FORMAT          "x11grab"
#define VIDEO_INPUT_FORMAT_CONTEXT  ":0.0"


#else

#endif

AVDictionary* get_audio_options();
AVDictionary* get_video_options();

