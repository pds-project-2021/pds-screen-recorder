#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <math.h>
#include <string.h>
extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}
#include "exceptions/avException.h"
#include "exceptions/dataException.h"
#include "exceptions/fsException.h"
#include<thread>
#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
#include <dshow.h>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
};
#else
// Linux and MacOS
#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"

#include "libavdevice/avdevice.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

// libav resample

#include "libavutil/opt.h"
#include "libavutil/common.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/file.h"
#ifdef __cplusplus
};
#endif
#endif

// Refresh Event


class ScreenRecorder {
private:
//  AVFormatContext *pFormatCtx;
//  AVCodecContext *pCodecCtx;
//  AVCodecParameters *pCodecPar;
//  AVCodec *pCodec;
//  AVInputFormat *ifmt;
    //  AVDictionary *options;

//  int i, video_index;

    //  -----------------------
    AVInputFormat *inputFormat = nullptr;
    AVInputFormat *audioInputFormat = nullptr;
    AVOutputFormat *outputFormat = nullptr;
    AVOutputFormat *audioOutputFormat = nullptr;
    AVCodecContext *inputCodecContext = nullptr;
    AVCodecContext *audioInputCodecContext = nullptr;
    AVCodecContext *outputCodecContext = nullptr;
    AVCodecContext *audioOutputCodecContext = nullptr;
    AVFormatContext *inputFormatContext = nullptr;
    AVFormatContext *audioInputFormatContext = nullptr;
    AVFormatContext *outputFormatContext = nullptr;
    AVFormatContext *audioOutputFormatContext = nullptr;
    AVCodecParameters *inputCodecPar = nullptr;
    AVCodecParameters *audioInputCodecPar = nullptr;
    AVCodecParameters *outputCodecPar = nullptr;
    AVCodecParameters *audioOutputCodecPar = nullptr;

    AVCodec *inputCodec = nullptr;
    AVCodec *audioInputCodec = nullptr;
    AVCodec *outputCodec = nullptr;
    AVCodec *audioOutputCodec = nullptr;
    AVDictionary *options = nullptr;

    AVStream *videoStream = nullptr;
    AVStream *audioStream = nullptr;
    SwsContext* swsContext = nullptr;
    SwrContext* swrContext = nullptr;

    const char *output_file = nullptr;

    double video_pts;

    int out_size;
    int codec_id;

public:
    ScreenRecorder();
    ~ScreenRecorder();

    /* function to initiate communication with display library */
    int init();
    int init_outputfile();
    int CaptureVideoFrames();

    //  ----------------

//  static int sfp_refresh_thread(void *opaque);
//  void show_avfoundation_device();
//  int start_recording();
};
