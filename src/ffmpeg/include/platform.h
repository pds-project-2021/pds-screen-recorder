//
// Created by gabriele on 31/10/21.
//

/**
 * This file contains all platform specific code and functions
 */

#pragma once

#include <string>
#include <vector>

#include "ffmpegc.h"
#include "wrapper.h"

/** platform agnostic attributes */

#define AUDIO_CHANNELS      1
#define AUDIO_SAMPLE_RATE   44100
#define VIDEO_BITRATE       8*1000*1000
#define AUDIO_BITRATE       192*1000

#define DEFAULT_AUDIO_CODEC "aac"

#ifdef _WIN32
/** WINDOWS platform specific attributes */

#include <iostream>

#include <windows.h>
#pragma comment(lib, "strmiids.lib")
#define DEFAULT_AUDIO_INPUT_FORMAT  "dshow"
#define DEFAULT_AUDIO_INPUT_DEVICE   0
#define DEFAULT_VIDEO_INPUT_FORMAT  "gdigrab"
#define DEFAULT_VIDEO_INPUT_DEVICE  "desktop"
#define DEFAULT_VIDEO_CODEC         "h264_mf"


#elif defined linux
/** LINUX platform specific attributes */

#define DEFAULT_AUDIO_INPUT_FORMAT  "pulse"
#define DEFAULT_AUDIO_INPUT_DEVICE  "default"
#define DEFAULT_VIDEO_INPUT_FORMAT  "x11grab"
#define DEFAULT_VIDEO_INPUT_DEVICE  ":0.0"
#define DEFAULT_VIDEO_CODEC         "libx264"

#else
/** MACOS platform specific attributes */

// todo: add macOs default parameters

#define DEFAULT_AUDIO_INPUT_FORMAT  "pulse"
#define DEFAULT_AUDIO_INPUT_DEVICE  "default"
#define DEFAULT_VIDEO_INPUT_FORMAT  "x11grab"
#define DEFAULT_VIDEO_INPUT_DEVICE  ":0.0"
#define DEFAULT_VIDEO_CODEC         "libx264"

#endif

AVDictionary* get_audio_options();
AVDictionary* get_video_options();

std::string get_audio_input_format();
std::string get_audio_input_device();
std::string get_video_input_format();
std::string get_video_input_device(const Screen &screen);

int64_t get_ref_time(const wrapper<AVFormatContext>& ctx);
