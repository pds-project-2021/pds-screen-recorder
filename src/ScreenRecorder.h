#pragma once
#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
// Windows
extern "C" {
  #include "SDL2/SDL.h"
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
  #include <SDL2/SDL.h>
  #include <libavcodec/avcodec.h>
  #include <libavdevice/avdevice.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
#ifdef __cplusplus
};
#endif
#endif

// Refresh Event
#define SFM_REFRESH_EVENT (SDL_USEREVENT + 1)

#define SFM_BREAK_EVENT (SDL_USEREVENT + 2)

class ScreenRecorder {
private:
  AVFormatContext *pFormatCtx;
  AVCodecParameters *pCodecPar;
  AVCodec *pCodec;
  AVInputFormat *ifmt;
  AVDictionary *options;

  int i, video_index;

public:
  ScreenRecorder();

  static int sfp_refresh_thread(void *opaque);
  void show_dshow_device();
  void show_avfoundation_device();
  int start_recording();
};
