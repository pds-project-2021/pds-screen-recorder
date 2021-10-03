#pragma once
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "SDL2/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

//Output YUV420P 
#define OUTPUT_YUV420P 0
//'1' Use Dshow 
//'0' Use GDIgrab
#define USE_DSHOW 0

//Refresh Event
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT  (SDL_USEREVENT + 2)

class ScreenRecorder
{
private:
	AVFormatContext* pFormatCtx;
	int				i, videoindex, thread_exit;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	AVInputFormat* ifmt;
	AVDictionary* options;

public:
	int sfp_refresh_thread(void* opaque);
	void show_dshow_device();
	void show_avfoundation_device();
	void show_dshow_device();
	int start_recording();
};

