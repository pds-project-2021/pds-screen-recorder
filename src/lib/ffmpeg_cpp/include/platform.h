
/**
 * This file contains all platform specific code and functions
 */

#pragma once

#include <string>
#include <vector>

#include "ffmpegc.h"
#include "wrapper.h"

/** platform agnostic attributes */

#define AUDIO_SAMPLE_RATE   44100
#define VIDEO_BITRATE       8*1000*1000
#define AUDIO_BITRATE       128*1000
#define VIDEO_FRAMERATE     30
#define VIDEO_GOP_SIZE      15
#define VIDEO_MAX_B_FRAMES  10
#define PTS_SYNC_MULTIPLIER 1000000

#ifdef WIN32
/** WINDOWS platform specific attributes */

#include <iostream>
#include <windows.h>

#pragma comment(lib, "strmiids.lib")
#define DEFAULT_AUDIO_INPUT_FORMAT  "dshow"
#define DEFAULT_AUDIO_INPUT_DEVICE  0
#define DEFAULT_VIDEO_INPUT_FORMAT  "gdigrab"
#define DEFAULT_VIDEO_INPUT_DEVICE  "desktop"
#define DEFAULT_VIDEO_CODEC         "h264_mf"
#define FALLBACK_VIDEO_CODEC        "mpeg2video"
#define DEFAULT_AUDIO_CODEC         "aac"


#elif defined linux
/** LINUX platform specific attributes */

#define DEFAULT_AUDIO_INPUT_FORMAT  "pulse"
#define DEFAULT_AUDIO_INPUT_DEVICE  "default"
#define DEFAULT_VIDEO_INPUT_FORMAT  "x11grab"
#define DEFAULT_VIDEO_INPUT_DEVICE  ":0.0"
#define DEFAULT_VIDEO_CODEC         "libx264"
#define FALLBACK_VIDEO_CODEC        "mpeg2video"
#define DEFAULT_AUDIO_CODEC         "aac"

#else
/** MACOS platform specific attributes */

// todo: add macOs default parameters

#define DEFAULT_AUDIO_INPUT_FORMAT  "avfoundation"
#define DEFAULT_AUDIO_INPUT_DEVICE  ":0"
#define DEFAULT_VIDEO_INPUT_FORMAT  "avfoundation"
#define DEFAULT_VIDEO_INPUT_DEVICE  "0"
#define DEFAULT_VIDEO_CODEC         "libx264"
#define FALLBACK_VIDEO_CODEC        "mpeg2video"
#define DEFAULT_AUDIO_CODEC         "aac"

#endif

AVDictionary *get_audio_options();
AVDictionary *get_video_options();

std::string get_audio_input_format();
std::string get_audio_input_device();
std::string get_video_input_format();
std::string get_video_input_device(const std::string &offset);

int64_t get_ref_time(const wrapper<AVFormatContext> &ctx);
