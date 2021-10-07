#include "ScreenRecorder.h"
#include "exceptions/avException.h"
#include "exceptions/fsException.h"
#include "exceptions/dataException.h"

ScreenRecorder::ScreenRecorder() {
//  thread_exit = 0;

  avformat_network_init();
  pFormatCtx = avformat_alloc_context();
  // Open File
  // char filepath[]="src01_480x272_22.h265";
  // avformat_open_input(&pFormatCtx,filepath,NULL,NULL)

  // Register Device
  avdevice_register_all();

  // Windows
#ifdef _WIN32
  // Use gdigrab
  options = nullptr;
  // Set some options
  // grabbing frame rate
  // av_dict_set(&options,"framerate","5",0);
  // The distance from the left edge of the screen or desktop
  // av_dict_set(&options,"offset_x","20",0);
  // The distance from the top edge of the screen or desktop
  // av_dict_set(&options,"offset_y","40",0);
  // Video frame size. The default is to capture the full screen
  // av_dict_set(&options,"video_size","640x480",0);
  ifmt = av_find_input_format("gdigrab");
  if (avformat_open_input(&pFormatCtx, "desktop", ifmt, &options) != 0) {
    throw avException("Couldn't open input stream");
  }
#elif defined linux
  // Linux
  options = nullptr;
  // Set some options
  // grabbing frame rate
  // av_dict_set(&options,"framerate","5",0);
  // Make the grabbed area follow the mouse
  // av_dict_set(&options,"follow_mouse","centered",0);
  // Video frame size. The default is to capture the full screen
  // av_dict_set(&options,"video_size","640x480",0);
  ifmt = av_find_input_format("x11grab");
  // Grab at position 10,20
  if (avformat_open_input(&pFormatCtx, ":0.0+0.0", ifmt, &options) != 0) {
    throw avException("Couldn't open input stream");
  }
#else
  show_avfoundation_device();
  // Mac
  ifmt = av_find_input_format("avfoundation");
  // Avfoundation
  //[video]:[audio]
  if (avformat_open_input(&pFormatCtx, "1", ifmt, NULL) != 0) {
    throw avException("Couldn't open input stream");
  }
#endif
}

int ScreenRecorder::sfp_refresh_thread(void *opaque) {
//  thread_exit = 0;
//  while (1) {
//    SDL_Event event;
//    event.type = SFM_REFRESH_EVENT;
//    SDL_PushEvent(&event);
//    SDL_Delay(40);
//  }
//  thread_exit = 0;
  // Break
//  SDL_Event event;
//  event.type = SFM_BREAK_EVENT;
//  SDL_PushEvent(&event);
//
//  return 0;
}

// Show Dshow Device
void ScreenRecorder::show_dshow_device() {
  pFormatCtx = avformat_alloc_context();
  options = nullptr;
  av_dict_set(&options, "list_devices", "true", 0);
  AVInputFormat *iformat = av_find_input_format("dshow");
  printf("========Device Info=============\n");
  avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
  printf("================================\n");
}

// Show AVFoundation Device
void ScreenRecorder::show_avfoundation_device() {
  pFormatCtx = avformat_alloc_context();
  options = nullptr;
  av_dict_set(&options, "list_devices", "true", 0);
  AVInputFormat *iformat = av_find_input_format("avfoundation");
  printf("==AVFoundation Device Info===\n");
  avformat_open_input(&pFormatCtx, "", iformat, &options);
  printf("=============================\n");
}

int ScreenRecorder::start_recording() {
  if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
    throw avException("Couldn't find stream information");
  }
  video_index = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
    if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_index = i;
      break;
    }
  if (video_index == -1) {
    throw avException("Didn't find a video stream");
  }
  pCodecPar = pFormatCtx->streams[video_index]->codecpar;
  pCodec = avcodec_find_decoder(pCodecPar->codec_id);
  auto pCodecCtx = pFormatCtx->streams[video_index]->codec;
  if (pCodec == nullptr) {
    throw avException("Codec not found");
  }
  if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
    throw avException("Could not open codec");
  }
  AVFrame *pFrame, *pFrameYUV;
  pFrame = av_frame_alloc();
  pFrameYUV = av_frame_alloc();
  // unsigned char *out_buffer=(unsigned char
  // *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width,
  // pCodecCtx->height)); avpicture_fill((AVPicture *)pFrameYUV, out_buffer,
  // AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
  // SDL----------------------------
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    throw avException(std::string("Could not initialize SDL - ") + SDL_GetError());
  }

  int screen_w = 640, screen_h = 360;
  SDL_DisplayMode *dm;
  SDL_GetCurrentDisplayMode(0, dm);
  // Half of the Desktop's width and height.
  screen_w = dm->w / 2;
  screen_h = dm->h / 2;
  SDL_Window *screen;
  screen = SDL_CreateWindow("Video Recording", 0, 0, screen_w, screen_h, 0);

  if (!screen) {
    throw avException(std::string("SDL: could not set video mode - exiting:") + SDL_GetError());
  }
  SDL_Renderer *renderer = SDL_CreateRenderer(screen, -1, 0);
  if (!renderer) {
    throw avException(std::string("SDL: could not create renderer - exiting:")+ SDL_GetError());
  }

  SDL_Texture *txr = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       pCodecCtx->width, pCodecCtx->height);
  SDL_Rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.w = screen_w;
  rect.h = screen_h;
  // SDL End------------------------
  int ret, got_picture;

  AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));

  struct SwsContext *img_convert_ctx;
  img_convert_ctx = sws_getContext(
      pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width,
      pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
  //------------------------------
  SDL_Thread *video_tid = SDL_CreateThread(sfp_refresh_thread, "video", nullptr);
  // Event Loop
  SDL_Event event;

  for (;;) {
    // Wait
    SDL_WaitEvent(&event);
    if (event.type == SFM_REFRESH_EVENT) {
      //------------------------------
      if (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == video_index) {
          // TODO update deprecated call
          ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
          if (ret < 0) {
            throw avException("Decode Error");
          }
          if (got_picture) {
//            SDL_LockYUVOverlay(bmp);
//            pFrameYUV->data[0] = bmp->pixels[0];
//            pFrameYUV->data[1] = bmp->pixels[2];
//            pFrameYUV->data[2] = bmp->pixels[1];
//            pFrameYUV->linesize[0] = bmp->pitches[0];
//            pFrameYUV->linesize[1] = bmp->pitches[2];
//            pFrameYUV->linesize[2] = bmp->pitches[1];
//            sws_scale(img_convert_ctx,
//                      (const unsigned char *const *)pFrame->data,
//                      pFrame->linesize, 0, pCodecCtx->height, pFrameYUV->data,
//                      pFrameYUV->linesize);
//
//            SDL_UnlockYUVOverlay(bmp);
//
//            SDL_DisplayYUVOverlay(bmp, &rect);
          }
        }
        av_packet_unref(packet);
      } else {
        // Exit Thread
//        thread_exit = 1;
      }
    } else if (event.type == SDL_QUIT) {
//      thread_exit = 1;
    } else if (event.type == SFM_BREAK_EVENT) {
      break;
    }
  }

  sws_freeContext(img_convert_ctx);
  SDL_Quit();

  av_free(pFrameYUV);
  avcodec_close(pCodecCtx);
  avformat_close_input(&pFormatCtx);
  return 0;
}