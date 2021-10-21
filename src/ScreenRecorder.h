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
}
#include "exceptions/avException.h"
#include "exceptions/dataException.h"
#include "exceptions/fsException.h"

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
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
  AVOutputFormat *outputFormat = nullptr;
  AVCodecContext *inputCodecContext = nullptr;
  AVCodecContext *outputCodecContext = nullptr;
  AVFormatContext *inputFormatContext = nullptr;
  AVFormatContext *outputFormatContext = nullptr;
  AVCodecParameters *inputCodecPar = nullptr;
  AVCodecParameters *outputCodecPar = nullptr;

  AVCodec *inputCodec = nullptr;
  AVCodec *outputCodec = nullptr;
  AVDictionary *options = nullptr;

  AVStream *videoStream = nullptr;
  SwsContext* swsContext = nullptr;

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
