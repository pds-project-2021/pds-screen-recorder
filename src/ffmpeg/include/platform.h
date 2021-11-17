//
// Created by gabriele on 31/10/21.
//

/**
 * This file contains all platform specific code and functions
 */

#pragma once
#include <string>

#include "ffmpegc.h"

#define AUDIO               1
#define AUDIO_CHANNELS      1
#define AUDIO_SAMPLE_RATE   44100
#define AUDIO_MT            0
#define VIDEO_BITRATE       8*1000*1000
#define FRAME_COUNT         350
#define AUDIO_CODEC         86018 //86017 MP3; 86018 AAC;
#define AUDIO_BITRATE       192*1000

#ifdef _WIN32

#define DEFAULT_AUDIO_INPUT_FORMAT  "dhow"
#define DEFAULT_AUDIO_INPUT_DEVICE  ""
#define DEFAULT_VIDEO_INPUT_FORMAT  "gdigrab"
#define DEFAULT_VIDEO_INPUT_DEVICE  "desktop"

#define VIDEO_CODEC 27 //H264

#elif defined linux

#define DEFAULT_AUDIO_INPUT_FORMAT  "pulse"
#define DEFAULT_AUDIO_INPUT_DEVICE  "default"
#define DEFAULT_VIDEO_INPUT_FORMAT  "x11grab"
#define DEFAULT_VIDEO_INPUT_DEVICE  ":0.0"

#define VIDEO_CODEC 2 //MPEG2

#else

#define DEFAULT_AUDIO_INPUT_FORMAT  "pulse"
#define DEFAULT_AUDIO_INPUT_DEVICE  "default"
#define DEFAULT_VIDEO_INPUT_FORMAT  "x11grab"
#define DEFAULT_VIDEO_INPUT_DEVICE  ":0.0"

#define VIDEO_CODEC 2 //MPEG2

#endif

AVDictionary* get_audio_options();
AVDictionary* get_video_options();

std::string get_audio_input_format();
std::string get_audio_input_device();
std::string get_video_input_format();
std::string get_video_input_device();